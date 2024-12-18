
#include <string>
#include <cstdio>
#include <iostream>

#include <GL/glew.h>

#include "..\ResourceManager.h"
#include "..\SBatch.h"
#include "SpriteRenderer.h"
#include "Camera.h"
#include "Timing.h"
#include "TextRenderer.h"

#include "TileManager.h"

#include "../InputManager.h"
#include "MenuManager.h"
#include "PlacementModes.h"
#include "../SceneActions.h"
#include "../Vendor.h"

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

#include <string> 

#include <fstream>  
#include <nlohmann/json.hpp>
using json = nlohmann::json;

void init();
bool loadMap();
void saveMap();

void editInput(SDL_Event& event, bool& isRunning);
void switchMode();
void Draw();
void resizeWindow(int width, int height);

enum State { MAIN_MENU, NEW_MAP, EDITING, SET_BACKGROUND, RESIZE_MAP, SET_WIDTH, SCENE_EDITING };

State state = MAIN_MENU;

InputManager inputManager;

bool typing = true;
bool leftHeld = false;
bool rightHeld = false;

std::string bg;

const static int TILE_SIZE = 16;
const static int ENEMY_WRITE = 1;

std::unordered_map<glm::vec2, Object, vec2Hash2> objects;
std::unordered_map<glm::vec2, std::string, vec2Hash2> objectStrings;
std::unordered_map<glm::vec2, int, vec2Hash2> objectWriteTypes;

std::unordered_map<glm::vec2, Vendor, vec2Hash2> vendors;

std::vector<std::vector<glm::vec4>> enemyUVs;

Object displayObject;

int numberOfEnemies = 0;
int numberOfStarts = 0;
int numberOfVendors = 0;

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

std::vector<std::string> classNames;

std::vector<SceneObjects*> sceneObjects;
std::vector<VisitObjects> visitObjects;

//UnitID, game over dialogueID
std::vector<std::pair<int, int>> requiredUnits;

glm::ivec2 enemyEscapePoint;
glm::ivec2 seizePoint;

SBatch Batch;
int idleFrame = 0;
int idleAnimationDirection = 1;
float timeForFrame = 0.0f;

float testFrame = 0;

