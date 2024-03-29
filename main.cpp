
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

#include "Globals.h"
#include "InfoDisplays.h"

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
void loadMap(std::string nextMap);
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
//Unit unit;
//Unit allyUnit;
std::vector<Unit*> playerUnits;
//std::vector<Unit*> enemies;

InputManager inputManager;

BattleManager battleManager;

EnemyManager enemyManager;

InfoDisplays displays;

int currentTurn = 0;
bool turnTransition = false;
bool turnDisplay = false;
//Ugh. To handle healing units on turn transition
int turnUnit = 0;

struct UnitEvents : public Observer
{
	virtual void onNotify(Unit* lUnit)
	{
		displays.OnUnitLevel(lUnit);
	}
};

struct TurnEvents : public TurnObserver
{
	virtual void onNotify(int ID)
	{
		for (int i = 0; i < playerUnits.size(); i++)
		{
			if (playerUnits[i]->isDead)
			{
				playerUnits.erase(playerUnits.begin() + i);
				i--;
			}
		}
		enemyManager.RemoveDeadUnits();
		if (ID == 0)
		{
			cursor.focusedUnit = nullptr;
			for (int i = 0; i < playerUnits.size(); i++)
			{
				playerUnits[i]->EndTurn();
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
		}
		displays.ChangeTurn(currentTurn);
	}
};

struct BattleEvents : public BattleObserver
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

struct PostBattleEvents : public PostBattleObserver
{
	virtual void onNotify(int ID)
	{
		if (ID == 0)
		{
			battleManager.EndBattle(&cursor, &enemyManager, camera);
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
struct ItemEvents : public ItemUseObserver
{
	virtual void onNotify(Unit* unit, int index)
	{
		//Do something in display here...
		displays.StartUse(unit, index, &camera);
	}
};

std::mt19937 gen;
//gen.seed(1);
std::uniform_int_distribution<int> distribution(0, 99);
int main(int argc, char** argv)
{
	init();

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
	ResourceManager::LoadShader("Shaders/instanceVertexShader.txt", "Shaders/spriteFragmentShader.txt", nullptr, "instance");
	ResourceManager::LoadShader("Shaders/shapeVertexShader.txt", "Shaders/shapeFragmentShader.txt", nullptr, "shape");

	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera.getCameraMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);

	ResourceManager::GetShader("sprite").Use().SetInteger("image", 0);
	ResourceManager::GetShader("sprite").SetMatrix4("projection", camera.getCameraMatrix());

	ResourceManager::GetShader("instance").Use().SetInteger("image", 0);
	ResourceManager::GetShader("instance").SetMatrix4("projection", camera.getCameraMatrix());
	glm::vec4 color(1.0f);
	ResourceManager::GetShader("instance").Use().SetVector4f("spriteColor", color);

//	ResourceManager::LoadShader("Shaders/spriteVertexShader.txt", "Shaders/sliceFragmentShader.txt", nullptr, "slice");

//	ResourceManager::LoadShader("Shaders/postVertexShader.txt", "Shaders/postFragmentShader.txt", nullptr, "postprocessing");

	ResourceManager::LoadTexture("E:/Damon/dev stuff/FE5Test/TestSprites/tilesheet2.png", "tiles");
	ResourceManager::LoadTexture("E:/Damon/dev stuff/FE5Test/TestSprites/cursor.png", "cursor");
	ResourceManager::LoadTexture("E:/Damon/dev stuff/FE5Test/TestSprites/sprites.png", "sprites");

	Shader myShader;
	myShader = ResourceManager::GetShader("sprite");
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
	PostBattleEvents* postBattleEvents = new PostBattleEvents();
	ItemEvents* itemEvents = new ItemEvents();
	ItemManager::itemManager.subject.addObserver(itemEvents);
	battleManager.subject.addObserver(battleEvents);
	displays.subject.addObserver(postBattleEvents);
	loadMap("2.map");
	std::vector<glm::vec4> playerUVs = ResourceManager::GetTexture("sprites").GetUVs(TILE_SIZE, TILE_SIZE);

	std::ifstream f("BaseStats.json");
	json data = json::parse(f);
	json bases = data["PlayerUnits"];
	int currentUnit = 0;
	std::unordered_map<std::string, int> weaponNameMap;
	weaponNameMap["Sword"] = WeaponData::TYPE_SWORD;
	weaponNameMap["Axe"] = WeaponData::TYPE_AXE;
	weaponNameMap["Lance"] = WeaponData::TYPE_LANCE;
	weaponNameMap["Bow"] = WeaponData::TYPE_BOW;
	weaponNameMap["Thunder"] = WeaponData::TYPE_THUNDER;
	weaponNameMap["Fire"] = WeaponData::TYPE_FIRE;
	weaponNameMap["Wind"] = WeaponData::TYPE_WIND;
	weaponNameMap["Dark"] = WeaponData::TYPE_DARK;
	weaponNameMap["Light"] = WeaponData::TYPE_LIGHT;
	weaponNameMap["Staff"] = WeaponData::TYPE_STAFF;
	playerUnits.resize(bases.size());
	for (const auto& unit : bases) {
		int ID = unit["ID"];
		std::string name = unit["Name"];
		std::string unitClass = unit["Class"];
		json stats = unit["Stats"];
		int HP = stats["HP"];
		int str = stats["Str"];
		int mag = stats["Mag"];
		int skl = stats["Skl"];
		int spd = stats["Spd"];
		int lck = stats["Lck"];
		int def = stats["Def"];
		int bld = stats["Bld"];
		int mov = stats["Mov"];
		Unit* newUnit = new Unit(unitClass, name, HP, str, mag, skl, spd, lck, def, bld, mov);

		json growths = unit["GrowthRates"];
		HP = growths["HP"];
		str = growths["Str"];
		mag = growths["Mag"];
		skl = growths["Skl"];
		spd = growths["Spd"];
		lck = growths["Lck"];
		def = growths["Def"];
		bld = growths["Bld"];
		mov = growths["Mov"];

		newUnit->growths = StatGrowths{ HP, str, mag, skl, spd, lck, def, bld, mov };
		json weaponProf = unit["WeaponProf"];
		for (auto it = weaponProf.begin(); it != weaponProf.end(); ++it)
		{
			newUnit->weaponProficiencies[weaponNameMap[it.key()]] = int(it.value());
		}
		if (unit.find("SpecialWeapons") != unit.end()) {
			auto specialWeapons = unit["SpecialWeapons"];
			for (const auto& weapon : specialWeapons)
			{
				newUnit->uniqueWeapons.push_back(int(weapon));
			}
		}

		if (unit.find("Skills") != unit.end()) 
		{
			auto skills = unit["Skills"];
			for (const auto& skill : skills)
			{
				newUnit->skills.push_back(int(skill));
			}
		}
		json inventory = unit["Inventory"];
		for (const auto& item : inventory) 
		{
			newUnit->addItem(int(item));
		}
		if (unit.find("ClassPower") != unit.end())
		{
			newUnit->classPower = unit["ClassPower"];
		}

		newUnit->subject.addObserver(unitEvents);
		newUnit->init(&gen, &distribution);
		newUnit->sprite.uv = &playerUVs;
		playerUnits[currentUnit] = newUnit;
		currentUnit++;
	}
	playerUnits[0]->placeUnit(48, 304);
	playerUnits[0]->currentHP = 11;
//	playerUnits[0]->experience = 90;
//	playerUnits[0]->currentHP = 10;
//	playerUnits[0]->magic = 20;
	playerUnits[1]->placeUnit(32, 304);
	playerUnits[1]->movementType = Unit::FOOT;
	playerUnits[1]->mount = new Mount(Unit::HORSE, 1, 1, 1, 2, 3);
	playerUnits[2]->placeUnit(48, 288);
	playerUnits[2]->currentHP = 10;
//	playerUnits[2]->defense = playerUnits[0]->defense;
//	playerUnits[2]->strength = 20;

//	enemies[0]->init(&gen, &distribution);
//	enemies[0]->sprite.uv = &playerUVs;
//	enemies[1]->sprite.uv = &playerUVs;
//	enemies[0]->skills.push_back(Unit::VANTAGE);
//	enemies[0]->skills.push_back(Unit::WRATH);

//	enemies[0]->addItem(8);
//	enemies[0]->equipWeapon(0);

	//enemies[0]->LevelEnemy(9);

//	enemyManager.enemies = enemies;
	cursor.position = playerUnits[0]->sprite.getPosition();

	MenuManager::menuManager.SetUp(&cursor, Text, &camera, shapeVAO, Renderer, &battleManager);
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

		if (MenuManager::menuManager.menus.size() == 0)
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
							if (turnUnit >= playerUnits.size())
							{
								turnUnit = 0;
								cursor.position = playerUnits[0]->sprite.getPosition();
								cursor.focusedUnit = playerUnits[0];
								camera.SetMove(cursor.position);
								turnTransition = false;
							}
							else
							{
								playerUnits[turnUnit]->StartTurn(displays, &camera);

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
						enemyManager.Update(deltaTime, battleManager, camera);
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
		for (int i = 0; i < playerUnits.size(); i++)
		{
			playerUnits[i]->Update(deltaTime);
			playerUnits[i]->UpdateMovement(deltaTime);
		}
		if (!camera.moving)
		{
			enemyManager.UpdateEnemies(deltaTime);
		}
		else
		{
			int a = 2;
		}

		/*if (inputManager.isKeyPressed(SDLK_e))
		{
			enemyManager.GetPriority();
			//if (enemies[1]->movementComponent.moving)
			{
				testMoving = true;
			}
		}*/

		camera.update();

		Draw();
		fps = fpsLimiter.end();
		//std::cout << fps << std::endl;
	}

	delete Renderer;
	delete Text;
	enemyManager.Clear();
	for (int i = 0; i < playerUnits.size(); i++)
	{
		delete playerUnits[i];
	}
	playerUnits.clear();
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

void loadMap(std::string nextMap)
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
			enemyManager.SetUp(map, &gen, &distribution, &playerUnits);
		}
	}

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
		for (int i = 0; i < playerUnits.size(); i++)
		{
			playerUnits[i]->Draw(Renderer);
		}
		enemyManager.Draw(Renderer);

