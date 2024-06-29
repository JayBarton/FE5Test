
#include <string>
#include <cstdio>
#include <iostream>

#include <GL/glew.h>

#include "ResourceManager.h"
#include "SpriteRenderer.h"
#include "Camera.h"
#include "Timing.h"
#include "TextRenderer.h"
#include "TileManager.h"
#include "InputManager.h"
#include "Cursor.h"
#include "Unit.h"
#include "MenuManager.h"
#include "Items.h"
#include "BattleManager.h"
#include "EnemyManager.h"
#include "TextAdvancer.h"
#include "PlayerManager.h"
#include "SceneManager.h"

#include "Globals.h"
#include "InfoDisplays.h"
#include "Settings.h"

#include "SBatch.h"
#include "ShapeBatch.h"
#include "Minimap.h"

#include "Vendor.h"

#include "UnitResources.h"

#include "csv.h"
#include <nlohmann/json.hpp>

// for convenience
using json = nlohmann::json;

#include <vector>
#include <algorithm>
#include <map>
#include <cmath>
#include <SDL.h>
#include <SDL_Image.h>
#include <SDL_mixer.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtx/rotate_vector.hpp>

#include <sstream>
#include <fstream>

#include <random>
#include <ctime>

void init();
void Draw();
void DrawUnitRanges();
void DrawText();
void resizeWindow(int width, int height);

const static int TILE_SIZE = 16;

SDL_Window *window;
SpriteRenderer *Renderer;

GLuint shapeVAO;

TextRenderer* Text;

Camera camera;

Texture2D Texture;

std::string levelDirectory = "Levels/";
int levelWidth;
int levelHeight;

Cursor cursor;
PlayerManager playerManager;

SceneManager sceneManager;

InputManager inputManager;

BattleManager battleManager;

EnemyManager enemyManager;

InfoDisplays displays;

std::unordered_map<int, Unit*> sceneUnits;
std::vector<VisitObject> visitObjects;
std::vector<Vendor> vendors;

//unitID, dialogueID
std::unordered_map<int, int> gameOverDialogues;

enum GameOverState
{
	START,
	MESSAGE,
	FADE_OUT_OF_GAME,
	FADE_IN_BG,
	FADE_IN_TEXT
};
struct GameOverMode
{
	GameOverState state = START;
	bool active = false;
	bool canDraw = false;
	bool canExit = false;
	float gameOverMessageTimer = 0.0f;
	float gameOverMessageDelay = 0.2f;
	float fadeOutAlpha = 0.0f;
	float fadeInAlpha = 0.0f;
	float textAlpha = 0.0f;
	int messageID = -1;

	void init(int messageID)
	{
		this->messageID = messageID;
		active = true;
	}
	void Update(float deltaTime, InputManager& inputManager)
	{

		float fadeTime = 0.5f;
		switch (state)
		{
		case START:
			gameOverMessageTimer += deltaTime;
			if (gameOverMessageTimer >= gameOverMessageDelay)
			{
				displays.PlayerLost(messageID);
				state = MESSAGE;
			}
			break;
		case MESSAGE:
			if (displays.state == NONE)
			{
				state = FADE_OUT_OF_GAME;
			}
			break;
		case FADE_OUT_OF_GAME:
			fadeOutAlpha += fadeTime * deltaTime;
			if (fadeOutAlpha >= 1.0f)
			{
				fadeOutAlpha = 1.0f;
				state = FADE_IN_BG;
				canDraw = true;
			}
			break;
		case FADE_IN_BG:
			fadeInAlpha += fadeTime * deltaTime;
			if (fadeInAlpha >= 1)
			{
				fadeInAlpha = 1;
				state = FADE_IN_TEXT;
			}
			break;
		case FADE_IN_TEXT:
			textAlpha += fadeTime * deltaTime;
			if (textAlpha >= 1)
			{
				textAlpha = 1;
				canExit = true;
			}
			break;
		}
	}

	void DrawBG(SpriteRenderer* renderer, Camera& camera)
	{
		ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera.getOrthoMatrix());
		Renderer->setUVs();
		Texture2D displayTexture = ResourceManager::GetTexture("GameOver1");
		Renderer->DrawSprite(displayTexture, glm::vec2(0, 0), 0.0f, glm::vec2(256, 224), glm::vec4(1, 1, 1, fadeInAlpha));
		displayTexture = ResourceManager::GetTexture("GameOver2");
		Renderer->DrawSprite(displayTexture, glm::vec2(91, 172), 0.0f, glm::vec2(66, 20), glm::vec4(1, 1, 1, textAlpha));
	}
};
GameOverMode gameOverMode;

SBatch Batch;

Minimap minimap;

//Global
float unitSpeed = 2.5f;

int currentRound = 0;
int currentTurn = 0;
bool turnTransition = false;
bool turnDisplay = false;

//Ugh. To handle healing units on turn transition
int turnUnit = 0;

int idleFrame = 0;
int idleAnimationDirection = 1;
float timeForFrame = 0.0f;
float carryBlinkTime = 0.0f;

bool showCarry = false;

struct UnitEvents : public Observer<Unit*>
{
	virtual void onNotify(Unit* lUnit)
	{
		displays.OnUnitLevel(lUnit);
	}
};
Subject<int> roundSubject;
struct TurnEvents : public Observer<int>
{
	virtual void onNotify(int ID)
	{
		//Going to need instant feedback on enemy deaths in the future, but with how enemy manager's update works currently that won't work right now
		enemyManager.RemoveDeadUnits(sceneUnits);
		if (ID == 0)
		{
			cursor.focusedUnit = nullptr;
			for (int i = 0; i < playerManager.playerUnits.size(); i++)
			{
				playerManager.playerUnits[i]->EndTurn();
			}
			//Whatever enemy manager set up here
			//Probably going to want to figure out some sort of priority for the order in which enemies act
			turnTransition = true;
			turnDisplay = true;
			enemyManager.currentEnemy = 0;
			currentTurn = 1;
			std::cout << "Enemy Turn Start\n";
		}
		else if (ID == 1)
		{
			currentTurn = 0;
			//Start turn set up here
			//I'm just looping through right now, will need some different stuff set up to get heal animations playing properly
			turnTransition = true;
			turnDisplay = true;
			turnUnit = 0;
			currentRound++;
			roundSubject.notify(currentRound);
			if (Settings::settings.autoCursor)
			{
				cursor.SetFocus(playerManager.playerUnits[0]);
			}
		}
		displays.ChangeTurn(currentTurn);
	}
};

