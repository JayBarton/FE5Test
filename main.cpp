
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

#include <vector>
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

#include <algorithm>
#include <random>
#include <ctime>

void init();
void Draw();
void DrawText();
void loadMap(std::string nextMap);
std::string intToString(int i);


// The Width of the screen
const GLuint SCREEN_WIDTH = 800;
// The height of the screen
const GLuint SCREEN_HEIGHT = 600;

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

//Not sure where I want all this defined
struct Cursor
{
	glm::vec2 position;
	glm::vec2 dimensions = glm::vec2(TILE_SIZE, TILE_SIZE);
	std::vector<glm::vec4> uvs;
};
Cursor cursor;
bool fastCursor = false;

InputManager inputManager;

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

	camera.setPosition(glm::vec2(0.0f, 0.0f));
	camera.update();

	ResourceManager::LoadShader("Shaders/spriteVertexShader.txt", "Shaders/spriteFragmentShader.txt", nullptr, "sprite");
	ResourceManager::LoadShader("Shaders/instanceVertexShader.txt", "Shaders/spriteFragmentShader.txt", nullptr, "instance");

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

	ResourceManager::LoadTexture("E:/Damon/dev stuff/FE5Test/TestSprites/tilesheettest.png", "tiles");
	ResourceManager::LoadTexture("E:/Damon/dev stuff/FE5Test/TestSprites/cursor.png", "cursor");

	Shader myShader;
	myShader = ResourceManager::GetShader("sprite");
	Renderer = new SpriteRenderer(myShader);

	TileManager::tileManager.uvs = ResourceManager::GetTexture("tiles").GetUVs(TILE_SIZE, TILE_SIZE);

	cursor.uvs = ResourceManager::GetTexture("cursor").GetUVs(TILE_SIZE, TILE_SIZE);
	cursor.dimensions = glm::vec2(TileManager::TILE_SIZE);
	cursor.position = glm::vec2(0);
	Text = new TextRenderer(800, 600);
	Text->Load("fonts/Teko-Light.TTF", 30);

	loadMap("1.map");

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
			}

			if (inputManager.isKeyPressed(SDLK_ESCAPE))
			{
				isRunning = false;
			}
		}

		float cursorSpeed = 0.5f;
		if (inputManager.isKeyDown(SDLK_LSHIFT))
		{
			fastCursor = true;
			cursorSpeed = 1.0f;
		}
		else if (inputManager.isKeyReleased(SDLK_LSHIFT))
		{
			fastCursor = false;
		}
		int xSpeed = 0;
		int ySpeed = 0;
		if (inputManager.isKeyDown(SDLK_RIGHT))
		{
			xSpeed = 1;
		}
		if (inputManager.isKeyDown(SDLK_LEFT))
		{
			xSpeed = -1;
		}
		if (inputManager.isKeyDown(SDLK_UP))
		{
			ySpeed = -1;
		}
		if (inputManager.isKeyDown(SDLK_DOWN))
		{
			ySpeed = 1;
		}
		float size = TileManager::TILE_SIZE * camera.getScale();
		cursor.position.x += xSpeed * cursorSpeed * TileManager::TILE_SIZE;
		cursor.position.y += ySpeed * cursorSpeed * TileManager::TILE_SIZE;

		//Keep cursor positions on the tiles
		//This all sucks, need a better way of handling this. The issue is I need to floor when going left or down but need
		//ceil when going up or right
		if (inputManager.isKeyReleased(SDLK_LEFT))
		{
			cursor.position.x = int(cursor.position.x / TileManager::TILE_SIZE) * TileManager::TILE_SIZE;
		}
		else if (inputManager.isKeyReleased(SDLK_RIGHT))
		{
			cursor.position.x = ceil(cursor.position.x / TileManager::TILE_SIZE) * TileManager::TILE_SIZE;
		}
		if (inputManager.isKeyReleased(SDLK_UP))
		{
			cursor.position.y = int(cursor.position.y / TileManager::TILE_SIZE) * TileManager::TILE_SIZE;
		}
		else if (inputManager.isKeyReleased(SDLK_DOWN))
		{
			cursor.position.y = ceil(cursor.position.y / TileManager::TILE_SIZE) * TileManager::TILE_SIZE;
		}

		camera.setPosition(cursor.position);
		camera.update();

		Draw();
		fps = fpsLimiter.end();
		std::cout << fps << std::endl;
	}

	delete Renderer;

	/*for (int i = 0; i < soundEffects.size(); i++)
	{
		for (int c = 0; c < soundEffects[i].size(); c++)
		{
			Mix_FreeChunk(soundEffects[i][c]);
			soundEffects[i][c] = nullptr;
		}
	}*/

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
	int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
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

	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

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

		map.close();

		levelWidth = xTiles * TileManager::TILE_SIZE;
		levelHeight = yTiles * TileManager::TILE_SIZE;
		camera = Camera(800, 600, levelWidth, levelHeight);
		//camera = Camera(256, 224, levelWidth, levelHeight);


		camera.setPosition(glm::vec2(0, 0));
		camera.setScale(2.0f);
		camera.update();
	}
}

void Draw()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	ResourceManager::GetShader("sprite").Use().SetMatrix4("projection", camera.getCameraMatrix());
	Texture2D texture = ResourceManager::GetTexture("bg");
	Renderer->setUVs();

	ResourceManager::GetShader("shape").Use();

	ResourceManager::GetShader("shape").SetMatrix4("projection", camera.getCameraMatrix());

	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0);

	ResourceManager::GetShader("instance").Use();
	ResourceManager::GetShader("instance").SetMatrix4("projection", camera.getCameraMatrix());
	TileManager::tileManager.showTiles(Renderer, camera);

	ResourceManager::GetShader("sprite").Use().SetMatrix4("projection", camera.getCameraMatrix());

	Renderer->setUVs(cursor.uvs[1]);
	Texture2D displayTexture = ResourceManager::GetTexture("cursor");
	Renderer->DrawSprite(displayTexture, cursor.position, 0.0f, cursor.dimensions);

	DrawText();
	
	SDL_GL_SwapWindow(window);

}

void DrawText()
{
	if (!fastCursor)
	{
		auto tile = TileManager::tileManager.getTile(cursor.position.x, cursor.position.y)->properties;

		//Going to need to look into a better way of handling UI placement at some point
		int xStart = SCREEN_WIDTH;
		glm::vec2 fixedPosition = camera.worldToScreen(cursor.position);
		if (fixedPosition.x >= SCREEN_WIDTH * 0.5f)
		{
			xStart = 178;
		}
		Text->RenderText(tile.name, xStart - 110, 20, 1);
		Text->RenderText("DEF", xStart - 120, 50, 0.7f, glm::vec3(0.69f, 0.62f, 0.49f));
		Text->RenderText(intToString(tile.defense), xStart - 95, 50, 0.7f);
		Text->RenderText("AVO", xStart - 85, 50, 0.7f, glm::vec3(0.69f, 0.62f, 0.49f));
		Text->RenderText(intToString(tile.avoid) + "%", xStart - 60, 50, 0.7f);
	}
}

std::string intToString(int i)
{
	std::stringstream s;
	s << i;
	return s.str();
}