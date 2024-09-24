
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
#include "Minimap.h"

#include "Vendor.h"

#include "UnitResources.h"

#include "TitleScreen.h"

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
void IdleAnimation(GLfloat deltaTime);
void StartTurnChecks();
void PlayerUpdate(GLfloat deltaTime);
void EnemyUpdate(GLfloat deltaTime);
void LoadEverythingElse(std::vector<IObserver*>& observers);
void SetShaderDefaults();
void ClearMap();
void CarryIconAnimation();
void Draw();
void DrawUnits();
void DrawIntroUnits();
void DrawText();
void resizeWindow(int width, int height);

void SuspendGame();

void PlayerTurnMusic();

const static int TILE_SIZE = 16;

bool endingGame = false;
bool turnTransition = false;
bool showCarry = false;
bool fadingIn = false;
bool skippingScene = false;
bool fadeInDelay = false;
//I am not reloading assets if the game returns to the title
bool loaded = false;
bool returningToMenu = false;
bool menuDelay = false;
bool endDelay = false;

float fadeDelayTimer = 0.0f;

SDL_Window *window;
SpriteRenderer* Renderer;

GLuint shapeVAO;

TextRenderer* Text;

Camera camera;

Texture2D Texture;

std::string levelDirectory = "Levels/";
std::string currentLevel;
int levelWidth;
int levelHeight;

Cursor cursor;
PlayerManager playerManager;

SceneManager sceneManager;

InputManager inputManager;

BattleManager battleManager;

EnemyManager enemyManager;

InfoDisplays displays;

TextObjectManager textManager;

std::unordered_map<int, Unit*> sceneUnits;
std::vector<VisitObject> visitObjects;
std::vector<Vendor> vendors;

glm::vec4 terrainStatusUVs;
std::vector<glm::vec4> nameBoxUVs;
glm::vec4 mapTitleUV;

glm::ivec2 seizePoint;
int endingID = -1;

//0 = standard
//1 = winning
//2 = losing
int mapMusic;

//unitID, dialogueID
std::unordered_map<int, int> gameOverDialogues;

float fadeAlpha;

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
	float fadeInAlpha = 255.0f;
	float textAlpha = 255.0f;
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
				Mix_HookMusicFinished(nullptr);
				Mix_FadeOutMusic(2000.0f);
			}
			break;
		case FADE_OUT_OF_GAME:
			fadeOutAlpha += fadeTime * deltaTime;
			if (fadeOutAlpha >= 1.0f)
			{
				fadeOutAlpha = 0.0f;
				state = FADE_IN_BG;
				canDraw = true;
				ResourceManager::PlayMusic("GameOver");
			}
			break;
		case FADE_IN_BG:
			fadeInAlpha -= 100 * deltaTime;
			if (fadeInAlpha <= 0)
			{
				fadeInAlpha = 0;
				state = FADE_IN_TEXT;
			}
			break;
		case FADE_IN_TEXT:
			textAlpha -= 100 * deltaTime;
			if (textAlpha <= 0)
			{
				textAlpha = 0;
				canExit = true;
			}
			break;
		}
	}

	void DrawBG(SpriteRenderer* renderer, Camera& camera)
	{
		ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera.getOrthoMatrix());
		ResourceManager::GetShader("Nsprite").SetFloat("subtractValue", fadeInAlpha);
		Renderer->setUVs();
		Texture2D displayTexture = ResourceManager::GetTexture("GameOver1");
		Renderer->DrawSprite(displayTexture, glm::vec2(0, 0), 0.0f, glm::vec2(256, 224));
		ResourceManager::GetShader("Nsprite").SetFloat("subtractValue", textAlpha);
		displayTexture = ResourceManager::GetTexture("GameOver2");
		Renderer->DrawSprite(displayTexture, glm::vec2(91, 172), 0.0f, glm::vec2(66, 20));
	}
};
GameOverMode gameOverMode;

TitleScreen* titleScreen = nullptr;

SBatch Batch;

Minimap minimap;

//Global
float unitSpeed = 2.5f;

int currentRound = 0;
int currentTurn = 0;

//Ugh. To handle healing units on turn transition
int turnUnit = 0;

int idleFrame = 0;
int idleAnimationDirection = 1;
float timeForFrame = 0.0f;
float carryBlinkTime = 0.0f;

