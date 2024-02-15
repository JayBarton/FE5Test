
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

#include <vector>
#include <map>
#include <unordered_map>
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

#include <string>     // std::string, std::stoi

void init();
bool loadMap();
void saveMap();

void editInput(SDL_Event& event, bool& isRunning);
void switchMode();
std::string intToString(int i);
void Draw();
void resizeWindow(int width, int height);


enum State { MAIN_MENU, NEW_MAP, EDITING, SET_BACKGROUND, RESIZE_MAP, SET_WIDTH };

State state = MAIN_MENU;

bool typing = true;
bool leftHeld = false;
bool rightHeld = false;

std::string bg;

const static int TILE_SIZE = 16;
const static int NUMBER_OF_ENEMIES = 2;
const static int ENEMY_WRITE = 1;

struct Object
{
    int type;
    glm::vec2 position;
    glm::vec2 dimensions = glm::vec2(TILE_SIZE, TILE_SIZE);
    glm::vec4 uvs;
};

struct vec2Hash
{
    size_t operator()(const glm::vec2& vec) const
    {
        return ((std::hash<float>()(vec.x) ^ (std::hash<float>()(vec.y) << 1)) >> 1);
    }
};

std::unordered_map<glm::vec2, Object, vec2Hash> objects;
std::unordered_map<glm::vec2, std::string, vec2Hash> objectStrings;
std::unordered_map<glm::vec2, int, vec2Hash> objectWriteTypes;

std::vector<glm::vec4> enemyUVs;

Object displayObject;

int numberOfPickups = 0;
int numberOfEnemies = 0;

struct EditMode
{
    const static int TILE = 0;
    const static int PICKUP = 1;
    const static int ENEMY = 2;
    int currentElement;
    int maxElement;
    //The current edit mode
    int type = -1;

    EditMode()
    {
        currentElement = 0;
        displayObject.type = currentElement;
    }

    virtual void rightClick(int x, int y) {}
    virtual void leftClick(int x, int y) {}
    virtual void switchElement(int next)
    {
        currentElement += next;
        if (currentElement < 0)
        {
            currentElement = maxElement;
        }
        if (currentElement > maxElement)
        {
            currentElement = 0;
        }

        displayObject.type = currentElement;
    }

    virtual ~EditMode() {}
    //texture to use
};

struct TileMode : public EditMode
{
    TileMode()
    {
        maxElement = 5;
        type = TILE;
    }

    void rightClick(int x, int y)
    {
        //Remove tile
        TileManager::tileManager.placeTile(x, y, -1);
    }

    //TODO need a method to set a tile and attribute
    void leftClick(int x, int y)
    {
        TileManager::tileManager.placeTile(x, y, displayObject.type);
    }

    ~TileMode()
    {
    }
};


struct EnemyMode : public EditMode
{
    //1 when facing right, 2 when facing left
    //will double the type for uvs
    int facing;
    const static int RIGHT = 0;
    const static int LEFT = 1;

    const static int WALKER = 0;
    const static int CHARGER = 1;
    const static int ARMOR_CHARGER = 2;
    const static int SHOOTER = 3;
    const static int ARMOR_SHOOTER = 4;
    const static int FLYER = 5;

    EnemyMode()
    {
        facing = RIGHT;
        maxElement = NUMBER_OF_ENEMIES - 1;
        updateDisplay();
        type = ENEMY;
    }

    void rightClick(int x, int y)
    {
        glm::vec2 mousePosition(x, y);
        if (objects.count(mousePosition) == 1)
        {
            objects.erase(mousePosition);
            objectStrings.erase(mousePosition);
            objectWriteTypes.erase(mousePosition);
            numberOfEnemies--;
        }
    }

    void leftClick(int x, int y)
    {
        glm::vec2 mousePosition(x, y);


        if (objects.count(mousePosition) == 0)
        {
            glm::vec2 	startPosition = displayObject.position;
            objects[startPosition] = displayObject;
            objects[startPosition].position = startPosition;

            std::stringstream objectStream;
            objectStream << displayObject.type << " " << startPosition.x << " " << startPosition.y;

            objectWriteTypes[startPosition] = ENEMY_WRITE; //not sure which of these I'm using

            numberOfEnemies++;

            objectStrings[startPosition] = objectStream.str();

            std::cout << objectStrings[startPosition] << std::endl;
        }
    }

    void switchElement(int next)
    {
        EditMode::switchElement(next);
        updateDisplay();
    }

    void swapFacing()
    {
        if (facing == RIGHT)
        {
            facing = LEFT;
        }
        else
        {
            facing = RIGHT;
        }
        updateDisplay();
    }