int main(int argc, char** argv)
{
    init();
    Batch.init();

    inputText.resize(2);

    std::ifstream f("../BaseStats.json");
    json data = json::parse(f);
    json enemyData = data["classes"];

    classNames.resize(enemyData.size());
    int currentName = 0;
    for (const auto& enemy : enemyData) {
        classNames[currentName] = enemy["Name"];
        currentName++;
    }

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

    ResourceManager::LoadShader("Shaders/spriteVertexShader.txt", "Shaders/spriteFragmentShader.txt", nullptr, "spriteTiles");
    ResourceManager::LoadShader("E:/Damon/dev stuff/FE5Test/Shaders/spriteVertexShader.txt", "E:/Damon/dev stuff/FE5Test/Shaders/spriteFragmentShader.txt", nullptr, "sprite");

    ResourceManager::LoadShader("Shaders/shapeVertexShader.txt", "Shaders/shapeFragmentShader.txt", nullptr, "shape");

   // ResourceManager::LoadTexture("spritesheet.png", "sprites");
    ResourceManager::LoadTexture("E:/Damon/dev stuff/FE5Test/Textures/Tiles.png", "tiles");
    ResourceManager::LoadTexture2("E:/Damon/dev stuff/FE5Test/Textures/sprites.png", "sprites");
    ResourceManager::LoadTexture("E:/Damon/dev stuff/FE5Test/Textures/palette.png", "palette");
    //E:\Damon\dev stuff\FE5Test\Textures

    ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera.getCameraMatrix());
    ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);

    ResourceManager::GetShader("sprite").Use().SetInteger("image", 0);
    ResourceManager::GetShader("sprite").Use().SetInteger("palette", 1);
    ResourceManager::GetShader("sprite").SetMatrix4("projection", camera.getCameraMatrix());

    ResourceManager::GetShader("spriteTiles").Use().SetInteger("image", 0);
    ResourceManager::GetShader("spriteTiles").SetMatrix4("projection", camera.getCameraMatrix());

    Shader myShader;
    myShader = ResourceManager::GetShader("spriteTiles");
    Renderer = new SpriteRenderer(myShader);

    enemyUVs.resize(3);
    enemyUVs[0] = ResourceManager::GetTexture("sprites").GetUVs(0, 16, TileManager::TILE_SIZE, TileManager::TILE_SIZE, 3, 1);
    enemyUVs[1] = ResourceManager::GetTexture("sprites").GetUVs(48, 48, TileManager::TILE_SIZE, TileManager::TILE_SIZE, 3, 1);
    enemyUVs[2] = ResourceManager::GetTexture("sprites").GetUVs(96, 0, TileManager::TILE_SIZE, TileManager::TILE_SIZE, 3, 1);

    editMode = new TileMode(&displayObject);

    Text = new TextRenderer(800, 600);
    Text->Load("fonts\\Teko-Light.TTF", 30);
    MenuManager::menuManager.SetUp(Text, &camera, shapeVAO, Renderer);

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
        inputManager.update(deltaTime);

        while (SDL_PollEvent(&event) != 0)
        {
            //User requests quit
            if (event.type == SDL_QUIT)
            {
                isRunning = false;
            }
            else if (event.type == SDL_KEYDOWN)
            {
                inputManager.pressKey(event.key.keysym.sym);
            }                      
            else if (event.type == SDL_KEYUP)
            {
                inputManager.releaseKey(event.key.keysym.sym);
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
        if (inputManager.isKeyPressed(SDLK_ESCAPE))
        {
            isRunning = false;
        }

        if (MenuManager::menuManager.menus.size() > 0)
        {
            MenuManager::menuManager.menus.back()->CheckInput(inputManager, deltaTime);
        }
        else if (!typing)
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

        camera.update();
        Draw();
        fps = fpsLimiter.end();
    }

    for (int i = 0; i < sceneObjects.size(); i++)
    {
        delete sceneObjects[i];
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

    enemyEscapePoint = glm::ivec2(-16);
    seizePoint = glm::ivec2(-16);

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
                int level;
                int growthID;
                int inventorySize;
                std::vector<int> inventory;
                map >> type >> position.x >> position.y >> level >> growthID >> inventorySize;

                Object tObject;
                tObject.position = position;
                tObject.type = type;
                tObject.uvs = enemyUVs[type];
                tObject.level = level;
                tObject.growthRateID = growthID;

                inventory.resize(inventorySize);
                for (int i = 0; i < inventorySize; i++)
                {
                    map >> inventory[i];
                }
                tObject.inventory = inventory;

                int editedStats;
                map >> editedStats;
                tObject.editedStats = editedStats;

                if (editedStats)
                {
                    tObject.stats.resize(9);
                    for (int i = 0; i < 9; i++)
                    {
                        map >> tObject.stats[i];
                    }
                }

                int editedProfs;
                map >> editedProfs;
                tObject.editedProfs = editedProfs;
                if (editedProfs)
                {
                    tObject.profs.resize(10);
                    for (int i = 0; i < 10; i++)
                    {
                        map >> tObject.profs[i];
                    }
                }

                map >> tObject.activationType >> tObject.stationary >> tObject.bossBonus;

                map >> tObject.sceneID;

                std::stringstream stream;
                stream << type << " " << position.x << " " << position.y << " " << level << " " << growthID << " " << inventorySize;
                for (int i = 0; i < inventorySize; i++)
                {
                    stream << " " << inventory[i];
                }
                stream << " " << editedStats;
                if (editedStats)
                {
                    for (int i = 0; i < 9; i++)
                    {
                        stream << " " << tObject.stats[i];
                    }
                }
                stream << " " << editedProfs;
                if (editedProfs)
                {
                    for (int i = 0; i < 10; i++)
                    {
                        stream << " " << tObject.profs[i];
                    }
                }
                stream << " " << tObject.activationType << " " << tObject.stationary << " " << tObject.bossBonus;

                stream << " " << tObject.sceneID;

                objects[position] = tObject;
                objectWriteTypes[position] = ENEMY_WRITE;
                objectStrings[position] = stream.str();
            }
        }
        else if (thing == "Starts")
        {
            map >> numberOfStarts;
            for (size_t i = 0; i < numberOfStarts; i++)
            {
                Object tObject;
                tObject.sprite = false;
                map >> tObject.type >> tObject.position.x >> tObject.position.y;

                std::stringstream stream;
                stream << tObject.type << " " << tObject.position.x << " " << tObject.position.y;

                objects[tObject.position] = tObject;
                objectWriteTypes[tObject.position] = 3;
                objectStrings[tObject.position] = stream.str();
            }
        }
        else if (thing == "Scenes")
        {
            int numberOfScenes = 0;
            map >> numberOfScenes;
            sceneObjects.resize(numberOfScenes);
            for (int i = 0; i < numberOfScenes; i++)
            {
                int numberOfActions = 0;
                map >> numberOfActions;
                sceneObjects[i] = new SceneObjects;
                auto currentObject = sceneObjects[i];
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
                        int itemID;
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
                        int delay;
                        map >> delay;
                        currentObject->actions[c] = new StopMusic(actionType, delay);
                    }
                    else if (actionType == SHOW_MAP_TITLE)
                    {
                        int delay;
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
                    currentObject->activation = new TalkActivation(activationType, talker, listener);
                }
                else if (activationType == 1)
                {
                    int round = 0;
                    map >> round;
                    currentObject->activation = new EnemyTurnEnd(activationType, round);
                }
                else if(activationType == 2)
                {
                    currentObject->activation = new Activation(2);
                }
                else if (activationType == 3)
                {
                    currentObject->activation = new Activation(3);
                }
                else
                {
                    currentObject->activation = new Activation(4);
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
                map >> visitObjects[i].position.x >> visitObjects[i].position.y;
                int numberOfIDs = 0;
                map >> numberOfIDs;
                for (int c = 0; c < numberOfIDs; c++)
                {
                    int unitID = 0;
                    int sceneID = 0;
                    map >> unitID >> sceneID;
                    visitObjects[i].sceneMap[unitID] = sceneID;
                    sceneObjects[sceneID]->inUse = true;
                }
            }
        }
        else if (thing == "Vendors")
        {
            int numberOfVendors = 0;
            map >> numberOfVendors;
            vendors.reserve(numberOfVendors);
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
                vendors[position] = Vendor{ items };
            }
        }
        else if (thing == "EnemyEscape")
        {
            int x = 0;
            int y = 0;
            map >> x >> y;
            enemyEscapePoint = glm::ivec2(x, y);
        }
        else if (thing == "Requirements")
        {
            int units = 0;
            map >> units;
            requiredUnits.resize(units);
            for (int i = 0; i < units; i++)
            {
                int ID;
                int gameOverID;
                map >> ID >> gameOverID;
                requiredUnits[i] = std::pair<int, int>(ID, gameOverID);
            }
        }
        else if (thing == "Seize")
        {
            int x = 0;
            int y = 0;
            map >> x >> y;
            seizePoint = glm::ivec2(x, y);
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
    std::string starts = "Starts\n";
    std::string vendorString = "Vendors\n";
    std::string eEscapeString = "EnemyEscape\n";
    std::string requiredUnitsString = "Requirements\n";
    std::string seizeString = "Seize\n";
    enemies += intToString(numberOfEnemies);

    starts += intToString(numberOfStarts);

    for (auto& iter : objectWriteTypes)
    {
        if (iter.second == ENEMY_WRITE)
        {
            enemies += "\n" + objectStrings[iter.first];
        }
        else if (iter.second == 3)
        {
            starts += "\n" + objectStrings[iter.first]; //I probably do actually need these ordered for how I am reading them right now
        }
    }
    vendorString += intToString(vendors.size());
    for (auto& iter : vendors)
    {
        vendorString += "\n" + intToString(iter.first.x) + " " + intToString(iter.first.y) + " " + intToString(iter.second.items.size());
        for (int i = 0; i < iter.second.items.size(); i++)
        {
            vendorString += " " + intToString(iter.second.items[i]);
        }
    }
    eEscapeString += intToString(enemyEscapePoint.x) + " " + intToString(enemyEscapePoint.y);
    seizeString += intToString(seizePoint.x) + " " + intToString(seizePoint.y);
    requiredUnitsString += intToString(requiredUnits.size());
    for (int i = 0; i < requiredUnits.size(); i++)
    {
        requiredUnitsString += "\n" + intToString(requiredUnits[i].first) + " " + intToString(requiredUnits[i].second);
    }

    std::string scenes = "Scenes\n";
    scenes += intToString(sceneObjects.size());
    for (int i = 0; i < sceneObjects.size(); i++)
    {
        auto currentObject = sceneObjects[i];
        scenes += "\n" + intToString(currentObject->actions.size()) + " ";
        for (int c = 0; c < currentObject->actions.size(); c++)
        {
            auto currentAction = currentObject->actions[c];
            scenes += intToString(currentAction->type) + " ";
            if (currentAction->type == CAMERA_ACTION)
            {
                auto action = static_cast<CameraMove*>(currentAction);
                scenes += intToString(action->position.x) + " " + intToString(action->position.y) + " ";
            }
            else if (currentAction->type == NEW_UNIT_ACTION)
            {
                auto action = static_cast<AddUnit*>(currentAction);
                scenes += intToString(action->unitID) + " " + intToString(action->start.x) + " " + intToString(action->start.y) + " " + intToString(action->end.x) + " " + intToString(action->end.y) + " ";
            }
            else if (currentAction->type == MOVE_UNIT_ACTION)
            {
                auto action = static_cast<UnitMove*>(currentAction);
                scenes += intToString(action->unitID) + " " + intToString(action->end.x) + " " + intToString(action->end.y) + " ";
            }
            else if (currentAction->type == DIALOGUE_ACTION)
            {
                auto action = static_cast<DialogueAction*>(currentAction);
                scenes += intToString(action->ID) + " ";
            }
            else if (currentAction->type == ITEM_ACTION)
            {
                auto action = static_cast<ItemAction*>(currentAction);
                scenes += intToString(action->ID) + " ";
            }
            else if (currentAction->type == NEW_SCENE_UNIT_ACTION)
            {
                auto action = static_cast<AddSceneUnit*>(currentAction);
                scenes += intToString(action->unitID) + " " + intToString(action->team) + " " + intToString(action->path.size()) + " ";
                for (int i = 0; i < action->path.size(); i++)
                {
                    scenes += intToString(action->path[i].x) + " " + intToString(action->path[i].y) + " ";
                }
                scenes += floatToString(action->nextActionDelay) + " " + floatToString(action->nextMoveDelay) + " ";
            }
            else if (currentAction->type == SCENE_UNIT_MOVE_ACTION)
            {
                auto action = static_cast<SceneUnitMove*>(currentAction);
                scenes += intToString(action->unitID) + " " + intToString(action->path.size()) + " ";
                for (int i = 0; i < action->path.size(); i++)
                {
                    scenes += intToString(action->path[i].x) + " " + intToString(action->path[i].y) + " ";
                }
                scenes += floatToString(action->nextActionDelay) + " " + floatToString(action->moveSpeed) + " " + intToString(action->facing) + " ";
            }
            else if (currentAction->type == SCENE_UNIT_REMOVE_ACTION)
            {
                auto action = static_cast<SceneUnitRemove*>(currentAction);
                scenes += intToString(action->unitID) + " " + floatToString(action->nextActionDelay) + " ";
            }
            else if (currentAction->type == START_MUSIC)
            {
                auto action = static_cast<StartMusic*>(currentAction);
                scenes += intToString(action->ID) + " ";
            }
            else if (currentAction->type == STOP_MUSIC)
            {
                auto action = static_cast<StopMusic*>(currentAction);
                scenes += intToString(action->nextActionDelay) + " ";
            }
            else if (currentAction->type == SHOW_MAP_TITLE)
            {
                scenes += intToString(currentAction->nextActionDelay) + " ";
            }
        }
        scenes += intToString(currentObject->activation->type) + " ";
        if (currentObject->activation->type == SceneActivationMenu::TALK)
        {
            auto activation = static_cast<TalkActivation*>(currentObject->activation);
            scenes += intToString(activation->talker) + " " + intToString(activation->listener) + " ";
        }
        if (currentObject->activation->type == SceneActivationMenu::ENEMY_TURN_END)
        {
            auto activation = static_cast<EnemyTurnEnd*>(currentObject->activation);
            scenes += intToString(activation->round) + " ";
        }
        scenes += intToString(currentObject->repeat) + " ";
    }
    std::string visit = "Visits\n";
    visit += intToString(visitObjects.size());
    for (int i = 0; i < visitObjects.size(); i++)
    {
        auto currentVisit = visitObjects[i];
        visit += "\n" + intToString(currentVisit.position.x) + " " + intToString(currentVisit.position.y) + " " + intToString(currentVisit.sceneMap.size()) + " ";

        for (const auto& pair : currentVisit.sceneMap)
        {
            visit += intToString(pair.first) + " " + intToString(pair.second) + " ";
        }
    }

    map << enemies << "\n" << starts << "\n" << scenes << "\n" << visit << "\n" << vendorString << "\n" << eEscapeString << "\n" << requiredUnitsString << "\n" << seizeString << "\n";

    map.close();

    //save to game folder
    std::ofstream mapP("E:\\Damon\\dev stuff\\FE5Test\\Levels\\" + mapName);
    mapP << "Level\n";
    mapP << TileManager::tileManager.saveTiles();
    mapP << "\n";
    mapP << enemies << "\n" << starts << "\n" << scenes << "\n" << visit << "\n" << vendorString << "\n" << eEscapeString << "\n" << requiredUnitsString << "\n" << seizeString << "\n";
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

//Use input manager for this later
void editInput(SDL_Event& event, bool& isRunning)
{
    if (MenuManager::menuManager.menus.size() == 0)
    {
        if (editMode->type == EditMode::TILE)
        {
            if (inputManager.isKeyPressed(SDLK_1))
            {
                editMode->currentElement = 0;
            }
            else if (inputManager.isKeyPressed(SDLK_2))
            {
                editMode->currentElement = 75;
            }
            else if (inputManager.isKeyPressed(SDLK_3))
            {
                editMode->currentElement = 83;
            }
            else if (inputManager.isKeyPressed(SDLK_4))
            {
                editMode->currentElement = 93;
            }
            else if (inputManager.isKeyPressed(SDLK_5))
            {
                editMode->currentElement = 110;
            }
            else if (inputManager.isKeyPressed(SDLK_6))
            {
                editMode->currentElement = 125;
            }
            else if (inputManager.isKeyPressed(SDLK_7))
            {
                editMode->currentElement = 153;
            }
            else if (inputManager.isKeyPressed(SDLK_8))
            {
                editMode->currentElement = 181;
            }
            editMode->dObject->type = editMode->currentElement;
        }
        if (event.type == SDL_KEYDOWN)
        {
            if (event.key.keysym.sym == SDLK_TAB)
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
                else if (inputManager.isKeyPressed(SDLK_s))
                {
                    MenuManager::menuManager.OpenSceneMenu(sceneObjects, visitObjects);
                }
                else if (inputManager.isKeyPressed(SDLK_n))
                {
                    MenuManager::menuManager.OpenRequirementsMenu(requiredUnits);
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
}


void switchMode()
{
    EditMode* newMode;
    if (editMode->type == EditMode::TILE)
    {
        newMode = new EnemyMode(&displayObject, objects, objectStrings, objectWriteTypes, numberOfEnemies, enemyUVs);
    }
    else if (editMode->type == EditMode::ENEMY)
    {
        newMode = new PlayerStartMode(&displayObject, objects, objectStrings, objectWriteTypes, numberOfStarts);
    }
    else if (editMode->type == EditMode::STARTS)
    {
        newMode = new VendorMode(&displayObject, vendors, numberOfVendors);
    }
    else if (editMode->type == EditMode::VENDORS)
    {
        newMode = new EnemyEscapeMode(&displayObject, enemyEscapePoint);
    }
    else if (editMode->type == EditMode::ENEMY_ESCAPE)
    {
        newMode = new SeizeMode(&displayObject, seizePoint);
    }
    else
    {
        newMode = new TileMode(&displayObject);
    }

    delete editMode;
    editMode = newMode;
    displayObject.dimensions = glm::vec2(TILE_SIZE, TILE_SIZE);
}

void Draw()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (state != State::NEW_MAP && state != State::MAIN_MENU)
    {
        ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera.getCameraMatrix());

        ResourceManager::GetShader("spriteTiles").Use().SetMatrix4("projection", camera.getCameraMatrix());

        TileManager::tileManager.showTiles(Renderer, camera);

        ResourceManager::GetShader("sprite").Use().SetMatrix4("projection", camera.getCameraMatrix());
        Batch.begin();

        //Should separate objects into units and other things since this is getting ridiculous
        for (auto& iter : objects)
        {
            if (iter.second.sprite)
            {
                //   Renderer->setUVs(iter.second.uvs);
                 //  Renderer->DrawSprite(texture, iter.second.position, 0.0f, iter.second.dimensions);
                Texture2D texture = ResourceManager::GetTexture("sprites");
                glm::vec4 colorAndAlpha = glm::vec4(1);
                glm::vec2 position = iter.second.position;
                glm::vec2 size;

                size = glm::vec2(16);
                //  position += sprite.drawOffset;

                Batch.addToBatch(texture.ID, position, size, colorAndAlpha, 0, false, 1, iter.second.uvs[idleFrame]);
            }
            else
            {
                ResourceManager::GetShader("shape").Use().SetFloat("alpha", 0.5f);
                glm::mat4 model = glm::mat4();
                model = glm::translate(model, glm::vec3(iter.second.position, 0.0f));

                model = glm::scale(model, glm::vec3(16, 16, 0.0f));

                ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.5f, 1.0f));

                ResourceManager::GetShader("shape").SetMatrix4("model", model);
                glBindVertexArray(shapeVAO);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                glBindVertexArray(0);

                glm::vec2 drawPosition = glm::vec2(iter.second.position) + glm::vec2(4);
                drawPosition = camera.worldToRealScreen(drawPosition, SCREEN_WIDTH, SCREEN_HEIGHT);
                Text->RenderText(intToString(iter.second.type), drawPosition.x, drawPosition.y, 1, glm::vec3(0.0f));
            }
        }
        if (editMode->type == EditMode::ENEMY)
        {
            Texture2D displayTexture = ResourceManager::GetTexture("sprites");
            Batch.addToBatch(displayTexture.ID, displayObject.position, displayObject.dimensions, glm::vec4(1), 0, false, 1, displayObject.uvs[idleFrame]);
        }
        Batch.end();
        Batch.renderBatch();
        for (auto& iter : vendors)
        {
            ResourceManager::GetShader("shape").Use().SetFloat("alpha", 0.5f);
            glm::mat4 model = glm::mat4();
            model = glm::translate(model, glm::vec3(iter.first, 0.0f));

            model = glm::scale(model, glm::vec3(16, 16, 0.0f));

            ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(1.0f, 0.5f, 0.0f));

            ResourceManager::GetShader("shape").SetMatrix4("model", model);
            glBindVertexArray(shapeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);

            glm::vec2 drawPosition = glm::vec2(iter.first) + glm::vec2(4);
            drawPosition = camera.worldToRealScreen(drawPosition, SCREEN_WIDTH, SCREEN_HEIGHT);
        }

        ResourceManager::GetShader("shape").Use().SetFloat("alpha", 0.5f);
        glm::mat4 model = glm::mat4();
        model = glm::translate(model, glm::vec3(enemyEscapePoint, 0.0f));

        model = glm::scale(model, glm::vec3(16, 16, 0.0f));

        ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(1.0f, 0.2f, 0.0f));

        ResourceManager::GetShader("shape").SetMatrix4("model", model);
        glBindVertexArray(shapeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        model = glm::mat4();
        model = glm::translate(model, glm::vec3(seizePoint, 0.0f));

        model = glm::scale(model, glm::vec3(16, 16, 0.0f));

        ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(1.0f, 0.0f, 1.0f));

        ResourceManager::GetShader("shape").SetMatrix4("model", model);
        glBindVertexArray(shapeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        if (MenuManager::menuManager.menus.size() > 0)
        {
            auto menu = MenuManager::menuManager.menus.back();
            menu->Draw();
        }
        else
        {
            if (editMode->type == EditMode::TILE)
            {
                Renderer->setUVs(TileManager::tileManager.uvs[displayObject.type]);
                Texture2D displayTexture = ResourceManager::GetTexture("tiles");
                Renderer->DrawSprite(displayTexture, displayObject.position, 0.0f, displayObject.dimensions);
                Text->RenderText("Tile Mode", SCREEN_WIDTH * 0.5f - TILE_SIZE, 0, 1);
                Text->RenderText("Current: " + intToString(editMode->currentElement), SCREEN_WIDTH * 0.5f + 128, TILE_SIZE, 1);
                std::string tileType;
                if (editMode->currentElement < 70)
                {
                    tileType = "Plains";
                }
                else if (editMode->currentElement < 78)
                {
                    tileType = "Forest";
                }
                else if (editMode->currentElement < 89)
                {
                    tileType = "Thicket";
                }
                else if (editMode->currentElement < 108)
                {
                    tileType = "Road";
                }
                else if (editMode->currentElement < 124)
                {
                    tileType = "Cliff";
                }
                else if (editMode->currentElement < 154)
                {
                    tileType = "Peak";
                }
                else if (editMode->currentElement < 180)
                {
                    tileType = "---";
                }
                else if (editMode->currentElement < 182)
                {
                    tileType = "Gate";
                }
                else if (editMode->currentElement < 183)
                {
                    tileType = "Vendor";
                }
                else if (editMode->currentElement < 184)
                {
                    tileType = "House";
                }
                Text->RenderText(tileType, SCREEN_WIDTH * 0.5f + 128, 42, 1);

            }
            else if (editMode->type == EditMode::ENEMY)
            {
                Text->RenderText("Enemy Mode", SCREEN_WIDTH * 0.5f, 0, 1);
                Text->RenderText("Current: " + classNames[editMode->currentElement], SCREEN_WIDTH * 0.5f + 128, TILE_SIZE, 1);
            }
            else if (editMode->type == EditMode::STARTS)
            {
                Text->RenderText("Starts Mode", SCREEN_WIDTH * 0.5f - TILE_SIZE, 0, 1);

                ResourceManager::GetShader("shape").Use().SetFloat("alpha", 0.5f);
                glm::mat4 model = glm::mat4();
                model = glm::translate(model, glm::vec3(displayObject.position, 0.0f));

                model = glm::scale(model, glm::vec3(16, 16, 0.0f));

                ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.5f, 1.0f));

                ResourceManager::GetShader("shape").SetMatrix4("model", model);
                glBindVertexArray(shapeVAO);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                glBindVertexArray(0);

                glm::vec2 drawPosition = glm::vec2(displayObject.position) + glm::vec2(4);
                drawPosition = camera.worldToRealScreen(drawPosition, SCREEN_WIDTH, SCREEN_HEIGHT);
                Text->RenderText(intToString(editMode->currentElement), drawPosition.x, drawPosition.y, 1, glm::vec3(0.0f));
            }
            else if (editMode->type == EditMode::VENDORS)
            {
                Text->RenderText("Vendors Mode", SCREEN_WIDTH * 0.5f - TILE_SIZE, 0, 1);

                ResourceManager::GetShader("shape").Use().SetFloat("alpha", 0.5f);
                glm::mat4 model = glm::mat4();
                model = glm::translate(model, glm::vec3(displayObject.position, 0.0f));

                model = glm::scale(model, glm::vec3(16, 16, 0.0f));

                ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(1.0f, 0.5f, 0.0f));

                ResourceManager::GetShader("shape").SetMatrix4("model", model);
                glBindVertexArray(shapeVAO);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                glBindVertexArray(0);

                glm::vec2 drawPosition = glm::vec2(displayObject.position) + glm::vec2(4);
                drawPosition = camera.worldToRealScreen(drawPosition, SCREEN_WIDTH, SCREEN_HEIGHT);
            }
            else if (editMode->type == EditMode::ENEMY_ESCAPE)
            {
                Text->RenderText("Enemy Escape Mode", SCREEN_WIDTH * 0.5f - TILE_SIZE, 0, 1);

                ResourceManager::GetShader("shape").Use().SetFloat("alpha", 0.5f);
                glm::mat4 model = glm::mat4();
                model = glm::translate(model, glm::vec3(displayObject.position, 0.0f));

                model = glm::scale(model, glm::vec3(16, 16, 0.0f));

                ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(1.0f, 0.2f, 0.0f));

                ResourceManager::GetShader("shape").SetMatrix4("model", model);
                glBindVertexArray(shapeVAO);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                glBindVertexArray(0);

                glm::vec2 drawPosition = glm::vec2(displayObject.position) + glm::vec2(4);
                drawPosition = camera.worldToRealScreen(drawPosition, SCREEN_WIDTH, SCREEN_HEIGHT);
            }
            else if (editMode->type == EditMode::SEIZE)
            {
                Text->RenderText("Seize Mode", SCREEN_WIDTH * 0.5f - TILE_SIZE, 0, 1);

                ResourceManager::GetShader("shape").Use().SetFloat("alpha", 0.5f);
                glm::mat4 model = glm::mat4();
                model = glm::translate(model, glm::vec3(displayObject.position, 0.0f));

                model = glm::scale(model, glm::vec3(16, 16, 0.0f));

                ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(1.0f, 0.0f, 1.0f));

                ResourceManager::GetShader("shape").SetMatrix4("model", model);
                glBindVertexArray(shapeVAO);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                glBindVertexArray(0);

                glm::vec2 drawPosition = glm::vec2(displayObject.position) + glm::vec2(4);
                drawPosition = camera.worldToRealScreen(drawPosition, SCREEN_WIDTH, SCREEN_HEIGHT);
            }
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