float endingDelayTimer = 0.0f;

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
		Mix_HookMusicFinished(nullptr);
		Mix_FadeOutMusic(500.0f);
		if (ID == 0)
		{
			cursor.focusedUnit = nullptr;
			for (int i = 0; i < playerManager.units.size(); i++)
			{
				playerManager.units[i]->EndTurn();
			}
			//Whatever enemy manager set up here
			//Probably going to want to figure out some sort of priority for the order in which enemies act
			turnTransition = true;
			enemyManager.currentEnemy = 0;
			currentTurn = 1;
		}
		else if (ID == 1)
		{
			currentTurn = 0;
			//Start turn set up here
			//I'm just looping through right now, will need some different stuff set up to get heal animations playing properly
			turnTransition = true;
			turnUnit = 0;
			currentRound++;
			roundSubject.notify(currentRound);
			if (Settings::settings.autoCursor)
			{
				cursor.SetFocus(playerManager.units[0]);
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
			displays.AddExperience(attacker, defender, battleManager.rightPosition);
		}
		else
		{
			displays.AddExperience(defender, attacker, battleManager.rightPosition);
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
			auto it = std::find(playerManager.units.begin(), playerManager.units.end(), deadUnit);
			sceneUnits.erase(deadUnit->sceneID);
			playerManager.units.erase(it);
			delete deadUnit;
		}
		else if (deadUnit->team == 1)
		{
			auto it = std::find(enemyManager.units.begin(), enemyManager.units.end(), deadUnit);
			sceneUnits.erase(deadUnit->sceneID);
			enemyManager.units.erase(it);
			if (currentTurn == 1)
			{
				enemyManager.currentEnemy--;
			}
			else
			{
				if (mapMusic != 1)
				{
					if (enemyManager.units.size() < playerManager.units.size())
					{
						ResourceManager::pausedMusic = false;
						mapMusic = 1;
						ResourceManager::PlayMusic("WinningStart", "WinningLoop");
					}
				}
			}
			delete deadUnit;
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
			//We're going to have some sort of check here that will set a battle manager delay state to handle fading out the music before calling this
			//again and finally exiting.

			if (battleManager.battleScene)
			{
				battleManager.fadeOutBattle = true;
				Mix_HookMusicFinished(nullptr);
				Mix_FadeOutMusic(1000.0f);
			}
			//If we are here it means there was a talk during a map battle
			else if (battleManager.talkingUnit && battleManager.attacker->team == 0)
			{
				Mix_HookMusicFinished(nullptr);
				Mix_FadeOutMusic(500.0f);
				battleManager.delayFromTalk = true;
				battleManager.drawInfo = false;
			}
			else
			{
				battleManager.EndBattle(&cursor, &enemyManager, camera);
				//Real brute force here
				battleManager.defender->sprite.moveAnimate = false;
				battleManager.defender->sprite.currentFrame = idleFrame;
				textManager.textObjects[3].showAnyway = false;
			}
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

struct EndingEvents : public Observer<>
{
	virtual void onNotify()
	{
		endingDelayTimer = 0.0f;
		endingGame = true;
		endDelay = true;
		Mix_HookMusicFinished(nullptr);
		Mix_FadeOutMusic(500.0f);
		textManager.EndingScene();
		sceneManager.scenes[endingID]->activation->CheckActivation();
	}
};

struct SuspendEvent : public Observer<int>
{
	virtual void onNotify(int option)
	{
		if (option == 0)
		{
			SuspendGame();
		}
		else
		{
			returningToMenu = true;
		}
	}
};

struct ChangeMusicEvent : public Observer<int>
{
	virtual void onNotify(int ID)
	{
		if (ID == 0)
		{
			if (currentTurn == 0)
			{
				PlayerTurnMusic();
			}
			else
			{
				ResourceManager::PlayMusic("EnemyTurnStart", "EnemyTurnLoop");
			}
		}
		else
		{

		}
	}
};
std::vector<IObserver*> observers;
void loadMap(std::string nextMap);
void loadSuspendedGame();


void SetFadeIn(bool delay = false);

struct StartGameEvent : public Observer<int>
{
	virtual void onNotify(int ID)
	{
		Mix_VolumeMusic(128);
		textManager = TextObjectManager();
		textManager.setUVs();
		gameOverMode = GameOverMode();
		SetFadeIn();
		fadeAlpha = 1.0f;
		currentTurn = 0;
		if (!loaded)
		{
			LoadEverythingElse(observers);
		}
		if (ID == 0)
		{
			//start new game
			loadMap("2.map");
			currentRound = 0;
			cursor.SetFocus(playerManager.units[0]);
			Settings::settings.backgroundColors = Settings::settings.defaultColors;
			Settings::settings.editedColor.resize(Settings::settings.backgroundColors.size());
		}
		else
		{
			loadSuspendedGame();
			//Going to be loading this from suspend
			camera.setPosition(cursor.position);
			if (auto tile = TileManager::tileManager.getTile(cursor.position.x, cursor.position.y))
			{
				cursor.focusedUnit = tile->occupiedBy;
			}
		}
		camera.update();

		delete titleScreen;
		titleScreen = nullptr;
	}
};

bool queuedMusic;
float queuedMusicDelay;
struct QueueMusicEvent : public Observer<float>
{
	virtual void onNotify(float delay)
	{
		queuedMusic = true;
		queuedMusicDelay = delay;
	}
};


std::mt19937 gen;
std::uniform_int_distribution<int> distribution(0, 99);
int main(int argc, char** argv)
{
	//gen.seed(2);

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
	fpsLimiter.setMaxFPS(60.0f);

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
	ResourceManager::LoadShader("Shaders/rangeVertexShader.txt", "Shaders/rangeFragmentShader.txt", nullptr, "range");
	ResourceManager::LoadShader("Shaders/shapeSpecialVertexShader.txt", "Shaders/shapeSpecialFragmentShader.txt", nullptr, "shapeSpecial");
	ResourceManager::LoadShader("Shaders/shapeInstanceVertexShader.txt", "Shaders/shapeInstanceFragmentShader.txt", nullptr, "shapeInstance");
	ResourceManager::LoadShader("Shaders/sliceVertexShader.txt", "Shaders/sliceFragmentShader.txt", nullptr, "slice");
	ResourceManager::LoadShader("Shaders/normalSpriteVertexShader.txt", "Shaders/sliceFullFragmentShader.txt", nullptr, "sliceFull");
	ResourceManager::LoadShader("Shaders/clipVertexShader.txt", "Shaders/clipFragmentShader.txt", nullptr, "clip");
	ResourceManager::LoadShader("Shaders/normalSpriteVertexShader.txt", "Shaders/outlineFragmentShader.txt", nullptr, "outline");
	ResourceManager::LoadShader("Shaders/patternsVertexShader.txt", "Shaders/patternsFragmentShader.txt", nullptr, "patterns");
	ResourceManager::LoadShader("Shaders/gradientShapeVertexShader.txt", "Shaders/gradientShapeFragmentShader.txt", nullptr, "gradient");

	SetShaderDefaults();

	Text = new TextRenderer(800, 600);
	Text->Load("fonts/chary___.TTF", 30);

	Shader myShader;
	myShader = ResourceManager::GetShader("Nsprite");
	Renderer = new SpriteRenderer(myShader);

	StartGameEvent* startEvent = new StartGameEvent();

	titleScreen = new TitleScreen();
	titleScreen->subject.addObserver(startEvent);
	ResourceManager::LoadTexture("TestSprites/UIItems.png", "UIItems");
	ResourceManager::LoadTexture("TestSprites/icons.png", "icons");
	ResourceManager::LoadTexture("TestSprites/UIStuff.png", "UIStuff");
	ResourceManager::LoadTexture("TestSprites/testpattern.png", "testpattern");

	MenuManager::menuManager.SetUp(&cursor, Text, &camera, shapeVAO, Renderer, &battleManager, &playerManager, &enemyManager);
	titleScreen->init();

	camera = Camera(256, 224, 0, 0);

	while (isRunning)
	{
		GLfloat timeValue = SDL_GetTicks() / 1000.0f;
		// Calculate deltatime of current frame
		GLfloat currentFrame = timeValue;
		deltaTime = currentFrame - lastFrame;
		deltaTime = glm::clamp(deltaTime, 0.0f, 0.02f); //Clamped in order to prevent odd updates if there is a pause
		lastFrame = currentFrame;

		fpsLimiter.beginFrame();

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

		/*if (inputManager.isKeyPressed(SDLK_f))
		{
			SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

			resizeWindow(1920, 1080);
		}*/
		if (returningToMenu)
		{
			if (menuDelay)
			{
				endingDelayTimer += deltaTime;
				if (endingDelayTimer >= 1.0f)
				{
					endingDelayTimer = 1.0f;
					menuDelay = false;
					Mix_HookMusicFinished(nullptr);
					Mix_FadeOutMusic(500.0f);
				}
				fadeAlpha = glm::mix(0.0f, 1.0f, endingDelayTimer);
			}
			else
			{
				ClearMap();
				cursor.ClearTiles();
				titleScreen = new TitleScreen();
				titleScreen->subject.addObserver(startEvent);
				titleScreen->init();
				SetShaderDefaults();
				returningToMenu = false;
				fadeAlpha = 0.0f;
			}
		}
		else if (titleScreen)
		{
			titleScreen->Update(deltaTime, inputManager);
		}
		else
		{
			IdleAnimation(deltaTime);
			CarryIconAnimation();
			if (sceneManager.HideUnits())
			{
				if (inputManager.isKeyPressed(SDLK_SPACE))
				{
					skippingScene = true;
					fadeAlpha = 0.0f;
				}
			}
			if (skippingScene)
			{
				//15 frames/0.25s
				fadeAlpha += 0.0666f;
				if (fadeAlpha >= 1)
				{
					fadeAlpha = 1;
					skippingScene = false;
					SetFadeIn(true);
					sceneManager.ExitScene(cursor);
					camera.moving = false;
					camera.setPosition(cursor.position);
					textManager.active = false;
					ResourceManager::GetShader("instance").SetFloat("backgroundFade", 0, true);
				}
			}
			if (endingGame)
			{
				if (endDelay)
				{
					endingDelayTimer += deltaTime;
					if (endingDelayTimer >= 1.0f)
					{
						endingDelayTimer = 0.0f;
						ResourceManager::PlayMusic("Victory");
						endDelay = false;
					}
				}
				else if (textManager.active)
				{
					textManager.Update(deltaTime, inputManager, true);
					if (inputManager.isKeyPressed(SDLK_SPACE) || inputManager.isKeyPressed(SDLK_z))
					{
						textManager.active = false;
					}
				}
				else if (sceneManager.PlayingScene())
				{
					sceneManager.Update(deltaTime, &playerManager, sceneUnits, camera, inputManager, cursor, displays);
				}
				else
				{
					fadeAlpha = 0.0f;
					endingDelayTimer = 0.0f;
					returningToMenu = true;
					menuDelay = true;
					endingGame = false;
				}
			}
			else if (fadingIn)
			{
				if (fadeInDelay)
				{
					fadeDelayTimer += deltaTime;
					if (fadeDelayTimer >= 0.5f)
					{
						fadeDelayTimer = 0.0f;
						fadeInDelay = false;
					}
				}
				else
				{
					//15 frames/0.25s
					fadeAlpha -= 0.0666f;
					if (fadeAlpha <= 0)
					{
						fadeAlpha = 0;
						fadingIn = false;
					}
				}
			}
			else if (textManager.active)
			{
				textManager.Update(deltaTime, inputManager);
				if (inputManager.isKeyPressed(SDLK_SPACE) || inputManager.isKeyPressed(SDLK_z))
				{
					if (battleManager.battleActive && battleManager.battleScene)
					{
						textManager.BattleTextClose();
					}
					else
					{
						textManager.state = PORTRAIT_FADE_OUT;
						textManager.finishing = true;
					}
				}
				//Annoying dupe for now
				if (sceneManager.PlayingScene())
				{
					auto introUnits = sceneManager.scenes[sceneManager.currentScene]->introUnits;
					for (int i = 0; i < introUnits.size(); i++)
					{
						introUnits[i]->sprite.HandleAnimation(deltaTime, idleFrame);
					}
				}
			}
			else if (MenuManager::menuManager.menus.size() > 0)
			{
				MenuManager::menuManager.menus.back()->CheckInput(inputManager, deltaTime);
			}
			else if (sceneManager.PlayingScene())
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
				if (battleManager.battleActive && battleManager.battleScene && !battleManager.transitionIn)
				{
					if (!displays.displayingExperience) //This check is to help with an issue with displaying experience in cases where the unit does not level up
						//It does not help very much.
					{
						battleManager.Update(deltaTime, &gen, &distribution, inputManager);
					}
					displays.Update(deltaTime, inputManager);
				}
				else if (displays.state != NONE)
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
						if (camera.moving)
						{
							camera.MoveTo(deltaTime, 5.0f);
						}
						else
						{
							battleManager.Update(deltaTime, &gen, &distribution, inputManager);
						}
					}
					else
					{
						if (gameOverMode.active)
						{
							if (gameOverMode.canExit)
							{
								if (inputManager.isKeyPressed(SDLK_RETURN))
								{
									//Return to main menu
									returningToMenu = true;
									menuDelay = true;
									fadeAlpha = 0.0f;
									endingDelayTimer = 0.0f;
								}
							}
							else
							{
								gameOverMode.Update(deltaTime, inputManager);
								fadeAlpha = gameOverMode.fadeOutAlpha;
							}
						}
						else if (turnTransition)
						{
							StartTurnChecks();
						}
						//nesting getting a little deep here
						else if (currentTurn == 0)
						{
							PlayerUpdate(deltaTime);
						}
						else
						{
							EnemyUpdate(deltaTime);
						}
					}
				}
			}

			//Update unit animation
			playerManager.Update(deltaTime, idleFrame, inputManager);
			enemyManager.UpdateEnemies(deltaTime, idleFrame);
			camera.update();

			//ugh
			for (int i = 0; i < visitObjects.size(); i++)
			{
				if (visitObjects[i].toDelete)
				{
					TileManager::tileManager.getTile(visitObjects[i].position.x, visitObjects[i].position.y)->visitSpot = nullptr;
					TileManager::tileManager.getTile(visitObjects[i].position.x, visitObjects[i].position.y)->uvID = 184;
					TileManager::tileManager.reDraw();
					visitObjects[i].sceneMap.clear();
					visitObjects.erase(visitObjects.begin() + i);
					i--;
				}
			}
		}

		Draw();

		fps = fpsLimiter.end();
	}

	ClearMap();

	ResourceManager::Clear();

	delete Renderer;
	delete Text;
	delete titleScreen;

	for (int i = 0; i < observers.size(); i++)
	{
		delete observers[i];
		observers[i] = nullptr;
	}
	delete startEvent;

	SDL_DestroyWindow(window);
	window = nullptr;

	SDL_Quit();
	IMG_Quit();
	Mix_CloseAudio();
	Mix_Quit();

	return 0;
}