struct BattleEvents : public Observer<Unit*, Unit*>
{
	virtual void onNotify(Unit* attacker, Unit* defender)
	{
		if (currentTurn == 0)
		{
			displays.AddExperience(attacker, defender);
		}
		else
		{
			displays.AddExperience(defender, attacker);
		}
	}
};

struct DeathEvent : public Observer<Unit*>
{
	virtual void onNotify(Unit* deadUnit)
	{
		if (gameOverDialogues.find(deadUnit->sceneID) != gameOverDialogues.end())
		{
			gameOverMode.init(gameOverDialogues[deadUnit->sceneID]);
		}
		if (deadUnit->team == 0)
		{
			auto it = std::find(playerManager.playerUnits.begin(), playerManager.playerUnits.end(), deadUnit);
			sceneUnits.erase(deadUnit->sceneID);
			playerManager.playerUnits.erase(it);
			delete deadUnit;
		}
		//For use in the future
		else if (deadUnit->team == 1)
		{

		}
	}
};

struct PostBattleEvents : public Observer<int>
{
	virtual void onNotify(int ID)
	{
		switch (ID)
		{
			//Battle ended
		case 0:
			battleManager.EndBattle(&cursor, &enemyManager, camera);
			//Real brute force here
			battleManager.defender->sprite.moveAnimate = false;
			battleManager.defender->sprite.currentFrame = idleFrame;
			break;
			//Player used an item
		case 1:
			if (cursor.selectedUnit->isMounted() && cursor.selectedUnit->mount->remainingMoves > 0)
			{
				//If the player attacked we need to return control to the cursor
				cursor.GetRemainingMove();
			}
			else
			{
				cursor.Wait();
			}
			break;
			//Enemy used an item
			//Even if they can canto, if an enemy uses an item I want them to end their move
		case 2:
			enemyManager.FinishMove();
			break;
			//For healing tiles
		case 3:
			turnUnit++;
			break;
			//Player captures enemy after battle
		case 4:
			battleManager.CaptureUnit();
			break;
		}
		
	}
};
//This is identical to above so I'm not sure I even need it here...
struct ItemEvents : public Observer<Unit*, int>
{
	virtual void onNotify(Unit* unit, int index)
	{
		//Do something in display here...
		displays.StartUse(unit, index, &camera);
	}
};
void loadMap(std::string nextMap, UnitEvents* unitEvents);

std::mt19937 gen;
//gen.seed(1);
std::uniform_int_distribution<int> distribution(0, 99);
int main(int argc, char** argv)
{
	init();
	Batch.init();
	GLfloat deltaTime = 0.0f;
	GLfloat lastFrame = 0.0f;

	//Main loop flag
	bool isRunning = true;

	//Event handler
	SDL_Event event;

	int fps = 0;
	FPSLimiter fpsLimiter;
	fpsLimiter.setMaxFPS(69990.0f);

	const float MS_PER_SECOND = 1000;
	const float DESIRED_FPS = 60;
	const float DESIRED_FRAMETIME = MS_PER_SECOND / DESIRED_FPS;
	const float MAXIMUM_DELTA_TIME = 1.0f;

	const int MAXIMUM_STEPS = 6;

	float previousTicks = SDL_GetTicks();

	GLfloat verticies[] =
	{
		0.0f, 1.0f, // Left
		1.0f, 0.0f, // Right
		0.0f, 0.0f,  // Top
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f
	};

	GLuint tVBO;
	glGenVertexArrays(1, &shapeVAO);
	glGenBuffers(1, &tVBO);
	// Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
	glBindVertexArray(shapeVAO);

	glBindBuffer(GL_ARRAY_BUFFER, tVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verticies), verticies, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	camera.setPosition(glm::vec2(0.0f, 0.0f));
	camera.update();

	ResourceManager::LoadShader("Shaders/spriteVertexShader.txt", "Shaders/spriteFragmentShader.txt", nullptr, "sprite");
	ResourceManager::LoadShader("Shaders/normalSpriteVertexShader.txt", "Shaders/normalSpriteFragmentShader.txt", nullptr, "Nsprite");
	ResourceManager::LoadShader("Shaders/instanceVertexShader.txt", "Shaders/instanceFragmentShader.txt", nullptr, "instance");
	ResourceManager::LoadShader("Shaders/shapeVertexShader.txt", "Shaders/shapeFragmentShader.txt", nullptr, "shape");
	ResourceManager::LoadShader("Shaders/shapeInstanceVertexShader.txt", "Shaders/shapeInstanceFragmentShader.txt", nullptr, "shapeInstance");

	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera.getCameraMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);

	ResourceManager::GetShader("shapeInstance").Use().SetMatrix4("projection", camera.getCameraMatrix());
	ResourceManager::GetShader("shapeInstance").SetFloat("alpha", 1.0f);

	ResourceManager::GetShader("sprite").Use().SetInteger("image", 0);
	ResourceManager::GetShader("sprite").Use().SetInteger("palette", 1);
	ResourceManager::GetShader("sprite").SetMatrix4("projection", camera.getCameraMatrix());

	ResourceManager::GetShader("Nsprite").Use().SetInteger("image", 0);
	ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera.getCameraMatrix());

	ResourceManager::GetShader("instance").Use().SetInteger("image", 0);
	ResourceManager::GetShader("instance").SetMatrix4("projection", camera.getCameraMatrix());
	ResourceManager::GetShader("instance").Use().SetVector4f("spriteColor", glm::vec4(1));

//	ResourceManager::LoadShader("Shaders/spriteVertexShader.txt", "Shaders/sliceFragmentShader.txt", nullptr, "slice");