    void updateDisplay()
    {
        displayObject.uvs = enemyUVs[currentElement];

        displayObject.dimensions = glm::vec2(16, 16);
    }

    ~EnemyMode()
    {
    }
};

// The Width of the screen
const GLuint SCREEN_WIDTH = 800;
// The height of the screen
const GLuint SCREEN_HEIGHT = 600;

const int CAMERA_WIDTH = 256;
const int CAMERA_HEIGHT = 224;

int currentWidth = 800;
int currentHeight = 600;

int currentVPX;
int currentVPY;

SDL_Window* window;
SpriteRenderer* Renderer;
TextRenderer* Text;

GLuint shapeVAO;

//Camera camera(SCREEN_WIDTH, SCREEN_HEIGHT, 2400, 800);
Camera camera;

EditMode* editMode;

glm::vec2 mousePosition;

std::string mapName;

std::vector<std::string> inputText;

int dontknow = 0;

//For text input
const static int LEVEL_WIDTH_STRING = 0;
const static int LEVEL_HEIGHT_STRING = 1;

//How long the message informing the user they have saved
//should display
bool saveDisplay = true;
float saveTimer = 0.0f;
float saveTime = 1.0f;

int levelWidth;
int levelHeight;

bool loading = true; //not great but it works

int main(int argc, char** argv)
{
    init();

    inputText.resize(2);

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

    ResourceManager::LoadShader("Shaders/spriteVertexShader.txt", "Shaders/spriteFragmentShader.txt", nullptr, "sprite");
    ResourceManager::LoadShader("Shaders/shapeVertexShader.txt", "Shaders/shapeFragmentShader.txt", nullptr, "shape");

   // ResourceManager::LoadTexture("spritesheet.png", "sprites");
    ResourceManager::LoadTexture("E:/Damon/dev stuff/FE5Test/TestSprites/tilesheettest.png", "tiles");
    ResourceManager::LoadTexture("E:/Damon/dev stuff/FE5Test/TestSprites/sprites.png", "sprites");

    ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera.getCameraMatrix());
    ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);

    ResourceManager::GetShader("sprite").Use().SetInteger("image", 0);
    ResourceManager::GetShader("sprite").SetMatrix4("projection", camera.getCameraMatrix());

    Shader myShader;
    myShader = ResourceManager::GetShader("sprite");
    Renderer = new SpriteRenderer(myShader);

    enemyUVs = ResourceManager::GetTexture("sprites").GetUVs(TILE_SIZE, TILE_SIZE);

    editMode = new TileMode();

    Text = new TextRenderer(800, 600);
    Text->Load("fonts\\Teko-Light.TTF", 30);

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

        //Handle events on queue

        SDL_StartTextInput();
        while (SDL_PollEvent(&event) != 0)
        {
            //User requests quit
            if (event.type == SDL_QUIT)
            {
                isRunning = false;
            }
            else if (event.type == SDL_WINDOWEVENT)
            {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    resizeWindow(event.window.data1, event.window.data2);
                }
            }
            if (typing)
            {
                if (event.type == SDL_TEXTINPUT)
                {
                    //Append character
                    inputText[dontknow] += event.text.text;
                }
                if (event.type == SDL_KEYDOWN)
                {
                    if (event.key.keysym.sym == SDLK_ESCAPE)
                    {
                        isRunning = false;
                    }
                    else if (event.key.keysym.sym == SDLK_BACKSPACE && inputText[dontknow].length() > 0)
                    {
                        inputText[dontknow].pop_back();
                    }
                    else if (event.key.keysym.sym == SDLK_RETURN)
                    {
                        if (state == State::MAIN_MENU)
                        {
                            mapName = inputText[0] + ".map";

                            if (loading)
                            {
                                if (loadMap())
                                {
                                    camera = Camera(256, 224, levelWidth * TILE_SIZE, levelHeight * TILE_SIZE);
                                    state = State::EDITING;
                                    typing = false;
                                    SDL_StopTextInput();
                                }
                            }
                            else
                            {
                                state = State::NEW_MAP;
                            }

                            inputText[0] = "";
                            inputText[1] = "";
                            dontknow = 0;
                        }
                        else
                        {
                            typing = false;
                            SDL_StopTextInput();
                            if (state == State::NEW_MAP)
                            {
                                levelWidth = atoi(inputText[LEVEL_WIDTH_STRING].c_str());
                                levelHeight = atoi(inputText[LEVEL_HEIGHT_STRING].c_str());

                                TileManager::tileManager.newMap(levelWidth, levelHeight);
                                camera = Camera(256, 224, levelWidth * TILE_SIZE, levelHeight * TILE_SIZE);
                            }
                            else if (state == RESIZE_MAP)
                            {
                                int oldHeight = levelHeight;
                                levelWidth = atoi(inputText[LEVEL_WIDTH_STRING].c_str());
                                levelHeight = atoi(inputText[LEVEL_HEIGHT_STRING].c_str());

                                bool shift = false;
                                if (shift)
                                {
                                    for (auto& iter : objects)
                                    {
                                        iter.second.position.y += (levelHeight - oldHeight) * TILE_SIZE;
                                    }
                                }

                                //also going to take a shift bool that will move the map when resizing
                                //calculate width/height distance here and then pass it in, use that difference to shift objects if using the shift
                                //Only know how to shift in Y direction right now. Might be all I need?

                                TileManager::tileManager.resizeMap(levelWidth, levelHeight);
                                camera = Camera(256, 224, levelWidth * TILE_SIZE, levelHeight * TILE_SIZE);
                            }
                            else if (state == SET_BACKGROUND)
                            {
                                bg = inputText[0];
                                std::string newBackground = bg + ".png";
                                ResourceManager::LoadTexture(newBackground.c_str(), "bg");
                            }
                            else if (state == SET_WIDTH)
                            {
                                displayObject.dimensions.x = atoi(inputText[0].c_str()) * TILE_SIZE;
                            }

                            state = State::EDITING;

                            inputText[0] = "";
                            inputText[1] = "";
                            dontknow = 0;
                        }
                    }
                    else if (event.key.keysym.sym == SDLK_TAB)
                    {
                        if (state == State::MAIN_MENU)
                        {
                            loading = !loading;
                        }
                        else
                        {
                            dontknow++;
                            if (dontknow > 1)
                            {
                                dontknow = 0;
                            }
                        }
                    }
                }
            }
            else
            {
                editInput(event, isRunning);
            }
        }
        camera.update();

        if (!typing)
        {
            const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);

            if (currentKeyStates[SDL_SCANCODE_RIGHT])
            {
                camera.setPosition(camera.getPosition() + glm::vec2(25.0f, 0.0f));
            }
            if (currentKeyStates[SDL_SCANCODE_LEFT])
            {
                camera.setPosition(camera.getPosition() - glm::vec2(25.0f, 0.0f));
            }
            if (currentKeyStates[SDL_SCANCODE_UP])
            {
                camera.setPosition(camera.getPosition() - glm::vec2(0.0f, 25.0f));
            }
            if (currentKeyStates[SDL_SCANCODE_DOWN])
            {
                camera.setPosition(camera.getPosition() + glm::vec2(0.0f, 25.0f));
            }
            camera.update();

            //Mouse offsets
            int x = 0;
            int y = 0;

            //Get mouse offsets
            SDL_GetMouseState(&x, &y);

            mousePosition = glm::vec2(x - TILE_SIZE * 0.5f, y - TILE_SIZE * 0.5f);

            mousePosition = camera.screenToWorld(mousePosition, currentWidth, currentHeight, currentVPX, currentVPY);

            mousePosition.x = round(float(mousePosition.x) / TILE_SIZE) * TILE_SIZE;
            mousePosition.y = round(float(mousePosition.y) / TILE_SIZE) * TILE_SIZE;

            displayObject.position = mousePosition;

            if (saveDisplay)
            {
                saveTimer += deltaTime;
                if (saveTimer >= saveTime)
                {
                    saveTimer = 0.0f;
                    saveDisplay = false;
                }
            }
        }

        Draw();
        fps = fpsLimiter.end();
    }


    delete Renderer;
    delete Text;
    delete editMode;

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

    window = SDL_CreateWindow("Map Editor", SDL_WINDOWPOS_CENTERED,
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
  //  glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    resizeWindow(SCREEN_WIDTH, SCREEN_HEIGHT);

    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glEnable(GL_MULTISAMPLE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

bool loadMap()
{
    std::ifstream map(mapName);

    std::cout << "start\n";
    bool good = (bool)map;
    while (map.good())
    {
        std::string thing;
        map >> thing;

        if (thing == "Level")
        {
            map >> levelWidth >> levelHeight;
            TileManager::tileManager.setTiles(map, levelWidth, levelHeight);
        }
        else if (thing == "Enemies")
        {
            map >> numberOfEnemies;
            for (size_t i = 0; i < numberOfEnemies; i++)
            {
                std::string str;
                glm::vec2 position;
                int type;

                map >> type >> position.x >> position.y;

                Object tObject;
                tObject.position = position;
                tObject.type = type;
                tObject.uvs = enemyUVs[type];

                std::stringstream stream;
                stream << type << " " << position.x << " " << position.y;

                objects[position] = tObject;
                objectWriteTypes[position] = ENEMY_WRITE;
                objectStrings[position] = stream.str();
            }
        }

    }
    map.close();
    return good;
}

void saveMap()
{
    std::ofstream map(mapName);

    map << "Level\n";
    map << TileManager::tileManager.saveTiles();
    map << "\n";
    std::string enemies = "Enemies\n";
    std::stringstream stream;
    stream << numberOfEnemies;
    enemies += stream.str();

    for (auto& iter : objectWriteTypes)
    {
        if (iter.second == ENEMY_WRITE)
        {
            enemies += "\n" + objectStrings[iter.first];
        }
    }

    map << enemies << "\n";

    map.close();

    //save to game folder
    std::ofstream mapP("E:\\Damon\\dev stuff\\FE5Test\\Levels\\" + mapName);
    mapP << "Level\n";
    mapP << TileManager::tileManager.saveTiles();
    mapP << "\n";
    mapP << enemies << "\n";
    mapP.close();
    //save to debug folder
   /* std::ofstream mapD("E:\\Damon\\dev stuff\\FE5Test\\bin\\Debug/" + mapName);
    mapD << "Level\n";
    mapD << TileManager::tileManager.saveTiles();

    //save to release folder
    std::ofstream mapR("E:\\Damon\\dev stuff\\FE5Test\\bin\\Release\\Levels/" + mapName);
    mapR << "Level\n";
    mapR << TileManager::tileManager.saveTiles();*/

    saveDisplay = true;
}

void editInput(SDL_Event& event, bool& isRunning)
{
    if (event.type == SDL_KEYDOWN)
    {
        if (event.key.keysym.sym == SDLK_ESCAPE)
        {
            isRunning = false;
        }
        else if (event.key.keysym.sym == SDLK_1)
        {
            if (editMode->type != EditMode::TILE)
            {
                EditMode* newMode = new TileMode();
                delete editMode;
                editMode = newMode;
            }
        }
        else if (event.key.keysym.sym == SDLK_TAB)
        {
            switchMode();
        }
        else if (event.key.keysym.sym == SDLK_RETURN)
        {
            //save map
            saveMap();
        }
        else if (editMode->type == EditMode::TILE)
        {

            if (event.key.keysym.sym == SDLK_c)
            {
                TileManager::tileManager.replaceTile(0, 5);
            }
        }

        if (state == EDITING)
        {
            if (event.key.keysym.sym == SDLK_r)
            {
                state = RESIZE_MAP;
                typing = true;
            }
        }
    }
    if (event.type == SDL_MOUSEBUTTONDOWN)
    {
        if (editMode->type == EditMode::TILE)
        {
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                leftHeld = true;
            }
            else if (event.button.button == SDL_BUTTON_RIGHT)
            {
                rightHeld = true;
            }
        }
        else
        {
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                editMode->leftClick(mousePosition.x, mousePosition.y);
            }

            if (event.button.button == SDL_BUTTON_RIGHT)
            {
                editMode->rightClick(mousePosition.x, mousePosition.y);
            }
        }
    }
    else if (event.type == SDL_MOUSEBUTTONUP)
    {
        if (event.button.button == SDL_BUTTON_LEFT)
        {
            leftHeld = false;
        }
        if (event.button.button == SDL_BUTTON_RIGHT)
        {
            rightHeld = false;
        }
    }
    if (event.type == SDL_MOUSEWHEEL)
    {
        editMode->switchElement(event.wheel.y);
    }
    if (editMode->type == EditMode::TILE)
    {
        if (leftHeld)
        {
            editMode->leftClick(mousePosition.x, mousePosition.y);
        }
        else if (rightHeld)
        {
            editMode->rightClick(mousePosition.x, mousePosition.y);
        }
    }
}