void ClearMap()
{
	enemyManager.Clear();
	playerManager.Clear();
	for (int i = 0; i < sceneManager.scenes.size(); i++)
	{
		sceneManager.scenes[i]->ClearActions();
		delete sceneManager.scenes[i];
	}
	cursor.focusedUnit = nullptr;
	sceneManager.scenes.clear();
	visitObjects.clear();
	vendors.clear();

	MenuManager::menuManager.ClearMenu();
	TileManager::tileManager.clearTiles();
}

void SetShaderDefaults()
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera.getCameraMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);

	ResourceManager::GetShader("range").Use().SetMatrix4("projection", camera.getCameraMatrix());
	ResourceManager::GetShader("range").SetFloat("alpha", 1.0f);

	ResourceManager::GetShader("shapeSpecial").Use().SetMatrix4("projection", camera.getCameraMatrix());
	ResourceManager::GetShader("shapeSpecial").SetFloat("alpha", 1.0f);

	ResourceManager::GetShader("shapeInstance").Use().SetMatrix4("projection", camera.getCameraMatrix());
	ResourceManager::GetShader("shapeInstance").SetFloat("alpha", 1.0f);

	ResourceManager::GetShader("gradient").Use().SetMatrix4("projection", camera.getCameraMatrix());
	ResourceManager::GetShader("gradient").SetFloat("alpha", 1.0f);

	ResourceManager::GetShader("sprite").Use().SetInteger("image", 0);
	ResourceManager::GetShader("sprite").SetInteger("palette", 1);
	ResourceManager::GetShader("sprite").SetInteger("BattleFadeIn", 2);
	ResourceManager::GetShader("sprite").SetVector2f("screenResolution", glm::vec2(286, 224));
	ResourceManager::GetShader("sprite").SetInteger("battleScreen", 0);
	ResourceManager::GetShader("sprite").SetMatrix4("projection", camera.getCameraMatrix());

	ResourceManager::GetShader("Nsprite").Use().SetInteger("image", 0);
	ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera.getCameraMatrix());
	ResourceManager::GetShader("Nsprite").SetFloat("subtractValue", 0);

	ResourceManager::GetShader("instance").Use().SetInteger("image", 0);
	ResourceManager::GetShader("instance").SetMatrix4("projection", camera.getCameraMatrix());
	ResourceManager::GetShader("instance").SetVector4f("spriteColor", glm::vec4(1));
	ResourceManager::GetShader("instance").SetFloat("backgroundFade", 0);

	ResourceManager::GetShader("slice").Use().SetInteger("image", 0);
	ResourceManager::GetShader("slice").Use().SetInteger("image2", 1);
	ResourceManager::GetShader("slice").SetMatrix4("projection", camera.getOrthoMatrix());

	ResourceManager::GetShader("sliceFull").Use().SetInteger("image", 0);
	ResourceManager::GetShader("sliceFull").SetMatrix4("projection", camera.getOrthoMatrix());

	ResourceManager::GetShader("clip").Use().SetInteger("image", 0);
	ResourceManager::GetShader("clip").SetMatrix4("projection", camera.getOrthoMatrix());

	ResourceManager::GetShader("outline").Use().SetInteger("image", 0);
	ResourceManager::GetShader("outline").Use().SetMatrix4("projection", camera.getOrthoMatrix());

	ResourceManager::GetShader("patterns").Use().SetInteger("image", 0);
	ResourceManager::GetShader("patterns").SetMatrix4("projection", camera.getOrthoMatrix());
	ResourceManager::GetShader("patterns").SetVector2f("sheetScale", glm::vec2(64, 32) / glm::vec2(128, 32));

}