//	ResourceManager::LoadShader("Shaders/postVertexShader.txt", "Shaders/postFragmentShader.txt", nullptr, "postprocessing");

	ResourceManager::LoadTexture("E:/Damon/dev stuff/FE5Test/TestSprites/tilesheet2.png", "tiles");
	ResourceManager::LoadTexture("E:/Damon/dev stuff/FE5Test/TestSprites/cursor.png", "cursor");
	ResourceManager::LoadTexture2("E:/Damon/dev stuff/FE5Test/TestSprites/sprites.png", "sprites");
	ResourceManager::LoadTexture2("E:/Damon/dev stuff/FE5Test/TestSprites/movesprites.png", "movesprites");
	ResourceManager::LoadTexture("E:/Damon/dev stuff/FE5Test/TestSprites/palette.png", "palette");
	ResourceManager::LoadTexture("E:/Damon/dev stuff/FE5Test/TestSprites/icons.png", "icons");
	ResourceManager::LoadTexture("E:/Damon/dev stuff/FE5Test/TestSprites/carryingIcons.png", "carryingIcons");
	ResourceManager::LoadTexture("E:/Damon/dev stuff/FE5Test/TestSprites/gameovermain.png", "GameOver1");
	ResourceManager::LoadTexture("E:/Damon/dev stuff/FE5Test/TestSprites/gameovertext.png", "GameOver2");
	ResourceManager::LoadTexture("E:/Damon/dev stuff/FE5Test/TestSprites/Portraits.png", "Portraits");

	Shader myShader;
	myShader = ResourceManager::GetShader("Nsprite");
	Renderer = new SpriteRenderer(myShader);

	TileManager::tileManager.uvs = ResourceManager::GetTexture("tiles").GetUVs(TILE_SIZE, TILE_SIZE);

	cursor.uvs = ResourceManager::GetTexture("cursor").GetUVs(0, 0, TILE_SIZE, TILE_SIZE, 2, 1, 2);
	minimap.cursorUvs = ResourceManager::GetTexture("cursor").GetUVs(32, 0, 70, 62, 2, 1, 2);
	cursor.dimensions = glm::vec2(TileManager::TILE_SIZE);

	UnitResources::LoadUVs();
	UnitResources::LoadAnimData();

	Text = new TextRenderer(800, 600);
	Text->Load("fonts/Teko-Light.TTF", 30);
	ItemManager::itemManager.SetUpItems();
	UnitEvents* unitEvents = new UnitEvents();
	TurnEvents* turnEvents = new TurnEvents();
	BattleEvents* battleEvents = new BattleEvents();
	DeathEvent* deathEvents = new DeathEvent();
	PostBattleEvents* postBattleEvents = new PostBattleEvents();
	ItemEvents* itemEvents = new ItemEvents();
	ItemManager::itemManager.subject.addObserver(itemEvents);
	battleManager.subject.addObserver(battleEvents);
	battleManager.unitDiedSubject.addObserver(deathEvents);
	displays.subject.addObserver(postBattleEvents);
	playerManager.init(&gen, &distribution, unitEvents, &sceneUnits);

	loadMap("2.map", unitEvents);

	cursor.position = playerManager.playerUnits[0]->sprite.getPosition();
	cursor.focusedUnit = playerManager.playerUnits[0];

	MenuManager::menuManager.SetUp(&cursor, Text, &camera, shapeVAO, Renderer, &battleManager, &playerManager, &enemyManager);
	MenuManager::menuManager.subject.addObserver(turnEvents);
	enemyManager.subject.addObserver(turnEvents);
	enemyManager.unitEscapedSubject.addObserver(deathEvents);
	enemyManager.displays = &displays;

	while (isRunning)
	{
		GLfloat timeValue = SDL_GetTicks() / 1000.0f;
		// Calculate deltatime of current frame
		GLfloat currentFrame = timeValue;
		deltaTime = currentFrame - lastFrame;
		deltaTime = glm::clamp(deltaTime, 0.0f, 0.02f); //Clamped in order to prevent odd updates if there is a pause
		lastFrame = currentFrame;

		fpsLimiter.beginFrame();

		float newTicks = SDL_GetTicks();
		float frameTime = newTicks - previousTicks;
		previousTicks = newTicks;

		float totalDeltaTime = frameTime / DESIRED_FRAMETIME; //Consider deleting all of this.

		inputManager.update(deltaTime);
		//Handle events on queue
		while (SDL_PollEvent(&event) != 0)
		{
			switch (event.type)
			{
			case SDL_QUIT:
				//User requests quit
				isRunning = false;
				break;
			case SDL_KEYDOWN:
				inputManager.pressKey(event.key.keysym.sym);
				break;
			case SDL_KEYUP:
				inputManager.releaseKey(event.key.keysym.sym);
				break;
			case SDL_MOUSEWHEEL:
				//Not keeping this, just need it to get a sense of size
				camera.setScale(glm::clamp(camera.getScale() + event.wheel.y * 0.1f, 0.3f, 4.5f));
				break;

			case SDL_WINDOWEVENT:

				if (event.window.event == SDL_WINDOWEVENT_RESIZED)
				{
					resizeWindow(event.window.data1, event.window.data2);
				}
				break;
			}

			if (inputManager.isKeyPressed(SDLK_ESCAPE))
			{
				isRunning = false;
			}
		}

		timeForFrame += deltaTime;
		carryBlinkTime += deltaTime;
		float animationDelay = 0.0f;
		animationDelay = 0.27f;
		if (timeForFrame >= animationDelay)
		{
			timeForFrame = 0;
			if (idleAnimationDirection > 0)
			{
				if (idleFrame < 2)
				{
					idleFrame++;
				}
				else
				{
					idleAnimationDirection = -1;
					idleFrame--;
				}
			}
			else
			{
				if (idleFrame > 0)
				{
					idleFrame--;
				}
				else
				{
					idleAnimationDirection = 1;
					idleFrame++;
				}
			}
		}
		if (showCarry)
		{
			if (carryBlinkTime >= 1.0f)
			{
				carryBlinkTime = 0.0f;
				showCarry = !showCarry;
			}
		}
		else
		{
			if (carryBlinkTime >= 0.5f)
			{
				carryBlinkTime = 0.0f;
				showCarry = !showCarry;
			}
		}

		if (MenuManager::menuManager.menus.size() > 0)
		{
			MenuManager::menuManager.menus.back()->CheckInput(inputManager, deltaTime);
		}
		else if(sceneManager.PlayingScene())
		{
			sceneManager.scenes[sceneManager.currentScene]->Update(deltaTime, &playerManager, sceneUnits, camera, inputManager, cursor, displays);
			auto introUnits = sceneManager.scenes[sceneManager.currentScene]->introUnits;
			for (int i = 0; i < introUnits.size(); i++)
			{
				introUnits[i]->sprite.HandleAnimation(deltaTime, idleFrame);
			}
		}
		else
		{
			if (displays.state != NONE)
			{
				if (camera.moving)
				{
					camera.MoveTo(deltaTime, 5.0f);
				}
				else
				{
					displays.Update(deltaTime, inputManager);
				}
			}
			else
			{
				//if in battle, handle battle, don't check input
				//Should be able to level up while in the battle state, need to figure that out
				if (battleManager.battleActive)
				{
					battleManager.Update(deltaTime, &gen, &distribution, displays, inputManager);
					if (camera.moving)
					{
						camera.MoveTo(deltaTime, 5.0f);
					}
				}
				else
				{
					if (gameOverMode.active)
					{
						if(gameOverMode.canExit)
						{
							if (inputManager.isKeyPressed(SDLK_RETURN))
							{
								//Return to main menu
								isRunning = false;
							}
						}
						else
						{
							gameOverMode.Update(deltaTime, inputManager);
						}
					}
					else if (turnTransition)
					{
						if (currentTurn == 0)
						{
							if (turnUnit >= playerManager.playerUnits.size())
							{
								turnUnit = 0;
								camera.SetMove(cursor.position);
								turnTransition = false;
							}
							else
							{
								playerManager.playerUnits[turnUnit]->StartTurn(displays, &camera);

								if (displays.state == NONE)
								{
									turnUnit++;
								}
							}
						}
						else
						{
							if (turnUnit >= enemyManager.enemies.size())
							{
								turnUnit = 0;
								turnTransition = false;
							}
							else
							{
								enemyManager.enemies[turnUnit]->StartTurn(displays, &camera);

								if (displays.state == NONE)
								{
									turnUnit++;
								}
							}
						}
					}
					//nesting getting a little deep here
					else if (currentTurn == 0)
					{
						if (!minimap.show)
						{
							if (inputManager.isKeyPressed(SDLK_m))
							{
								minimap.show = true;
							}
							//Oh man I hate this
							if (!camera.moving)
							{
								cursor.CheckInput(inputManager, deltaTime, camera);
							}
							if (!camera.moving)
							{
								camera.Follow(cursor.position);
							}
							else
							{
								camera.MoveTo(deltaTime, 5.0f);
							}
						}
						else
						{
							if (inputManager.isKeyPressed(SDLK_m) || inputManager.isKeyPressed(SDLK_z))
							{
								minimap.show = false;
								cursor.position = camera.getPosition();
							}
							else
							{
								minimap.Update(inputManager, deltaTime, camera);
							}
						}
					}
					else
					{
						//enemy management
						enemyManager.Update(deltaTime, battleManager, camera, inputManager);
						if (!camera.moving)
						{
							if (enemyManager.followCamera)
							{
								if (auto enemy = enemyManager.GetCurrentUnit())
								{
									camera.Follow(enemy->sprite.getPosition());
								}
							}
						}
						else
						{
							camera.MoveTo(deltaTime, 5.0f);
						}
					}
				}
			}
		}
		//These two update functions are basically just going to handle animations
		//if (!sceneManager.scenes[sceneManager.currentScene]->playingScene)
		{
			playerManager.Update(deltaTime, idleFrame, inputManager);
		}
		//if (!camera.moving)
		{
			enemyManager.UpdateEnemies(deltaTime, idleFrame);
		}
		camera.update();

		//ugh
		for (int i = 0; i < visitObjects.size(); i++)
		{
			if (visitObjects[i].toDelete)
			{
				TileManager::tileManager.getTile(visitObjects[i].position.x, visitObjects[i].position.y)->visitSpot = nullptr;
				TileManager::tileManager.getTile(visitObjects[i].position.x, visitObjects[i].position.y)->uvID = 30;
				TileManager::tileManager.reDraw();
				visitObjects[i].sceneMap.clear();
				visitObjects.erase(visitObjects.begin() + i);
				i--;
			}
		}

		Draw();

		fps = fpsLimiter.end();
		//std::cout << fps << std::endl;
	}

	delete Renderer;
	delete Text;
	enemyManager.Clear();
	playerManager.Clear();
	for (int i = 0; i < sceneManager.scenes.size(); i++)
	{
		delete sceneManager.scenes[i];
	}
	/*for (int i = 0; i < soundEffects.size(); i++)
	{
		for (int c = 0; c < soundEffects[i].size(); c++)
		{
			Mix_FreeChunk(soundEffects[i][c]);
			soundEffects[i][c] = nullptr;
		}
	}*/
	delete unitEvents;
	delete turnEvents;
	delete battleEvents;
	delete postBattleEvents;
	delete deathEvents;
	delete itemEvents;