		Renderer->setUVs(cursor.uvs[1]);
		Texture2D displayTexture = ResourceManager::GetTexture("cursor");
		Renderer->DrawSprite(displayTexture, cursor.position, 0.0f, cursor.dimensions);
	}
	if (drawingMenu)
	{
		auto menu = MenuManager::menuManager.menus.back();
		menu->Draw();
	}
	if (!fullScreenMenu)
	{
		DrawText();
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
		displays.Draw(&camera, Text, shapeVAO);
	}
	else if (battleManager.battleActive)
	{
		battleManager.Draw(Text, camera);
	}
	else if (!cursor.fastCursor && cursor.selectedUnit == nullptr && MenuManager::menuManager.menus.size() == 0)
	{
		auto tile = TileManager::tileManager.getTile(cursor.position.x, cursor.position.y)->properties;

		//Going to need to look into a better way of handling UI placement at some point
		int xStart = SCREEN_WIDTH;
		glm::vec2 fixedPosition = camera.worldToScreen(cursor.position);
		if (fixedPosition.x >= camera.screenWidth * 0.5f)
		{
			xStart = 178;
		}
		Text->RenderText(tile.name, xStart - 110, 20, 1);
		Text->RenderText("DEF", xStart - 120, 50, 0.7f, glm::vec3(0.69f, 0.62f, 0.49f));
		Text->RenderText(intToString(tile.defense), xStart - 95, 50, 0.7f);
		Text->RenderText("AVO", xStart - 85, 50, 0.7f, glm::vec3(0.69f, 0.62f, 0.49f));
		Text->RenderText(intToString(tile.avoid) + "%", xStart - 60, 50, 0.7f);

		if (auto unit = cursor.focusedUnit)
		{
			unit->sprite.getPosition();
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
	int aspectHeight = float(aspectWidth) / ratio;
	if (aspectHeight > height)
	{
		aspectHeight = height;
		aspectWidth = float(aspectHeight) * ratio;
	}
	int vpx = float(width) / 2.0f - float(aspectWidth) / 2.0f;
	int vpy = float(height) / 2.0f - float(aspectHeight) / 2.0f;
	glViewport(vpx, vpy, aspectWidth, aspectHeight);
}
