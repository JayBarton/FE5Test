
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

SBatch Batch;

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

float testFrame = 0;

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
		if (deadUnit->team == 0)
		{
			auto it = std::find(playerManager.playerUnits.begin(), playerManager.playerUnits.end(), deadUnit);
			sceneUnits.erase(deadUnit->sceneID);
			playerManager.playerUnits.erase(it);
			delete deadUnit;
		}
	}
};

struct PostBattleEvents : public Observer<int>
{
	virtual void onNotify(int ID)
	{
		if (ID == 0)
		{
			battleManager.EndBattle(&cursor, &enemyManager, camera);
			//Real brute force here
			battleManager.defender->moveAnimate = false;
			battleManager.defender->sprite.currentFrame = idleFrame;
			
		}
		//player used an item
		else if (ID == 1)
		{
			if (cursor.selectedUnit->isMounted() && cursor.selectedUnit->mount->remainingMoves > 0)
			{
				//If the player attacked we need to return control to the cursor
				cursor.GetRemainingMove();
			}
			else
			{
				cursor.Wait();
			}
		}
		//Enemy used an item
		//Even if they can canto, if an enemy uses an item I want them to end their move
		else if (ID == 2)
		{
			enemyManager.FinishMove();
		}
		else if (ID == 3)
		{
			turnUnit++;
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
	glm::vec4 color(1.0f);
	ResourceManager::GetShader("instance").Use().SetVector4f("spriteColor", color);

//	ResourceManager::LoadShader("Shaders/spriteVertexShader.txt", "Shaders/sliceFragmentShader.txt", nullptr, "slice");

//	ResourceManager::LoadShader("Shaders/postVertexShader.txt", "Shaders/postFragmentShader.txt", nullptr, "postprocessing");

	ResourceManager::LoadTexture("E:/Damon/dev stuff/FE5Test/TestSprites/tilesheet2.png", "tiles");
	ResourceManager::LoadTexture("E:/Damon/dev stuff/FE5Test/TestSprites/cursor.png", "cursor");
	ResourceManager::LoadTexture2("E:/Damon/dev stuff/FE5Test/TestSprites/sprites.png", "sprites");
	ResourceManager::LoadTexture2("E:/Damon/dev stuff/FE5Test/TestSprites/movesprites.png", "movesprites");
	ResourceManager::LoadTexture("E:/Damon/dev stuff/FE5Test/TestSprites/palette.png", "palette");

	Shader myShader;
	myShader = ResourceManager::GetShader("Nsprite");
	Renderer = new SpriteRenderer(myShader);

	TileManager::tileManager.uvs = ResourceManager::GetTexture("tiles").GetUVs(TILE_SIZE, TILE_SIZE);

	cursor.uvs = ResourceManager::GetTexture("cursor").GetUVs(TILE_SIZE, TILE_SIZE);
	cursor.dimensions = glm::vec2(TileManager::TILE_SIZE);

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

	MenuManager::menuManager.SetUp(&cursor, Text, &camera, shapeVAO, Renderer, &battleManager, &playerManager);
	MenuManager::menuManager.subject.addObserver(turnEvents);
	enemyManager.subject.addObserver(turnEvents);
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
		float animationDelay = 0.0f;
		animationDelay = 0.27f;
		testFrame += deltaTime;
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
		if (inputManager.isKeyPressed(SDLK_BACKSPACE))
		{
			//Doing this dumb thing for a minute
			sceneManager.scenes[sceneManager.currentScene]->init();
		}
		if(sceneManager.PlayingScene())
		{
			sceneManager.scenes[sceneManager.currentScene]->Update(deltaTime, &playerManager, sceneUnits, camera, inputManager, cursor);
		}
		else if (MenuManager::menuManager.menus.size() == 0)
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
					battleManager.Update(deltaTime, &gen, &distribution);
					if (camera.moving)
					{
						camera.MoveTo(deltaTime, 5.0f);
					}
				}
				else
				{
					if (turnTransition)
					{
						if (currentTurn == 0)
						{
							if (turnUnit >= playerManager.playerUnits.size())
							{
								turnUnit = 0;
								if (Settings::settings.autoCursor)
								{
									cursor.position = playerManager.playerUnits[0]->sprite.getPosition();
									cursor.focusedUnit = playerManager.playerUnits[0];
								}
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
		else
		{
			MenuManager::menuManager.menus.back()->CheckInput(inputManager, deltaTime);
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
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glEnable(GL_MULTISAMPLE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void loadMap(std::string nextMap, UnitEvents* unitEvents)
{
	std::ifstream map(levelDirectory + nextMap);

	int xTiles = 0;
	int yTiles = 0;
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
			enemyManager.SetUp(map, &gen, &distribution, &playerManager.playerUnits);
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
	}

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

		DrawUnitRanges();

		ResourceManager::GetShader("sprite").Use().SetMatrix4("projection", camera.getCameraMatrix());
		Batch.begin();
		playerManager.Draw(&Batch);
		enemyManager.Draw(&Batch);
		Batch.end();
		Batch.renderBatch();

		ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera.getCameraMatrix());
		if (currentTurn == 0)
		{
			Renderer->setUVs(cursor.uvs[1]);
			Texture2D displayTexture = ResourceManager::GetTexture("cursor");
			Renderer->DrawSprite(displayTexture, cursor.position, 0.0f, cursor.dimensions);
		}
	}
	if (sceneManager.scenes.size() > 0 && sceneManager.scenes[sceneManager.currentScene]->textManager.active)
	{
		sceneManager.scenes[sceneManager.currentScene]->textManager.Draw(Text);
	}
	else
	{
		if (drawingMenu)
		{
			auto menu = MenuManager::menuManager.menus.back();
			menu->Draw();
		}
		if (!fullScreenMenu)
		{
			DrawText();
		}
	}

	ResourceManager::GetShader("shapeInstance").Use().SetMatrix4("projection", camera.getOrthoMatrix());
	ResourceManager::GetShader("shapeInstance").SetFloat("alpha", 1.0f);
	int x = 256 * 0.5f - (TileManager::tileManager.rowTiles * 2);
	int y = 224 * 0.5f - (TileManager::tileManager.columnTiles * 2);
	int startY = y;
	int startX = x;
	ShapeBatch shapeBatch;
	shapeBatch.init();
	shapeBatch.begin();
	//Going to want to batch this, since this is quite a few draw calls
	for (int i = 0; i < TileManager::tileManager.totalTiles; i++)
	{
		shapeBatch.addToBatch(glm::vec2(x, y), 4, 4, TileManager::tileManager.tiles[i].properties.miniMapColor);
		x += 4;
		if (x >= levelWidth/4 + startX)
		{
			//Move back
			x = startX;

			//Move to the next row
			y += 4;
		}
	}

	for (int i = 0; i < enemyManager.enemies.size(); i++)
	{
		auto position = enemyManager.enemies[i]->sprite.getPosition();
		position /= 16;
		position *= 4;
		position += glm::vec2(startX, startY);
		shapeBatch.addToBatch(glm::vec2(position.x + 1, position.y), 2, 1, glm::vec3(1, 0, 0));
		shapeBatch.addToBatch(glm::vec2(position.x, position.y + 1), 4, 2, glm::vec3(1, 0, 0));
		shapeBatch.addToBatch(glm::vec2(position.x + 1, position.y + 3), 2, 1, glm::vec3(1, 0, 0));
	}

	for (int i = 0; i < playerManager.playerUnits.size(); i++)
	{
		auto position = playerManager.playerUnits[i]->sprite.getPosition();
		position /= 16;
		position *= 4;
		position += glm::vec2(startX, startY);
		shapeBatch.addToBatch(glm::vec2(position.x + 1, position.y), 2, 1, glm::vec3(0, 0, 1));
		shapeBatch.addToBatch(glm::vec2(position.x, position.y + 1), 4, 2, glm::vec3(0, 0, 1));
		shapeBatch.addToBatch(glm::vec2(position.x + 1, position.y + 3), 2, 1, glm::vec3(0, 0, 1));
	}

	shapeBatch.end();
	shapeBatch.renderBatch();

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
		displays.Draw(&camera, Text, shapeVAO);
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