//	unit.subject.observers.clear();

	MenuManager::menuManager.ClearMenu();
	TileManager::tileManager.clearTiles();

	SDL_DestroyWindow(window);
	window = nullptr;

	SDL_Quit();
	IMG_Quit();
	Mix_CloseAudio();
	Mix_Quit();

	return 0;
}

void init()
{
	int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
	SDL_Init(SDL_INIT_EVERYTHING);

	int imgFlags = IMG_INIT_PNG;
	if (!(IMG_Init(imgFlags) & imgFlags))
	{
		printf("SDL_image could not initialize! SDL_mage Error: %s\n", IMG_GetError());
	}

	//Initialize SDL_mixer
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
	{
		printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
	}

	SDL_GLContext context; //check if succesfully created later

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	//For multisampling
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

	window = SDL_CreateWindow("FE5", SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH,
		SCREEN_HEIGHT, flags);

	context = SDL_GL_CreateContext(window);
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::cout << "Failed to initialize GLEW" << std::endl;
		return;
	}

	SDL_GL_SetSwapInterval(1);

	//glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	resizeWindow(SCREEN_WIDTH, SCREEN_HEIGHT);
//	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glEnable(GL_MULTISAMPLE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void loadMap(std::string nextMap, UnitEvents* unitEvents)
{
	std::ifstream map(levelDirectory + nextMap);

	int xTiles = 0;
	int yTiles = 0;
	int intro = -1;
	std::string bg = "";

	while (map.good())
	{
		std::string thing;
		map >> thing;

		if (thing == "Level")
		{
			map >> xTiles >> yTiles;
			TileManager::tileManager.setTiles(map, xTiles, yTiles);
		}
		else if (thing == "Enemies")
		{
			enemyManager.SetUp(map, &gen, &distribution, &playerManager.playerUnits, &vendors);
			//proof of concept
			for (int i = 0; i < enemyManager.enemies.size(); i++)
			{
				auto unit = enemyManager.enemies[i];
				if (unit->sceneID >= 0)
				{
					sceneUnits[unit->sceneID] = unit;
				}
			}
		}
		else if (thing == "Starts")
		{
			playerManager.LoadUnits(map);
		}
		else if (thing == "Scenes")
		{
			int numberOfScenes = 0;
			map >> numberOfScenes;
			sceneManager.scenes.resize(numberOfScenes);
			for (int i = 0; i < numberOfScenes; i++)
			{
				int numberOfActions = 0;
				map >> numberOfActions;
				sceneManager.scenes[i] = new Scene();
				auto currentObject = sceneManager.scenes[i];
				currentObject->ID = i;
				currentObject->owner = &sceneManager;
				currentObject->actions.resize(numberOfActions);
				for (int c = 0; c < numberOfActions; c++)
				{
					int actionType = 0;
					map >> actionType;
					if (actionType == CAMERA_ACTION)
					{
						glm::vec2 position;
						map >> position.x >> position.y;
						currentObject->actions[c] = new CameraMove(actionType, position);
					}
					else if (actionType == NEW_UNIT_ACTION)
					{
						int unitID;
						glm::vec2 start;
						glm::vec2 end;
						map >> unitID >> start.x >> start.y >> end.x >> end.y;
						currentObject->actions[c] = new AddUnit(actionType, unitID, start, end);
					}
					else if (actionType == MOVE_UNIT_ACTION)
					{
						int unitID;
						glm::vec2 end;
						map >> unitID >> end.x >> end.y;
						currentObject->actions[c] = new UnitMove(actionType, unitID, end);
					}
					else if (actionType == DIALOGUE_ACTION)
					{
						int dialogueID;
						map >> dialogueID;
						currentObject->actions[c] = new DialogueAction(actionType, dialogueID);
					}
					else if (actionType == ITEM_ACTION)
					{
						int itemID = 0;
						map >> itemID;
						currentObject->actions[c] = new ItemAction(actionType, itemID);
					}
					else if (actionType == NEW_SCENE_UNIT_ACTION)
					{
						int unitID;
						int team;
						int pathSize;
						float nextDelay;
						float moveDelay;
						std::vector<glm::ivec2> path;
						map >> unitID >> team >> pathSize;
						path.resize(pathSize);
						for (int i = 0; i < pathSize; i++)
						{
							map >> path[i].x >> path[i].y;
						}
						map >> nextDelay >> moveDelay;
						currentObject->actions[c] = new AddSceneUnit(actionType, unitID, team, path, nextDelay, moveDelay);
					}
					else if (actionType == SCENE_UNIT_MOVE_ACTION)
					{
						int unitID;
						int pathSize;
						int facing;
						float nextDelay;
						float moveSpeed;
						std::vector<glm::ivec2> path;
						map >> unitID >> pathSize;
						path.resize(pathSize);
						for (int i = 0; i < pathSize; i++)
						{
							map >> path[i].x >> path[i].y;
						}
						map >> nextDelay >> moveSpeed >> facing;
						currentObject->actions[c] = new SceneUnitMove(actionType, unitID, path, nextDelay, moveSpeed, facing);
					}
					else if (actionType == SCENE_UNIT_REMOVE_ACTION)
					{
						int unitID;
						float nextDelay;

						map >> unitID >> nextDelay;
						currentObject->actions[c] = new SceneUnitRemove(actionType, unitID, nextDelay);
					}
				}
				int activationType = 0;
				map >> activationType;
				if (activationType == 0)
				{
					int talker = 0;
					int listener = 0;
					map >> talker >> listener;
					//Add a reference to this scene to the talking unit
					//Probably don't need this scene to exist at all if the talker is not in the level
					if (sceneUnits.count(talker))
					{
						currentObject->activation = new TalkActivation(currentObject, activationType, talker, listener);
						sceneUnits[talker]->talkData.push_back({ currentObject, listener });
					}

				}
				else if (activationType == 1)
				{
					int round = 0;
					map >> round;
					currentObject->activation = new EnemyTurnEnd(currentObject, activationType, round);
					currentObject->extraSetup(&roundSubject);
				}
				else if (activationType == 2)
				{
					currentObject->activation = new VisitActivation(currentObject, activationType);
				}
				else if (activationType == 3)
				{
					intro = i;
					currentObject->activation = new IntroActivation(currentObject, activationType);
				}

				map >> currentObject->repeat;
			}
		}
		else if (thing == "Visits")
		{
			int numberOfVisits = 0;
			map >> numberOfVisits;
			visitObjects.resize(numberOfVisits);
			for (int i = 0; i < numberOfVisits; i++)
			{
				glm::ivec2 position;
				map >> position.x >> position.y;
				int numberOfIDs = 0;
				map >> numberOfIDs;
				for (int c = 0; c < numberOfIDs; c++)
				{
					int unitID = 0;
					int sceneID = 0;
					map >> unitID >> sceneID;
					visitObjects[i].position = position;
					visitObjects[i].sceneMap[unitID] = sceneManager.scenes[sceneID];
					sceneManager.scenes[sceneID]->visit = &visitObjects[i];
				}
				TileManager::tileManager.placeVisit(position.x, position.y, &visitObjects[i]);
			}
		}
		else if (thing == "Vendors")
		{
			int numberOfVendors = 0;
			map >> numberOfVendors;
			vendors.resize(numberOfVendors);
			for (int i = 0; i < numberOfVendors; i++)
			{
				glm::ivec2 position;
				map >> position.x >> position.y;
				int numberOfItems = 0;
				map >> numberOfItems;
				std::vector<int> items;
				items.resize(numberOfItems);
				for (int c = 0; c < numberOfItems; c++)
				{
					map >> items[c];
				}
				vendors[i] = Vendor{ items, position };
				TileManager::tileManager.placeVendor(position.x, position.y, &vendors[i]);
			}
		}
		else if (thing == "Requirements")
		{
			int requiredUnits = 0;
			map >> requiredUnits;
			for (int i = 0; i < requiredUnits; i++)
			{
				int ID;
				map >> ID;
				map >> gameOverDialogues[ID];
			}
		}
	}
	if (intro >= 0)
	{
	//	sceneManager.scenes[intro]->init();
	}
	/*Scene* intro = new Scene();
	intro->ID = 10;
	intro->owner = &sceneManager;
	intro->actions.resize(4);
	intro->actions[0] = new CameraMove(CAMERA_ACTION, glm::vec2(208, 112));
	std::vector<glm::ivec2> path;
	path.push_back(glm::ivec2(272, 80));
	intro->actions[1] = new AddSceneUnit(NEW_SCENE_UNIT_ACTION, 10, 1, path);
	path.push_back(glm::ivec2(272, 96));
	intro->actions[2] = new AddSceneUnit(NEW_SCENE_UNIT_ACTION, 2, 1, path);
	intro->actions[3] = new DialogueAction(DIALOGUE_ACTION, 12);

	path.clear();
	path.push_back(glm::ivec2(288, 224));
	path.push_back(glm::ivec2(288, 192));
	path.push_back(glm::ivec2(240, 192));
	path.push_back(glm::ivec2(240, 128));
	path.push_back(glm::ivec2(272, 128));
	intro->actions.push_back(new AddSceneUnit(NEW_SCENE_UNIT_ACTION, 0, 1, path, 0.0f, 0.0f));

	path.clear();
	path.push_back(glm::ivec2(272, 232));
	path.push_back(glm::ivec2(272, 224));
	path.push_back(glm::ivec2(272, 192));
	path.push_back(glm::ivec2(256, 192));
	path.push_back(glm::ivec2(256, 128));
	intro->actions.push_back(new AddSceneUnit(NEW_SCENE_UNIT_ACTION, 1, 1, path));

	path.clear();
	path.push_back(glm::ivec2(272, 96));
	path.push_back(glm::ivec2(288, 96));
	path.push_back(glm::ivec2(288, 112));
	intro->actions.push_back(new SceneUnitMove(SCENE_UNIT_MOVE_ACTION, 1, path));
	path.clear();
	path.push_back(glm::ivec2(272, 80));
	path.push_back(glm::ivec2(272, 96));
	intro->actions.push_back(new SceneUnitMove(SCENE_UNIT_MOVE_ACTION, 0, path));
	
	//Add Nanna and Mareeta
	path.clear();
	path.push_back(glm::ivec2(256, 112));
	intro->actions.push_back(new AddSceneUnit(NEW_SCENE_UNIT_ACTION, 11, 0, path, 0.1f));
	path.clear();
	path.push_back(glm::ivec2(272, 112));
	intro->actions.push_back(new AddSceneUnit(NEW_SCENE_UNIT_ACTION, 12, 0, path, 0.1f));
	intro->actions.push_back(new DialogueAction(DIALOGUE_ACTION, 13));
	sceneManager.scenes.push_back(intro);

	//Remove Nanna and Mareeta
	path.clear();
	path.push_back(glm::ivec2(272, 80));
	path.push_back(glm::ivec2(272, 112));
	intro->actions.push_back(new SceneUnitMove(SCENE_UNIT_MOVE_ACTION, 0, path));
	intro->actions.push_back(new SceneUnitRemove(SCENE_UNIT_REMOVE_ACTION, 5, 0.1f));

	path.clear();
	path.push_back(glm::ivec2(272, 112));
	path.push_back(glm::ivec2(256, 112));
	intro->actions.push_back(new SceneUnitMove(SCENE_UNIT_MOVE_ACTION, 0, path));
	intro->actions.push_back(new SceneUnitRemove(SCENE_UNIT_REMOVE_ACTION, 4, 0.1f));

	//Reydric exits
	path.clear();
	path.push_back(glm::ivec2(256, 112));
	path.push_back(glm::ivec2(256, 128));
	path.push_back(glm::ivec2(240, 128));
	path.push_back(glm::ivec2(240, 176));
	path.push_back(glm::ivec2(304, 176));
	path.push_back(glm::ivec2(304, 192));
	path.push_back(glm::ivec2(336, 192));
	intro->actions.push_back(new SceneUnitMove(SCENE_UNIT_MOVE_ACTION, 0, path));
	intro->actions.push_back(new SceneUnitRemove(SCENE_UNIT_REMOVE_ACTION, 0));

	//Enemies get into position
	path.clear();
	path.push_back(glm::ivec2(288, 112));
	path.push_back(glm::ivec2(288, 80));
	path.push_back(glm::ivec2(272, 80));
	intro->actions.push_back(new SceneUnitMove(SCENE_UNIT_MOVE_ACTION, 1, path));

	path.clear();
	path.push_back(glm::ivec2(208, 192));
	path.push_back(glm::ivec2(240, 192));
	intro->actions.push_back(new AddSceneUnit(NEW_SCENE_UNIT_ACTION, 0, 1, path));

	intro->actions.push_back(new CameraMove(CAMERA_ACTION, glm::vec2(240, 144)));

	path.clear();
	path.push_back(glm::ivec2(208, 160));
	intro->actions.push_back(new AddSceneUnit(NEW_SCENE_UNIT_ACTION, 0, 1, path, 0.1f));

	path.clear();
	path.push_back(glm::ivec2(144, 160));
	intro->actions.push_back(new AddSceneUnit(NEW_SCENE_UNIT_ACTION, 1, 1, path, 0.1f));

	path.clear();
	path.push_back(glm::ivec2(176, 48));
	path.push_back(glm::ivec2(192, 48));
	path.push_back(glm::ivec2(192, 64));
	path.push_back(glm::ivec2(160, 64));
	path.push_back(glm::ivec2(160, 128));
	intro->actions.push_back(new AddSceneUnit(NEW_SCENE_UNIT_ACTION, 0, 1, path, 0.0f, 0.1f));

	path.clear();
	path.push_back(glm::ivec2(288, 208));
	intro->actions.push_back(new AddSceneUnit(NEW_SCENE_UNIT_ACTION, 0, 1, path, 0.1f, 0.1f));

	path.clear();
	path.push_back(glm::ivec2(128, 192));
	path.push_back(glm::ivec2(160, 192));
	intro->actions.push_back(new AddSceneUnit(NEW_SCENE_UNIT_ACTION, 0, 1, path, 0.1f, 0.1f));

	path.clear();
	path.push_back(glm::ivec2(288, 160));
	intro->actions.push_back(new AddSceneUnit(NEW_SCENE_UNIT_ACTION, 0, 1, path, 0.1f, 0.1f));

	path.clear();
	path.push_back(glm::ivec2(128, 80));
	intro->actions.push_back(new AddSceneUnit(NEW_SCENE_UNIT_ACTION, 0, 1, path, 0.1f, 0.1f));

	path.clear();
	path.push_back(glm::ivec2(128, 224));
	intro->actions.push_back(new AddSceneUnit(NEW_SCENE_UNIT_ACTION, 0, 1, path, 0.1f, 0.1f));

	path.clear();
	path.push_back(glm::ivec2(176, 192));
	intro->actions.push_back(new AddSceneUnit(NEW_SCENE_UNIT_ACTION, 1, 1, path, 0.1f, 0.1f));

	path.clear();
	path.push_back(glm::ivec2(192, 96));
	intro->actions.push_back(new AddSceneUnit(NEW_SCENE_UNIT_ACTION, 0, 1, path, 0.1f, 0.1f));

	path.clear();
	path.push_back(glm::ivec2(208, 256));
	intro->actions.push_back(new AddSceneUnit(NEW_SCENE_UNIT_ACTION, 0, 1, path, 2.0f));

	//Player units spawn
	intro->actions.push_back(new CameraMove(CAMERA_ACTION, glm::vec2(128, 224)));
	path.clear();
	path.push_back(glm::ivec2(0, 320));
	path.push_back(glm::ivec2(0, 304));
	path.push_back(glm::ivec2(48, 304));
	intro->actions.push_back(new AddSceneUnit(NEW_SCENE_UNIT_ACTION, 3, 0, path, 0, 0.1f));
	path.clear();
	path.push_back(glm::ivec2(0, 320));
	path.push_back(glm::ivec2(0, 304));
	path.push_back(glm::ivec2(32, 304));
	intro->actions.push_back(new AddSceneUnit(NEW_SCENE_UNIT_ACTION, 4, 0, path, 0, 0.1f));
	path.clear();
	path.push_back(glm::ivec2(0, 320));
	path.push_back(glm::ivec2(0, 304));
	path.push_back(glm::ivec2(48, 304));
	path.push_back(glm::ivec2(48, 288));
	intro->actions.push_back(new AddSceneUnit(NEW_SCENE_UNIT_ACTION, 5, 0, path, 0, 0.1f));
	path.clear();
	path.push_back(glm::ivec2(0, 320));
	path.push_back(glm::ivec2(0, 304));
	path.push_back(glm::ivec2(32, 304));
	path.push_back(glm::ivec2(32, 288));
	path.push_back(glm::ivec2(64, 288));
	intro->actions.push_back(new AddSceneUnit(NEW_SCENE_UNIT_ACTION, 6, 0, path, 0, 0.1f));
	path.clear();
	path.push_back(glm::ivec2(0, 320));
	path.push_back(glm::ivec2(0, 288));
	path.push_back(glm::ivec2(32, 288));
	intro->actions.push_back(new AddSceneUnit(NEW_SCENE_UNIT_ACTION, 6, 0, path));

	intro->actions.push_back(new DialogueAction(DIALOGUE_ACTION, 14));

	//Halvan's shit
	path.clear();
	path.push_back(glm::ivec2(32, 288));
	path.push_back(glm::ivec2(32, 272));
	path.push_back(glm::ivec2(64, 272));
	intro->actions.push_back(new SceneUnitMove(SCENE_UNIT_MOVE_ACTION, 22, path, 0.2f));

	path.clear();
	path.push_back(glm::ivec2(64, 272));
	path.push_back(glm::ivec2(64, 240));
	path.push_back(glm::ivec2(80, 240));
	intro->actions.push_back(new SceneUnitMove(SCENE_UNIT_MOVE_ACTION, 22, path, 0.2f));

	path.clear();
	path.push_back(glm::ivec2(80, 240));
	path.push_back(glm::ivec2(80, 224));
	intro->actions.push_back(new SceneUnitMove(SCENE_UNIT_MOVE_ACTION, 22, path, 0, 0.5f, 2));

	path.clear();
	path.push_back(glm::ivec2(80, 224));
	path.push_back(glm::ivec2(80, 240));
	intro->actions.push_back(new SceneUnitMove(SCENE_UNIT_MOVE_ACTION, 22, path, 0.2f));

	path.clear();
	path.push_back(glm::ivec2(80, 240));
	path.push_back(glm::ivec2(64, 240));
	path.push_back(glm::ivec2(64, 272));
	path.push_back(glm::ivec2(32, 272));
	path.push_back(glm::ivec2(32, 288));
	intro->actions.push_back(new SceneUnitMove(SCENE_UNIT_MOVE_ACTION, 22, path, 0, 2));

	intro->actions.push_back(new DialogueAction(DIALOGUE_ACTION, 15));

	intro->init();*/

	//sceneManager.scenes[1]->extraSetup(&roundSubject);

	map.close();

	levelWidth = xTiles * TileManager::TILE_SIZE;
	levelHeight = yTiles * TileManager::TILE_SIZE;
	camera = Camera(256, 224, levelWidth, levelHeight);

	camera.setPosition(glm::vec2(0, 0));
	//	camera.setScale(2.0f);
	camera.update();

}