void switchMode()
{
    EditMode* newMode;
    if (editMode->type == EditMode::TILE)
    {
        newMode = new EnemyMode();
    }
    else
    {
        newMode = new TileMode();
    }

    delete editMode;
    editMode = newMode;
    displayObject.dimensions = glm::vec2(TILE_SIZE, TILE_SIZE);
}

std::string intToString(int i)
{
    std::stringstream s;
    s << i;
    return s.str();
}

void Draw()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (state != State::NEW_MAP && state != State::MAIN_MENU)
    {
        TileManager::tileManager.showTiles(Renderer, camera);

        ResourceManager::GetShader("sprite").Use().SetMatrix4("projection", camera.getCameraMatrix());

        Texture2D texture = ResourceManager::GetTexture("sprites");
        for (auto& iter : objects)
        {
            Renderer->setUVs(iter.second.uvs);
            Renderer->DrawSprite(texture, iter.second.position, 0.0f, iter.second.dimensions);
        }

        if (editMode->type == EditMode::TILE)
        {
            Renderer->setUVs(TileManager::tileManager.uvs[displayObject.type]);
            Texture2D displayTexture = ResourceManager::GetTexture("tiles");
            Renderer->DrawSprite(displayTexture, displayObject.position, 0.0f, displayObject.dimensions);
            Text->RenderText("Tile Mode", SCREEN_WIDTH * 0.5f - TILE_SIZE, 0, 1);

        }
        else
        {
            Renderer->setUVs(displayObject.uvs);
            Texture2D displayTexture = ResourceManager::GetTexture("sprites");
            Renderer->DrawSprite(displayTexture, displayObject.position, 0.0f, displayObject.dimensions);
            if (editMode->type == EditMode::ENEMY)
            {
                Text->RenderText("Enemy Mode", SCREEN_WIDTH * 0.5f, 0, 1);
            }
        }

        Text->RenderText("Current " + intToString(editMode->currentElement), SCREEN_WIDTH * 0.5f + 128, TILE_SIZE, 1);
        Text->RenderText("Max " + intToString(editMode->maxElement), SCREEN_WIDTH * 0.5f + 128, 64, 1);

        Text->RenderText("Position " + intToString(displayObject.position.x), SCREEN_WIDTH * 0.5f + 128, 96, 1);
        Text->RenderText(intToString(displayObject.position.y), SCREEN_WIDTH * 0.5f + 256, 96, 1);


        if (saveDisplay)
        {
            Text->RenderText(mapName + " saved", SCREEN_WIDTH - 200, SCREEN_HEIGHT - TILE_SIZE, 1);
        }

        else if (state == RESIZE_MAP)
        {
            Text->RenderText("X Tiles " + intToString(levelWidth), SCREEN_WIDTH * 0.5f - 64, SCREEN_HEIGHT * 0.5f - 64, 1);
            Text->RenderText("Y Tiles " + intToString(levelHeight), SCREEN_WIDTH * 0.5f + 64, SCREEN_HEIGHT * 0.5f - 64, 1);

            Text->RenderText(inputText[LEVEL_WIDTH_STRING], SCREEN_WIDTH * 0.5f - 64, SCREEN_HEIGHT * 0.5f, 1);
            Text->RenderText(inputText[LEVEL_HEIGHT_STRING], SCREEN_WIDTH * 0.5f + 64, SCREEN_HEIGHT * 0.5f, 1);
        }
        else if (state == SET_WIDTH)
        {
            Text->RenderText("Tiles wide ", SCREEN_WIDTH * 0.5f - 64, SCREEN_HEIGHT * 0.5f - 64, 1);
            Text->RenderText(inputText[LEVEL_WIDTH_STRING], SCREEN_WIDTH * 0.5f - 64, SCREEN_HEIGHT * 0.5f, 1);
        }
    }
    else
    {
        if (state == State::MAIN_MENU)
        {
            std::string action = "";
            if (loading)
            {
                action = "Loading";
            }
            else
            {
                action = "New Map";
            }
            Text->RenderText(action, SCREEN_WIDTH * 0.5f, SCREEN_WIDTH * 0.5f - 64, 1);
            Text->RenderText(inputText[0], SCREEN_WIDTH * 0.5f, SCREEN_WIDTH * 0.5f, 1);
        }
        else
        {
            Text->RenderText("X Tiles", SCREEN_WIDTH * 0.5f - 64, SCREEN_HEIGHT * 0.5f - 64, 1);
            Text->RenderText("Y Tiles", SCREEN_WIDTH * 0.5f + 64, SCREEN_HEIGHT * 0.5f - 64, 1);

            Text->RenderText(inputText[LEVEL_WIDTH_STRING], SCREEN_WIDTH * 0.5f - 64, SCREEN_HEIGHT * 0.5f, 1);
            Text->RenderText(inputText[LEVEL_HEIGHT_STRING], SCREEN_WIDTH * 0.5f + 64, SCREEN_HEIGHT * 0.5f, 1);
        }
    }

    SDL_GL_SwapWindow(window);
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
    currentVPX = vpx;
    currentVPY = vpy;
    currentWidth = aspectWidth;
    currentHeight = aspectHeight;
    glViewport(vpx, vpy, aspectWidth, aspectHeight);
}