void LoadEverythingElse(std::vector<IObserver*>& observers)
{
	ResourceManager::LoadTexture("TestSprites/Tiles.png", "tiles");
	ResourceManager::LoadTexture2("TestSprites/sprites.png", "sprites");
	ResourceManager::LoadTexture2("TestSprites/movesprites.png", "movesprites");
	ResourceManager::LoadTexture("TestSprites/palette.png", "palette");
	ResourceManager::LoadTexture("TestSprites/gameovermain.png", "GameOver1");
	ResourceManager::LoadTexture("TestSprites/gameovertext.png", "GameOver2");
	ResourceManager::LoadTexture("TestSprites/Portraits.png", "Portraits");
	ResourceManager::LoadTexture("TestSprites/EndingBackground.png", "EndingBG");
	ResourceManager::LoadTexture("TestSprites/BattleBackground.png", "BattleBG");
	ResourceManager::LoadTexture("TestSprites/BattleFadeIn.png", "BattleFadeIn");
	ResourceManager::LoadTexture("TestSprites/BattleLevelBackground.png", "BattleLevelBackground");
	ResourceManager::LoadTexture("TestSprites/BattleExperienceBackground.png", "BattleExperienceBackground");
	ResourceManager::LoadTexture("TestSprites/MapExperienceBackground.png", "MapExperienceBackground");
	ResourceManager::LoadTexture("TestSprites/OptionsScreenBackground.png", "OptionsScreenBackground");
	ResourceManager::LoadTexture("TestSprites/UnitViewBG.png", "UnitViewBG");
	ResourceManager::LoadTexture("TestSprites/TradeMenuBG.png", "TradeMenuBG");
	ResourceManager::LoadTexture("TestSprites/StatusMenuBG.png", "StatusMenuBG");
	ResourceManager::LoadTexture("TestSprites/Backgrounds/page1lower.png", "page1lower");
	ResourceManager::LoadTexture("TestSprites/Backgrounds/page2lower.png", "page2lower");
	ResourceManager::LoadTexture("TestSprites/Backgrounds/unitViewUpper.png", "unitViewUpper");
	ResourceManager::LoadTexture("TestSprites/Backgrounds/TextBackground.png", "TextBackground");
	ResourceManager::LoadTexture("TestSprites/Backgrounds/TextBorder.png", "TextBorder");
	ResourceManager::LoadTexture("TestSprites/Backgrounds/VendorBackground.png", "VendorBackground");
	ResourceManager::LoadTexture("TestSprites/Backgrounds/BattleSceneBoxes.png", "BattleSceneBoxes");
	ResourceManager::LoadTexture("TestSprites/Backgrounds/EnemySelectBackground.png", "EnemySelectBackground");

	ResourceManager::LoadSound("Sounds/cursormove.wav", "cursorMove");
	ResourceManager::LoadSound("Sounds/heldCursorMove.wav", "heldCursorMove");
	ResourceManager::LoadSound("Sounds/select1.wav", "select1");
	ResourceManager::LoadSound("Sounds/select2.wav", "select2");
	ResourceManager::LoadSound("Sounds/movementFoot.wav", "footMove");
	ResourceManager::LoadSound("Sounds/movementHorse.wav", "horseMove");
	ResourceManager::LoadSound("Sounds/minimapClose.wav", "minimapClose");
	ResourceManager::LoadSound("Sounds/cancel.wav", "cancel");
	ResourceManager::LoadSound("Sounds/turnEnd.wav", "turnEnd");
	ResourceManager::LoadSound("Sounds/optionSelect2.wav", "optionSelect2");
	ResourceManager::LoadSound("Sounds/fadeout.wav", "fadeout");
	ResourceManager::LoadSound("Sounds/pagechange.wav", "pagechange");
	ResourceManager::LoadSound("Sounds/hit.wav", "hit");
	ResourceManager::LoadSound("Sounds/deathHit.wav", "deathHit");
	ResourceManager::LoadSound("Sounds/critHit.wav", "critHit");
	ResourceManager::LoadSound("Sounds/speech.wav", "speech");
	ResourceManager::LoadSound("Sounds/miss.wav", "miss");
	ResourceManager::LoadSound("Sounds/heal.wav", "heal");
	ResourceManager::LoadSound("Sounds/healthbar.wav", "healthbar");
	ResourceManager::LoadSound("Sounds/pointUp.wav", "pointUp");
	ResourceManager::LoadSound("Sounds/getItem.wav", "getItem");
	ResourceManager::LoadSound("Sounds/levelUp.wav", "levelUp");
	ResourceManager::LoadSound("Sounds/nodamage.wav", "nodamage");
	ResourceManager::LoadSound("Sounds/battleTransition.wav", "battleTransition");
	ResourceManager::LoadSound("Sounds/experience.wav", "experience");

	ResourceManager::LoadMusic("Sounds/Map1.ogg", "PlayerTurn");
	ResourceManager::LoadMusic("Sounds/Map2.1.ogg", "EnemyTurnStart");
	ResourceManager::LoadMusic("Sounds/Map2.2.ogg", "EnemyTurnLoop");
	ResourceManager::LoadMusic("Sounds/Map3.1.ogg", "HeroesEnterStart");
	ResourceManager::LoadMusic("Sounds/Map3.2.ogg", "HeroesEnterLoop");
	ResourceManager::LoadMusic("Sounds/Map4.1.ogg", "RaydrickStart");
	ResourceManager::LoadMusic("Sounds/Map4.2.ogg", "RaydrickLoop");
	ResourceManager::LoadMusic("Sounds/Map5.1.ogg", "WinningStart");
	ResourceManager::LoadMusic("Sounds/Map5.2.ogg", "WinningLoop");
	ResourceManager::LoadMusic("Sounds/Map6.1.ogg", "LosingStart");
	ResourceManager::LoadMusic("Sounds/Map6.2.ogg", "LosingLoop");
	ResourceManager::LoadMusic("Sounds/Map7.1.ogg", "TurnEndSceneStart");
	ResourceManager::LoadMusic("Sounds/Map7.2.ogg", "TurnEndSceneLoop");
	ResourceManager::LoadMusic("Sounds/PlayerAttackStart.ogg", "PlayerAttackStart");
	ResourceManager::LoadMusic("Sounds/PlayerAttackLoop.ogg", "PlayerAttackLoop");
	ResourceManager::LoadMusic("Sounds/EnemyAttack.ogg", "EnemyAttack");
	ResourceManager::LoadMusic("Sounds/BossStart.ogg", "BossStart");
	ResourceManager::LoadMusic("Sounds/BossLoop.ogg", "BossLoop");
	ResourceManager::LoadMusic("Sounds/GameOver.ogg", "GameOver");
	ResourceManager::LoadMusic("Sounds/Victory.ogg", "Victory");

	TileManager::tileManager.uvs = ResourceManager::GetTexture("tiles").GetUVs(TILE_SIZE, TILE_SIZE);
	auto UITexture = ResourceManager::GetTexture("UIItems");
	cursor.uvs = UITexture.GetUVs(208, 0, 21, 21, 2, 2, 3);
	minimap.cursorUvs = UITexture.GetUVs(64, 0, 70, 62, 2, 1, 2);
	terrainStatusUVs = UITexture.GetUVs(132, 64, 66, 34, 1, 1)[0];
	nameBoxUVs = UITexture.GetUVs(0, 64, 66, 32, 2, 1);

	mapTitleUV = UITexture.GetUVs(196, 132, 208, 48, 1, 1)[0];

	UnitResources::LoadUVs();
	UnitResources::LoadAnimData();

	textManager.setUVs();

	battleManager.GetUVs();

	ItemManager::itemManager.SetUpItems();

	displays.init(&textManager);

	UnitEvents* unitEvents = new UnitEvents();
	TurnEvents* turnEvents = new TurnEvents();
	BattleEvents* battleEvents = new BattleEvents();
	DeathEvent* deathEvents = new DeathEvent();
	PostBattleEvents* postBattleEvents = new PostBattleEvents();
	ItemEvents* itemEvents = new ItemEvents();
	EndingEvents* endingEvents = new EndingEvents();
	SuspendEvent* suspendEvents = new SuspendEvent();
	ChangeMusicEvent* changeMusicEvents = new ChangeMusicEvent();

	observers.push_back(unitEvents);
	observers.push_back(turnEvents);
	observers.push_back(battleEvents);
	observers.push_back(postBattleEvents);
	observers.push_back(itemEvents);
	observers.push_back(endingEvents);
	observers.push_back(suspendEvents);
	observers.push_back(changeMusicEvents);

	ItemManager::itemManager.subject.addObserver(itemEvents);
	battleManager.endAttackSubject.addObserver(battleEvents);
	battleManager.unitDiedSubject.addObserver(deathEvents);
	battleManager.resumeMusic.addObserver(changeMusicEvents);
	displays.endBattle.addObserver(postBattleEvents);
	displays.endTurn.addObserver(changeMusicEvents);
	playerManager.init(&gen, &distribution, unitEvents, &sceneUnits);
	enemyManager.init(&gen, &distribution);

	battleManager.displays = &displays;

	MenuManager::menuManager.subject.addObserver(turnEvents);
	MenuManager::menuManager.endingSubject.addObserver(endingEvents);
	MenuManager::menuManager.suspendSubject.addObserver(suspendEvents);
	MenuManager::menuManager.unitDiedSubject.addObserver(deathEvents);
	enemyManager.subject.addObserver(turnEvents);
	enemyManager.unitEscapedSubject.addObserver(deathEvents);
	enemyManager.displays = &displays;

	loaded = true;
}