void Draw()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	bool fullScreenMenu = false;
	bool drawingMenu = false;

	if (gameOverMode.canDraw)
	{
		gameOverMode.DrawBG(Renderer, camera);
	}
	else
	{
		if (MenuManager::menuManager.menus.size() > 0)
		{
			drawingMenu = true;
			auto menu = MenuManager::menuManager.menus.back();
			fullScreenMenu = menu->fullScreen;
		}
		if (!fullScreenMenu)
		{
			ResourceManager::GetShader("instance").Use();
			ResourceManager::GetShader("instance").SetMatrix4("projection", camera.getCameraMatrix());
			TileManager::tileManager.showTiles(Renderer, camera);
			//for intro
		//	if (sceneManager.PlayingScene() && sceneManager.scenes[sceneManager.currentScene]->activation->type == 3)
			{

			}
		//	else
			{
				DrawUnitRanges();

				if (displays.state == PLAYER_DEATH || displays.state == PLAYER_DIED)
				{
					displays.DrawFade(&camera, shapeVAO);
				}
				ResourceManager::GetShader("sprite").Use().SetMatrix4("projection", camera.getCameraMatrix());
				Batch.begin();
				std::vector<Sprite> carrySprites;
				playerManager.Draw(&Batch, carrySprites);
				enemyManager.Draw(&Batch, carrySprites);
				Batch.end();
				Batch.renderBatch();
				ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera.getCameraMatrix());

				if (showCarry)
				{
					for (int i = 0; i < carrySprites.size(); i++)
					{
						Texture2D texture = ResourceManager::GetTexture("carryingIcons");
						auto uvs = texture.GetUVs(8, 8);
						Renderer->setUVs(uvs[carrySprites[i].currentFrame]);
						Renderer->DrawSprite(texture, carrySprites[i].getPosition(), 0, carrySprites[i].getSize());
					}
				}
				if (displays.state == NONE)
				{
					if (currentTurn == 0)
					{
						Renderer->setUVs(cursor.uvs[1]);
						Texture2D displayTexture = ResourceManager::GetTexture("cursor");
						Renderer->DrawSprite(displayTexture, cursor.position, 0.0f, cursor.dimensions);
					}
				}
			}
		}
		if (drawingMenu)
		{
			auto menu = MenuManager::menuManager.menus.back();
			menu->Draw();
		}
		else if (sceneManager.PlayingScene())
		{
			ResourceManager::GetShader("sprite").Use().SetMatrix4("projection", camera.getCameraMatrix());

			auto introUnits = sceneManager.scenes[sceneManager.currentScene]->introUnits;
			SBatch testBatch;
			testBatch.init();
			testBatch.begin();
			for (int i = 0; i < introUnits.size(); i++)
			{
				if (introUnits[i]->draw)
				{
					Texture2D texture = ResourceManager::GetTexture("sprites");
					glm::vec3 color = introUnits[i]->sprite.color;
					glm::vec4 colorAndAlpha = glm::vec4(color.x, color.y, color.z, introUnits[i]->sprite.alpha);

					glm::vec2 position = introUnits[i]->sprite.getPosition();

					glm::vec2 size;
					if (introUnits[i]->sprite.moveAnimate)
					{
						size = glm::vec2(32, 32);
						position += glm::vec2(-8, -8);
						texture = ResourceManager::GetTexture("movesprites");
					}
					else
					{
						size = introUnits[i]->sprite.getSize();
						position += introUnits[i]->sprite.drawOffset;

					}
					testBatch.addToBatch(texture.ID, position, size, colorAndAlpha, 1.0f - introUnits[i]->sprite.alpha, false, introUnits[i]->team, introUnits[i]->sprite.getUV());
				}
			}
			testBatch.end();
			testBatch.renderBatch();
			if (sceneManager.scenes[sceneManager.currentScene]->textManager.active)
			{
				sceneManager.scenes[sceneManager.currentScene]->textManager.Draw(Text, Renderer, &camera);
			}
			else if (displays.state != NONE)
			{
				displays.Draw(&camera, Text, shapeVAO, Renderer);
			}
		}
		else if (!fullScreenMenu && !minimap.show)
		{
			DrawText();
		}
		minimap.Draw(playerManager.playerUnits, enemyManager.enemies, camera, shapeVAO, Renderer);
		ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera.getOrthoMatrix());
		ResourceManager::GetShader("shape").SetFloat("alpha", gameOverMode.fadeOutAlpha);
		glm::mat4 model = glm::mat4();
		model = glm::translate(model, glm::vec3(0, 0, 0.0f));
		model = glm::scale(model, glm::vec3(256, 224, 0.0f));

		ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.0f, 0.0f));

		ResourceManager::GetShader("shape").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
	}
	SDL_GL_SwapWindow(window);
}

