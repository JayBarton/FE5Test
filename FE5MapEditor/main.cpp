
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

#include "../InputManager.h"
#include "MenuManager.h"
#include "PlacementModes.h"
#include "../SceneActions.h"

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

std::vector<glm::vec4> enemyUVs;

Object displayObject;

int numberOfEnemies = 0;
int numberOfStarts = 0;

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

int main(int argc, char** argv)
{
    init();

    inputText.resize(2);

    std::ifstream f("../BaseStats.json");
    json data = json::parse(f);
    json enemyData = data["enemies"];

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

    ResourceManager::LoadShader("Shaders/spriteVertexShader.txt", "Shaders/spriteFragmentShader.txt", nullptr, "sprite");
    ResourceManager::LoadShader("Shaders/shapeVertexShader.txt", "Shaders/shapeFragmentShader.txt", nullptr, "shape");

   // ResourceManager::LoadTexture("spritesheet.png", "sprites");
    ResourceManager::LoadTexture("E:/Damon/dev stuff/FE5Test/TestSprites/tilesheet2.png", "tiles");
    ResourceManager::LoadTexture("E:/Damon/dev stuff/FE5Test/TestSprites/sprites.png", "sprites");

    ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera.getCameraMatrix());
    ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);

    ResourceManager::GetShader("sprite").Use().SetInteger("image", 0);
    ResourceManager::GetShader("sprite").SetMatrix4("projection", camera.getCameraMatrix());

    Shader myShader;
    myShader = ResourceManager::GetShader("sprite");
    Renderer = new SpriteRenderer(myShader);

    enemyUVs = ResourceManager::GetTexture("sprites").GetUVs(TILE_SIZE, TILE_SIZE);

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
                }
                int activationType = 0;
                map >> activationType;
                if (activationType == 1)
                {
                    int round = 0;
                    map >> round;
                    currentObject->activation = new EnemyTurnEnd(activationType, round);
                }
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
    std::string starts = "Starts\n";
    std::stringstream stream;
    stream << numberOfEnemies;
    enemies += stream.str();

    std::stringstream stream2;
    stream2 << numberOfStarts;
    starts += stream2.str();

    for (auto& iter : objectWriteTypes)
    {
        if (iter.second == ENEMY_WRITE)
        {
            enemies += "\n" + objectStrings[iter.first];
        }
        else if (iter.second == 3)
        {
            starts += "\n" + objectStrings[iter.first];
        }
    }
    std::string scenes = "Scenes\n";
    std::stringstream stream3;
    stream3 << sceneObjects.size();
    scenes += stream3.str();
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
        }
        scenes += intToString(currentObject->activation->type) + " ";
        if (currentObject->activation->type == 1)
        {
            auto activation = static_cast<EnemyTurnEnd*>(currentObject->activation);
            scenes += intToString(activation->round) + " ";
        }
    }

    map << enemies << "\n" << starts << "\n" << scenes << "\n";

    map.close();

    //save to game folder
    std::ofstream mapP("E:\\Damon\\dev stuff\\FE5Test\\Levels\\" + mapName);
    mapP << "Level\n";
    mapP << TileManager::tileManager.saveTiles();
    mapP << "\n";
    mapP << enemies << "\n" << starts << "\n" << scenes << "\n";
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
        if (event.type == SDL_KEYDOWN)
        {
            if (event.key.keysym.sym == SDLK_1)
            {
                if (editMode->type != EditMode::TILE)
                {
                    EditMode* newMode = new TileMode(&displayObject);
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
                else if (inputManager.isKeyPressed(SDLK_s))
                {
                    MenuManager::menuManager.OpenSceneMenu(sceneObjects);
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
        TileManager::tileManager.showTiles(Renderer, camera);

        ResourceManager::GetShader("sprite").Use().SetMatrix4("projection", camera.getCameraMatrix());

        Texture2D texture = ResourceManager::GetTexture("sprites");
        int help = 0;
        for (auto& iter : objects)
        {
            if (iter.second.sprite)
            {
                Renderer->setUVs(iter.second.uvs);
                Renderer->DrawSprite(texture, iter.second.position, 0.0f, iter.second.dimensions);
            }
            else
            {
                help++;
                ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera.getCameraMatrix());
                ResourceManager::GetShader("shape").SetFloat("alpha", 0.5f);
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

            }
            else if(editMode->type == EditMode::ENEMY)
            {
                Renderer->setUVs(displayObject.uvs);
                Texture2D displayTexture = ResourceManager::GetTexture("sprites");
                Renderer->DrawSprite(displayTexture, displayObject.position, 0.0f, displayObject.dimensions);
                if (editMode->type == EditMode::ENEMY)
                {
                    Text->RenderText("Enemy Mode", SCREEN_WIDTH * 0.5f, 0, 1);
                }
                Text->RenderText("Current: " + classNames[editMode->currentElement], SCREEN_WIDTH * 0.5f + 128, TILE_SIZE, 1);
            }
            else
            {
                ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera.getCameraMatrix());
                ResourceManager::GetShader("shape").SetFloat("alpha", 0.5f);
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