void EnemyUpdate(GLfloat deltaTime)
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

void PlayerUpdate(GLfloat deltaTime)
{
	if (!minimap.show)
	{
		if (inputManager.isKeyPressed(SDLK_m))
		{
			minimap.Open();
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
		if (!minimap.held && inputManager.isKeyPressed(SDLK_m) || inputManager.isKeyPressed(SDLK_z))
		{
			minimap.Close();
			cursor.position = camera.getPosition();
		}
		else
		{
			minimap.Update(inputManager, deltaTime, camera);
		}
	}
}

void StartTurnChecks()
{
	if (currentTurn == 0)
	{
		if (turnUnit >= playerManager.units.size())
		{
			turnUnit = 0;
			camera.SetMove(cursor.position);
			turnTransition = false;
		}
		else
		{
			playerManager.units[turnUnit]->StartTurn(displays, &camera);

			if (displays.state == NONE)
			{
				turnUnit++;
			}
		}
	}
	else
	{
		if (turnUnit >= enemyManager.units.size())
		{
			turnUnit = 0;
			turnTransition = false;
		}
		else
		{
			enemyManager.units[turnUnit]->StartTurn(displays, &camera);

			if (displays.state == NONE)
			{
				turnUnit++;
			}
		}
	}
}

void IdleAnimation(GLfloat deltaTime)
{
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
}

void CarryIconAnimation()
{
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
}

void init()
{
	int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);
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
	glDisable(GL_MULTISAMPLE);	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void loadMap(std::string nextMap)
{
	std::ifstream map(levelDirectory + nextMap);
	currentLevel = nextMap;
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
			enemyManager.SetUp(map, &playerManager.units, &vendors);
			//proof of concept
			for (int i = 0; i < enemyManager.units.size(); i++)
			{
				auto unit = enemyManager.units[i];
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
				sceneManager.scenes[i] = new Scene(&textManager);
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
					else if (actionType == START_MUSIC)
					{
						int musicID;
						map >> musicID;
						currentObject->actions[c] = new StartMusic(actionType, musicID);
					}
					else if (actionType == STOP_MUSIC)
					{
						float delay;
						map >> delay;
						currentObject->actions[c] = new StopMusic(actionType, delay);
					}
					else if (actionType == SHOW_MAP_TITLE)
					{
						float delay;
						map >> delay;
						currentObject->actions[c] = new ShowTitle(actionType, delay);
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
					currentObject->introDelay = 1.0f; //Not entirely sure if I want to hard code the intro delay or load them from the level...
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
				else if (activationType == 4)
				{
					currentObject->activation = new EndingActivation(currentObject, activationType);
					endingID = i;
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
		else if (thing == "EnemyEscape")
		{
			int x = 0;
			int y = 0;
			map >> x >> y;
			enemyManager.escapePoint = glm::ivec2(x, y);
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
		else if (thing == "Seize")
		{
			int x = 0;
			int y = 0;
			map >> x >> y;
			seizePoint = glm::vec2(x, y);
			TileManager::tileManager.placeSeizePoint(seizePoint.x, seizePoint.y);
		}
	}
	if (intro >= 0)
	{
		sceneManager.scenes[intro]->init();
		displays.ChangeTurn(currentTurn);
	}
	else
	{
		mapMusic = 0;	
		ResourceManager::PlayMusic("PlayerTurn");
	}

	map.close();

	levelWidth = xTiles * TileManager::TILE_SIZE;
	levelHeight = yTiles * TileManager::TILE_SIZE;
	camera = Camera(256, 224, levelWidth, levelHeight);

	camera.setPosition(glm::vec2(0, 0));
	camera.update();
}

void Draw()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	bool fullScreenMenu = false;
	bool drawingMenu = false;

	if (titleScreen)
	{
		titleScreen->Draw(Renderer, Text, camera);
	}
	else if (gameOverMode.canDraw)
	{
		gameOverMode.DrawBG(Renderer, camera);
	}
	else if (textManager.showBG)
	{
//		if (textManager.ShowText())
		{
			textManager.Draw(Text, Renderer, &camera);
		}
	}
	else if (battleManager.battleActive && battleManager.battleScene && !battleManager.transitionIn)
	{
		battleManager.Draw(Text, camera, Renderer, &cursor, &Batch, shapeVAO, &textManager);
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
			cursor.DrawUnitRanges(shapeVAO, camera);
			//for intro
			if (!sceneManager.HideUnits())
			{
				DrawUnits();
			}
			if (sceneManager.PlayingScene())
			{
				DrawIntroUnits();
				if (sceneManager.scenes[sceneManager.currentScene]->state == SHOWING_TITLE)
				{
					sceneManager.DrawTitle(Renderer, Text, camera, shapeVAO, mapTitleUV);
				}
				//Need another drawfade here
				if (displays.state != NONE)
				{
					displays.Draw(&camera, Text, shapeVAO, Renderer);
				}
			}
			if (textManager.active)
			{
				textManager.Draw(Text, Renderer, &camera);
				textManager.DrawFade(&camera, shapeVAO);
			}
		}
		if (drawingMenu)
		{
			auto menu = MenuManager::menuManager.menus.back();
			menu->Draw();
		}
		else if (!fullScreenMenu && !minimap.show && !sceneManager.PlayingScene())
		{
			if (displays.state != NONE)
			{
				displays.Draw(&camera, Text, shapeVAO, Renderer);
			}
			else if (battleManager.battleActive)
			{
				battleManager.Draw(Text, camera, Renderer, &cursor, &Batch, shapeVAO, &textManager);
			}
			else
			{
				if (currentTurn == 0)
				{
					DrawText();
					if (displays.state == NONE)
					{
						if (!cursor.movingUnit)
						{
							ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera.getCameraMatrix());
							Renderer->setUVs(cursor.uvs[cursor.currentFrame]);
							Texture2D displayTexture = ResourceManager::GetTexture("UIItems");
							Renderer->DrawSprite(displayTexture, cursor.position - glm::vec2(2, 3), 0.0f, cursor.dimensions);
						}
					}
				}
			}
		}
		minimap.Draw(playerManager.units, enemyManager.units, camera, shapeVAO, Renderer);
	}
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera.getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", fadeAlpha);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(0, 0, 0.0f));
	model = glm::scale(model, glm::vec3(256, 224, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.0f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
	SDL_GL_SwapWindow(window);
}

void DrawUnits()
{
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
			Texture2D texture = ResourceManager::GetTexture("UIItems");
			auto uvs = MenuManager::menuManager.carryingIconsUVs;
			Renderer->setUVs(uvs[carrySprites[i].currentFrame]);
			Renderer->DrawSprite(texture, carrySprites[i].getPosition(), 0, carrySprites[i].getSize());
		}
	}
}

void DrawIntroUnits()
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
			testBatch.addToBatch(texture.ID, position, size, colorAndAlpha, 0, false, introUnits[i]->team, introUnits[i]->sprite.getUV());
		}
	}
	testBatch.end();
	testBatch.renderBatch();
}