void DrawUnitRanges()
{
	for (int i = 0; i < cursor.foundTiles.size(); i++)
	{
		ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera.getCameraMatrix());
		ResourceManager::GetShader("shape").SetFloat("alpha", 0.35f);
		glm::mat4 model = glm::mat4();
		model = glm::translate(model, glm::vec3(cursor.foundTiles[i], 0.0f));

		model = glm::scale(model, glm::vec3(16, 16, 0.0f));
		float cost = float(cursor.costTile[i]) / 6.0f;
		ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0, 0.5f, 1.0f));

		ResourceManager::GetShader("shape").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
	}

	for (int i = 0; i < cursor.attackTiles.size(); i++)
	{
		ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera.getCameraMatrix());
		ResourceManager::GetShader("shape").SetFloat("alpha", 0.35f);
		glm::mat4 model = glm::mat4();
		model = glm::translate(model, glm::vec3(cursor.attackTiles[i], 0.0f));

		model = glm::scale(model, glm::vec3(16, 16, 0.0f));

		ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(1.0f, 0.5f, 0.0f));

		ResourceManager::GetShader("shape").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
	}

	for (int i = 0; i < cursor.drawnPath.size(); i++)
	{
		ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera.getCameraMatrix());
		ResourceManager::GetShader("shape").SetFloat("alpha", 0.5f);
		glm::mat4 model = glm::mat4();
		model = glm::translate(model, glm::vec3(cursor.drawnPath[i], 0.0f));

		model = glm::scale(model, glm::vec3(16, 16, 0.0f));

		ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.5f, 1.0f));

		ResourceManager::GetShader("shape").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
	}
}

void DrawText()
{
	if (displays.state != NONE)
	{
		displays.Draw(&camera, Text, shapeVAO, Renderer);
	}
	else if (battleManager.battleActive)
	{
		battleManager.Draw(Text, camera, Renderer, &cursor);
	}
	else if (!cursor.fastCursor && cursor.selectedUnit == nullptr && MenuManager::menuManager.menus.size() == 0)
	{
		glm::vec2 fixedPosition = camera.worldToScreen(cursor.position);
		if (Settings::settings.showTerrain)
		{
			auto tile = TileManager::tileManager.getTile(cursor.position.x, cursor.position.y)->properties;

			//Going to need to look into a better way of handling UI placement at some point
			int xStart = SCREEN_WIDTH;
			if (fixedPosition.x >= camera.screenWidth * 0.5f)
			{
				xStart = 178;
			}
			Text->RenderText(tile.name, xStart - 110, 20, 1);
			Text->RenderText("DEF", xStart - 120, 50, 0.7f, glm::vec3(0.69f, 0.62f, 0.49f));
			Text->RenderText(intToString(tile.defense), xStart - 95, 50, 0.7f);
			Text->RenderText("AVO", xStart - 85, 50, 0.7f, glm::vec3(0.69f, 0.62f, 0.49f));
			Text->RenderText(intToString(tile.avoid) + "%", xStart - 60, 50, 0.7f);
		}
		if (auto unit = cursor.focusedUnit)
		{
			int yOffset = 24;
			if (fixedPosition.y < 80) //Just hard setting a distance of 5 tiles from the top. Find a less silly way of doing this
			{
				yOffset = -24;
			}
			glm::vec2 drawPosition = glm::vec2(unit->sprite.getPosition()) - glm::vec2(8.0f, yOffset);
			drawPosition = camera.worldToRealScreen(drawPosition, SCREEN_WIDTH, SCREEN_HEIGHT);
			Text->RenderText(unit->name, drawPosition.x, drawPosition.y, 1, glm::vec3(0.0f));
			drawPosition.y += 22.0f;
			Text->RenderText("HP", drawPosition.x, drawPosition.y, 1, glm::vec3(0.1f, 0.11f, 0.22f));
			drawPosition.x += 25;
			Text->RenderText(intToString(unit->currentHP) + "/" + intToString(unit->maxHP), drawPosition.x, drawPosition.y, 1, glm::vec3(0.0f));
		}
	}
}

void resizeWindow(int width, int height)
{
	if (width < 256)
	{
		width = 256;
	}
	if (height < 224)
	{
		height = 224;
	}
	SDL_SetWindowSize(window, width, height);
	float ratio = 8.0f / 7.0f;
	int aspectWidth = width;
	int aspectHeight = round(float(aspectWidth) / ratio);
	if (aspectHeight > height)
	{
		aspectHeight = height;
		aspectWidth = round(float(aspectHeight) * ratio);
	}
	int vpx = round(float(width) / 2.0f - float(aspectWidth) / 2.0f);
	int vpy = round(float(height) / 2.0f - float(aspectHeight) / 2.0f);
	glViewport(vpx, vpy, aspectWidth, aspectHeight);
}