void DrawText()
{
	if (!cursor.fastCursor && cursor.selectedUnit == nullptr && MenuManager::menuManager.menus.size() == 0)
	{
		glm::vec2 fixedPosition = camera.worldToScreen(cursor.position);
		if (Settings::settings.showTerrain)
		{
			//Going to need to look into a better way of handling UI placement at some point
			int xStart = 625;
			int xWindow = 188;
			if (fixedPosition.x >= camera.screenWidth * 0.5f)
			{
				xStart = 50;
				xWindow = 4;
			}

			int patternID = Settings::settings.backgroundPattern;
			auto inColor = Settings::settings.backgroundColors[patternID];
			glm::vec3 topColor = glm::vec3(inColor[0], inColor[1], inColor[2]);
			glm::vec3 bottomColor = glm::vec3(inColor[3], inColor[4], inColor[5]);
			glm::vec2 size(58, 26);
			Renderer->shader = ResourceManager::GetShader("patterns");
			ResourceManager::GetShader("patterns").Use();
			ResourceManager::GetShader("patterns").SetMatrix4("projection", camera.getOrthoMatrix());
			ResourceManager::GetShader("patterns").SetInteger("gray", false);
			ResourceManager::GetShader("patterns").SetVector3f("topColor", topColor / 255.0f);
			ResourceManager::GetShader("patterns").SetVector3f("bottomColor", bottomColor / 255.0f);
			
			ResourceManager::GetShader("patterns").SetInteger("index", patternID);

			ResourceManager::GetShader("patterns").SetVector2f("scale", size / glm::vec2(64, 32));
			//ResourceManager::GetShader("patterns").SetVector2f("sheetScale", glm::vec2(64, 32) / glm::vec2(128, 32));

			auto patternTexture = ResourceManager::GetTexture("testpattern");

			Renderer->setUVs(MenuManager::menuManager.patternUVs[patternID]);
			Renderer->DrawSprite(patternTexture, glm::vec2(xWindow + 3, 7), 0.0f, size);

			Renderer->shader = ResourceManager::GetShader("Nsprite");


			Texture2D terrainBox = ResourceManager::GetTexture("UIItems");
			ResourceManager::GetShader("Nsprite").Use();
			ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera.getOrthoMatrix());
			Renderer->setUVs(terrainStatusUVs);
			Renderer->DrawSprite(terrainBox, glm::vec2(xWindow, 4), 0, glm::vec2(66, 34));


			auto tile = TileManager::tileManager.getTile(cursor.position.x, cursor.position.y)->properties;

			Text->RenderText(tile.name, xStart, 32, 1);
			Text->RenderText("DEF", xStart - 12, 66, 0.7f, glm::vec3(0.69f, 0.62f, 0.49f));
			Text->RenderText(intToString(tile.defense), xStart + 25, 66, 0.7f);
			Text->RenderText("AVO", xStart + 59, 66, 0.7f, glm::vec3(0.69f, 0.62f, 0.49f));
			Text->RenderText(intToString(tile.avoid) + "%", xStart + 100, 66, 0.7f);
		}
		auto unit = cursor.focusedUnit;
		if (unit && cursor.settled)
		{
			Texture2D test = ResourceManager::GetTexture("UIItems");
			ResourceManager::GetShader("Nsprite").Use();
			ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera.getCameraMatrix());
			Renderer->setUVs(nameBoxUVs[unit->team]);
			int yOffset = 42;
			int textOffset = 6;
			bool below = false;
			if (fixedPosition.y < 80) //Just hard setting a distance of 5 tiles from the top. Find a less silly way of doing this
			{
				yOffset = -26;
				textOffset = 10;
				below = true;
			}

			glm::vec3 hpTextColor;
			if (unit->team == 0)
			{
				hpTextColor = glm::vec3(0.1f, 0.11f, 0.22f);
			}
			else
			{
				hpTextColor = glm::vec3(0.19f, 0.06f, 0.06f);
			}

			glm::vec2 drawPosition = glm::vec2(unit->sprite.getPosition()) - glm::vec2(21.0f, yOffset);
			Renderer->DrawSprite(test, drawPosition, 0, glm::vec2(66, 32), glm::vec4(1), false, below);
			glm::vec2 textPosition = camera.worldToRealScreen(drawPosition + glm::vec2(5, textOffset), SCREEN_WIDTH, SCREEN_HEIGHT);
			Text->RenderText(unit->name, textPosition.x, textPosition.y, 1, glm::vec3(0.0f));
			textPosition.y += 34;
			Text->RenderText("HP", textPosition.x, textPosition.y, 1, hpTextColor);
			textPosition.x += 50;
			Text->RenderText(intToString(unit->currentHP) + "/" + intToString(unit->maxHP), textPosition.x, textPosition.y, 1, glm::vec3(0.0f));
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

json UnitToJson(Unit* unit)
{
	//Look I'm going to move this all elsewhere and it will be real nice over there don't worry about it
	std::vector<std::string> weaponNames;
	weaponNames.resize(10);
	weaponNames[WeaponData::TYPE_SWORD] = "Sword";
	weaponNames[WeaponData::TYPE_AXE] = "Axe";
	weaponNames[WeaponData::TYPE_LANCE] = "Lance";
	weaponNames[WeaponData::TYPE_BOW] = "Bow";
	weaponNames[WeaponData::TYPE_STAFF] = "Staff";
	weaponNames[WeaponData::TYPE_FIRE] = "Fire";
	weaponNames[WeaponData::TYPE_THUNDER] = "Thunder";
	weaponNames[WeaponData::TYPE_WIND] = "Wind";
	weaponNames[WeaponData::TYPE_LIGHT] = "Light";
	weaponNames[WeaponData::TYPE_DARK] = "Dark";

	json j;
	json stats;
	j["ID"] = unit->ID;
	j["LevelID"] = unit->levelID;
	j["Name"] = unit->name;
	j["Class"] = unit->unitClass;
	j["ClassID"] = unit->classID;

	stats["HP"] = unit->maxHP,
	stats["Str"] = unit->strength,
	stats["Mag"] = unit->magic,
	stats["Skl"] = unit->skill,
	stats["Spd"] = unit->speed,
	stats["Lck"] = unit->luck,
	stats["Def"] = unit->defense,
	stats["Bld"] = unit->build,
	stats["Mov"] = unit->move,
	stats["Level"] = unit->level;

	stats["currentHP"] = unit->currentHP;
	stats["exp"] = unit->experience;

	j["ClassPower"] = unit->classPower;
	j["Position"] = { unit->sprite.position.x, unit->sprite.position.y };

	if (unit->skills.size() > 0)
	{
		for (int i = 0; i < unit->skills.size(); i++)
		{
			j["Skills"].push_back(unit->skills[i]);
		}
	}
	if (unit->uniqueWeapons.size() > 0)
	{
		for (int i = 0; i < unit->uniqueWeapons.size(); i++)
		{
			j["SpecialWeapons"].push_back(unit->uniqueWeapons[i]);
		}
	}
	j["Stats"] = stats;

	j["Inventory"] = json::array();

	for (int i = 0; i < unit->inventory.size(); ++i) 
	{
		j["Inventory"].push_back({ unit->inventory[i]->ID, unit->inventory[i]->remainingUses });
	}
	json weaponProf;
	for (int i = 0; i < 10; i++)
	{
		if (unit->weaponProficiencies[i] > 0)
		{
			weaponProf[weaponNames[i]] = unit->weaponProficiencies[i];
		}
	}
	j["WeaponProf"] = weaponProf;

	if (unit->mount)
	{
		auto mount = unit->mount;
		json mountData;
		mountData["Str"] = mount->str;
		mountData["Skl"] = mount->skl;
		mountData["Spd"] = mount->spd;
		mountData["Def"] = mount->def;
		mountData["Mov"] = mount->mov;

		json mWeaponProf;
		for (int i = 0; i < 10; i++)
		{
			if (mount->weaponProficiencies[i] > 0)
			{
				mWeaponProf[weaponNames[i]] = mount->weaponProficiencies[i];
			}
		}
		mountData["WeaponProf"] = mWeaponProf;
		mountData["AnimID"] = mount->ID;
		mountData["IsMounted"] = mount->mounted;
		j["Mount"] = mountData;
	}

	if (unit->carryingUnit)
	{
		j["Carried"] = true;
	}
	else
	{
		j["Carried"] = false;
	}

	return j;
}

//Current method does not save growth rates. For players we just load the from the json since they aren't going to change
//For enemies, since they cannot level up, I don't really care what their growths are after the level starts, so we don't save or load them at all
void SuspendGame()
{
	json j;
	json map;
	json settings;
	map["Level"] = currentLevel;
	map["CurrentRound"] = currentRound;
	map["Funds"] = playerManager.funds;
	map["Cursor"] = json::array();
	map["Cursor"].push_back(cursor.position.x);
	map["Cursor"].push_back(cursor.position.y);

	settings["UnitSpeed"] = Settings::settings.unitSpeed;
	settings["TextSpeed"] = Settings::settings.textSpeed;
	settings["Volume"] = Settings::settings.volume;
	settings["Animations"] = Settings::settings.mapAnimations;
	settings["AutoCursor"] = Settings::settings.autoCursor;
	settings["ShowTerrain"] = Settings::settings.showTerrain;
	settings["Sterero"] = Settings::settings.sterero;
	settings["Music"] = Settings::settings.music;
	json tile;
	json colorData;
	settings["SelectedTile"] = Settings::settings.backgroundPattern;
	for (int i = 0; i < Settings::settings.backgroundColors.size(); i++)
	{
		auto color = Settings::settings.backgroundColors;
		
		colorData["UpperAndLower"] = color[i];
		colorData["Edited"] = Settings::settings.editedColor[i];
		tile.push_back(colorData);
	}
	settings["Tile"] = tile;

	for (int i = 0; i < playerManager.units.size(); i++)
	{
		auto unit = playerManager.units[i];
		json unitData = UnitToJson(playerManager.units[i]);
		unitData["HasMoved"] = unit->hasMoved;
		unitData["BattleAnimation"] = unit->battleAnimations;
		j["player"].push_back(unitData);
		if (unit->carriedUnit)
		{
			j["Carried"].push_back({ 0, unit->levelID, unit->carriedUnit->team, unit->carriedUnit->levelID });
		}
	}
	for (int i = 0; i < enemyManager.units.size(); i++)
	{
		auto unit = enemyManager.units[i];
		json something = UnitToJson(enemyManager.units[i]);
		json AI;
		AI["activationType"] = unit->activationType;
		AI["stationary"] = unit->stationary;
		AI["boss"] = unit->boss;
		AI["active"] = unit->active;
		//Just writing this to here now
		if (unit->battleMessage != "")
		{
			AI["battleMessage"] = unit->battleMessage;
		}

		if (unit->carriedUnit)
		{
			j["Carried"].push_back({ 1, unit->levelID, unit->carriedUnit->team, unit->carriedUnit->levelID });
		}
		
		something["AI"] = AI;
		j["enemy"].push_back(something);
	}
	j["Map"] = map;
	j["Settings"] = settings;
	j["Scenes"] = json::array();
	for (int i = 0; i < sceneManager.scenes.size(); i++)
	{
		if (!sceneManager.scenes[i] || !sceneManager.scenes[i]->activation)
		{
			j["Scenes"].push_back(false);
		}
		else
		{
			j["Scenes"].push_back(true);
		}
	}

	std::ofstream file("suspendData.json");
	if (file.is_open()) {
		file << j.dump(4);  // Pretty-print with 4-space indentation
		file.close();
	}
}

//levelMap should probably be part of the saveFile
void loadSuspendedGame()
{
	std::ifstream f("suspendData.json");
	json data = json::parse(f);

	json mapLevel = data["Map"];
	std::string levelMap = mapLevel["Level"];
	currentRound = mapLevel["CurrentRound"];
	currentLevel = mapLevel["Level"];
	playerManager.funds = mapLevel["Funds"];
	cursor.position = glm::vec2(mapLevel["Cursor"][0], mapLevel["Cursor"][1]);
	std::ifstream map(levelDirectory + levelMap);

	json settings = data["Settings"];
	Settings::settings.unitSpeed = settings["UnitSpeed"];
	Settings::settings.textSpeed = settings["TextSpeed"];
	Settings::settings.volume = settings["Volume"];
	Settings::settings.mapAnimations = settings["Animations"];
	Settings::settings.autoCursor = settings["AutoCursor"];
	Settings::settings.showTerrain = settings["ShowTerrain"];
	Settings::settings.sterero = settings["Sterero"];
	Settings::settings.music = settings["Music"];

	Settings::settings.backgroundPattern = settings["SelectedTile"];

	auto colors = settings["Tile"];
	//UpperAndLower
	std::vector<std::vector<int>> inColors;
	inColors.resize(2);
	Settings::settings.editedColor.resize(2);
	for (int i = 0; i < 2; i++)
	{
		Settings::settings.editedColor[i] = colors[i]["Edited"];
		inColors[i].resize(6);
		for (int c = 0; c < 6; c++)
		{
			inColors[i][c] = colors[i]["UpperAndLower"][c];
		}
	}

	Settings::settings.backgroundColors = inColors;

	int xTiles = 0;
	int yTiles = 0;

	//j["Scenes"]
	while (map.good())
	{
		std::string thing;
		//	map >> thing;
		std::getline(map, thing);
		if (thing == "Level")
		{
			map >> xTiles >> yTiles;
			TileManager::tileManager.setTiles(map, xTiles, yTiles);

			json enemy = data["enemy"];
			playerManager.Load(data["player"]);
			enemyManager.Load(enemy, &playerManager.units, &vendors);

			for (const auto& carryData : data["Carried"])
			{
				Unit* carryingUnit = nullptr;
				Unit* carriedUnit = nullptr;
				std::vector<Unit*>* carryingUnitTeam;
				std::vector<Unit*>* carriedUnitTeam;
				if (carryData[0] == 0)
				{
					carryingUnitTeam = &playerManager.units;
				}
				else
				{
					carryingUnitTeam = &enemyManager.units;
				}
				if (carryData[2] == 0)
				{
					carriedUnitTeam = &playerManager.units;
				}
				else
				{
					carriedUnitTeam = &enemyManager.units;
				}
				for (int i = 0; i < carryingUnitTeam->size(); i++)
				{
					auto unit = (*carryingUnitTeam)[i];
					auto levelID = carryData[1];
					if (unit->levelID == levelID)
					{
						carryingUnit = unit;
						break;
					}
				}
				for (int i = 0; i < carriedUnitTeam->size(); i++)
				{
					auto unit = (*carriedUnitTeam)[i];
					auto levelID = carryData[3];
					if (unit->levelID == levelID)
					{
						carriedUnit = unit;
						break;
					}
				}
				carryingUnit->holdUnit(carriedUnit);
				carriedUnit->hide = true;
			}

		}
		else if (thing == "Scenes")
		{
			int numberOfScenes = 0;
			map >> numberOfScenes;
			sceneManager.scenes.resize(numberOfScenes);
			auto sceneStatus = data["Scenes"];
			for (int i = 0; i < numberOfScenes; i++)
			{
				sceneManager.scenes[i] = new Scene(&textManager);
				if (sceneStatus[i] == true)
				{
					int numberOfActions = 0;
					map >> numberOfActions;
					
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
						else if (actionType == START_MUSIC)
						{
							int musicID;
							map >> musicID;
							currentObject->actions[c] = new StartMusic(actionType, musicID);
						}
						else if (actionType == STOP_MUSIC)
						{
							float delay;
							map >> delay;
							currentObject->actions[c] = new StopMusic(actionType, delay);
						}
						else if (actionType == SHOW_MAP_TITLE)
						{
							float delay;
							map >> delay;
							currentObject->actions[c] = new ShowTitle(actionType, delay);
						}
					}
					int activationType = 0;
					map >> activationType;
					if (activationType == 0)
					{
						int talker = 0;
						int listener = 0;
						map >> talker >> listener;
						if (sceneUnits.count(talker)) //Not currently going to work for enemy scene units...
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
					else if (activationType == 3) //Should never be hitting this
					{
					//	intro = i;
						currentObject->activation = new IntroActivation(currentObject, activationType);
					}
					else if (activationType == 4)
					{
						currentObject->activation = new EndingActivation(currentObject, activationType);
						endingID = i;
					}

					map >> currentObject->repeat;
				}
				else
				{
					map.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
					map.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				}
			}
		}
		else if (thing == "Visits")
		{
			int numberOfVisits = 0;
			map >> numberOfVisits;
			visitObjects.resize(numberOfVisits);
			bool redraw = false;
			for (int i = 0; i < numberOfVisits; i++)
			{
				glm::ivec2 position;
				map >> position.x >> position.y;
				int numberOfIDs = 0;
				map >> numberOfIDs;
				bool allGood = true;
				for (int c = 0; c < numberOfIDs; c++)
				{
					int unitID = 0;
					int sceneID = 0;
					map >> unitID >> sceneID;
					if (sceneManager.scenes[sceneID]->activation)
					{
						visitObjects[i].position = position;
						visitObjects[i].sceneMap[unitID] = sceneManager.scenes[sceneID];
						sceneManager.scenes[sceneID]->visit = &visitObjects[i];
					}
					else
					{
						allGood = false;
					}
				}
				if (allGood)
				{
					TileManager::tileManager.placeVisit(position.x, position.y, &visitObjects[i]);
				}
				else
				{
					TileManager::tileManager.getTile(position.x, position.y)->uvID = 184;
					redraw = true;
					visitObjects.erase(visitObjects.begin() + i);
					i--;
					numberOfVisits--;
				}
			}
			if (redraw)
			{
				TileManager::tileManager.reDraw();
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
		else if (thing == "EnemyEscape")
		{
			int x = 0;
			int y = 0;
			map >> x >> y;
			enemyManager.escapePoint = glm::ivec2(x, y);
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
		else if (thing == "Seize")
		{
			int x = 0;
			int y = 0;
			map >> x >> y;
			seizePoint = glm::vec2(x, y);
			TileManager::tileManager.placeSeizePoint(seizePoint.x, seizePoint.y);
		}
	}

	map.close();

	PlayerTurnMusic();

	levelWidth = xTiles * TileManager::TILE_SIZE;
	levelHeight = yTiles * TileManager::TILE_SIZE;
	camera = Camera(256, 224, levelWidth, levelHeight);

	camera.setPosition(glm::vec2(0, 0));
	//	camera.setScale(2.0f);
	camera.update();
}

void SetFadeIn(bool delay)
{
	fadingIn = true;
	fadeInDelay = delay;
	fadeDelayTimer = 0.0f;
}

void PlayerTurnMusic()
{
	if (enemyManager.units.size() < playerManager.units.size())
	{
		mapMusic = 1;
		ResourceManager::PlayMusic("WinningStart", "WinningLoop");
	}
	else if (playerManager.units.size() < 2)
	{
		mapMusic = 2;
		ResourceManager::PlayMusic("LosingStart", "LosingLoop");
	}
	else
	{
		mapMusic = 0;
		ResourceManager::PlayMusic("PlayerTurn");
	}
}