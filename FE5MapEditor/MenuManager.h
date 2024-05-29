#include <vector>
#include "../Unit.h"
#include "../Items.h"
#include "../Vendor.h"

class TextRenderer;
class Camera;
class SpriteRenderer;

struct Activation
{
    int type;
    Activation(int type) : type(type)
    {}
};

struct EnemyTurnEnd : public Activation
{
    int round;

    EnemyTurnEnd(int type, int round) : Activation(type), round(round)
    {}
};

struct TalkActivation : public Activation
{
    int talker;
    int listener;
    TalkActivation(int type, int talker, int listener) : Activation(type), talker(talker), listener(listener)
    {}
};

/*struct VisitActivation : public Activation
{
    glm::ivec2 position;
    int expectedID = -1;
};*/

struct SceneObjects
{
    std::vector<class SceneAction*> actions;
    Activation* activation = nullptr;
    bool repeat = false;
    ~SceneObjects()
    {
        for (int i = 0; i < actions.size(); i++)
        {
            delete actions[i];
        }
        actions.clear();
    }
};

struct VisitObjects
{
    glm::ivec2 position;
    //Key is the ID the unit that activates this visit, the value will be the index in the scene vector of the scene to play
    std::unordered_map<int, int> sceneMap;
};

struct Menu
{
    Menu() {}
    Menu(TextRenderer* Text, Camera* camera, int shapeVAO);
    virtual ~Menu() {}
    virtual void Draw() = 0;
    virtual void SelectOption() = 0;
    virtual void CancelOption();
    virtual void GetOptions() {};
    virtual void CheckInput(class InputManager& inputManager, float deltaTime);
    void ClearMenu();

    TextRenderer* text = nullptr;
    Camera* camera = nullptr;

    int shapeVAO; //Not sure I need this long term, as I will eventually replace shape drawing with sprites

    int currentOption = 0;
    int numberOfOptions = 0;
    std::vector<int> optionsVector;

};

struct MenuManager
{
    void SetUp(TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer);

    void AddMenu(int menuID);

    void AddEnemyMenu(class EnemyMode* mode, class Object* obj);
    void AddInventoryMenu(class EnemyMode* mode, class Object* obj, std::vector<int>& inventory, std::vector<Item>& items);
    void AddStatsMenu(EnemyMode* mode, Object* obj, std::vector<int>& baseStats, bool& editedStats);
    void AddProfsMenu(EnemyMode* mode, Object* obj, std::vector<int>& weaponProfs, bool& editedProfs);
    void AddVendorMenu(class VendorMode* mode, Vendor* vendor);
    void OpenSceneMenu(std::vector<SceneObjects*>& sceneObjects, std::vector<VisitObjects>& visitObjects);
    void OpenActionMenu(SceneObjects& sceneObject);
    void OpenVisitMenu(VisitObjects& visitObject);
    void OpenActivationMenu(SceneObjects& sceneObject);
    void OpenTalkMenu(SceneObjects& sceneObject);
    void SelectOptionMenu(int action, std::vector<SceneAction*>& sceneActions);
    void PreviousMenu();
    void ClearMenu();

    std::vector<Menu*> menus;

    TextRenderer* text = nullptr;
    Camera* camera = nullptr;
    SpriteRenderer* renderer = nullptr;

    int shapeVAO; //Not sure I need this long term, as I will eventually replace shape drawing with sprites

    static MenuManager menuManager;
};

struct EnemyMenu : public Menu
{
    EnemyMenu(TextRenderer* Text, Camera* camera, int shapeVAO, class EnemyMode* mode, class Object* object);
    virtual void Draw() override;
    virtual void SelectOption() override;
    virtual void CheckInput(class InputManager& inputManager, float deltaTime) override;

    const static int CHANGE_LEVEL = 0;
    const static int CHANGE_GROWTH = 1;
    const static int OPEN_INVENTORY = 2;
    const static int CHANGE_STATS = 0;
    const static int CHANGE_PROFS = 1;

    const static int ACTIVATION_OPTION = 0;
    const static int STATIONARY_OPTION = 1;
    const static int BOSS_OPTION = 2;

    const static int PAGE1_LIMIT = 2;

    EnemyMode* mode = nullptr;
    Object* object = nullptr;

    int level = 1;
    int growthRateID = 0;
    int page = 0;
    std::vector<StatGrowths> unitGrowths;
    std::vector<int> baseStats;
    std::vector<int> inventory;
    std::vector<Item> items;
    std::unordered_map<std::string, int> weaponNameMap;
    std::string weaponNamesArray[10];
    std::vector<int> weaponProficiencies;
    int pageOptions[3];

    bool inInventory = false;
    bool editedStats = false;
    bool editedProfs = false;
    bool stationary = false;

    bool bossBonus = false;
    //Could be an enum if I come up with other requirements
    //Current types:
    //0 = within attack range
    //1 = within a WALKING range of attack range + x. X is currently 1. Walking range means the unit has to be able to reach here by walking, so not through walls
    //2 = within attack WALKING range. So even if a range unit can hit the unit through a wall or something, if they cannot walk to the unit, they cannot attack
    int activationType = 0;
    int sceneID = -1;

    std::string growthNames[6];
    std::string className;

};

struct InventoryMenu : public Menu
{
    InventoryMenu(TextRenderer* Text, Camera* camera, int shapeVAO, class EnemyMode* mode, class Object* object, std::vector<int>& inventory, std::vector<Item>& items);
    virtual void Draw() override;
    virtual void SelectOption() override;
    virtual void CheckInput(class InputManager& inputManager, float deltaTime) override;
    virtual void CancelOption() override;

    std::vector<int>& inventory;
    std::vector<Item>& items;
    int currentSlot = -1;
    int currentItem = 0;
};

struct StatsMenu : public Menu
{
    StatsMenu(TextRenderer* Text, Camera* camera, int shapeVAO, class EnemyMode* mode, class Object* object, std::vector<int>& baseStats, bool& editedStats);
    virtual void Draw() override;
    virtual void SelectOption() override;
    virtual void CheckInput(class InputManager& inputManager, float deltaTime) override;

    std::vector<int>& stats;
    bool& edited;
};

struct ProfsMenu : public Menu
{
    ProfsMenu(TextRenderer* Text, Camera* camera, int shapeVAO, class EnemyMode* mode, class Object* object, std::vector<int>& weaponProfs, bool& editedProfs);
    virtual void Draw() override;
    virtual void SelectOption() override;
    virtual void CheckInput(class InputManager& inputManager, float deltaTime) override;

    std::vector<int>& profs;
    bool& edited;
};

struct SceneMenu : public Menu
{
    SceneMenu(TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<SceneObjects*>& sceneObjects, std::vector<VisitObjects>& visitObjects);
    virtual void Draw() override;
    virtual void CheckInput(class InputManager& inputManager, float deltaTime) override;
    virtual void SelectOption() override;

    bool addingScene = true;
    std::vector<SceneObjects*>& sceneObjects;
    std::vector<VisitObjects>& visitObjects;
};

struct VendorMenu : public Menu
{
    VendorMenu(TextRenderer* Text, Camera* camera, int shapeVAO, Vendor* vendor);
    virtual void Draw() override;
    virtual void SelectOption() override;
    virtual void CheckInput(class InputManager& inputManager, float deltaTime) override;

    int currentSlot = -1;
    int currentItem = 0;
    Vendor* vendor;
    std::vector<Item> items;
};

struct VisitMenu : public Menu
{
    VisitMenu(TextRenderer* Text, Camera* camera, int shapeVAO, VisitObjects& visitObject);
    virtual void Draw() override;
    virtual void SelectOption() override;
    virtual void CheckInput(class InputManager& inputManager, float deltaTime) override;
    virtual void CancelOption() override;
    int unitID = -1;
    int sceneID = 0;
    bool settingPosition = true;
    glm::vec2 cameraPosition;
    VisitObjects& visitObject;
};

struct SceneActionMenu : public Menu
{
    SceneActionMenu(TextRenderer* Text, Camera* camera, int shapeVAO, SceneObjects& sceneObject);
    virtual void Draw() override;
    virtual void SelectOption() override;
    virtual void CheckInput(class InputManager& inputManager, float deltaTime) override;
    virtual void CancelOption() override;

    bool activationMode = false;
    int swapAction = -1;
    int selectedAction = 0;
    std::vector<std::string> actionNames;
    SceneObjects& sceneObject;
    std::vector<SceneAction*>& sceneActions;
};

struct SceneActivationMenu : public Menu
{
    const static int TALK = 0;
    const static int ENEMY_TURN_END = 1;
    const static int VISIT = 2;
    SceneActivationMenu(TextRenderer* Text, Camera* camera, int shapeVAO, SceneObjects& sceneObject);
    virtual void Draw() override;
    virtual void SelectOption() override;

    SceneObjects& sceneObject;
};

struct TalkActivationMenu : public Menu
{
    TalkActivationMenu(TextRenderer* Text, Camera* camera, int shapeVAO, SceneObjects& sceneObject);
    virtual void Draw() override;
    virtual void SelectOption() override;
    virtual void CheckInput(class InputManager& inputManager, float deltaTime) override;

    bool setTalker = true;
    //The default initiator of the dialogue
    int talker = 0;
    //Who the talker is talking to
    int listener = 0;

    SceneObjects& sceneObject;
};

struct CameraActionMenu : public Menu
{
    CameraActionMenu(TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<SceneAction*>& sceneActions);
    virtual void Draw() override;
    virtual void SelectOption() override;
    virtual void CheckInput(class InputManager& inputManager, float deltaTime) override;

    glm::vec2 cameraPosition;
    std::vector<SceneAction*>& sceneActions;
};

struct NewUnitActionMenu : public Menu
{
    NewUnitActionMenu(TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<SceneAction*>& sceneActions);
    virtual void Draw() override;
    virtual void SelectOption() override;
    virtual void CheckInput(class InputManager& inputManager, float deltaTime) override;
    virtual void CancelOption() override;
    int unitID;
    bool setStart = true;
    bool finished = false;
    glm::vec2 startPosition;
    glm::vec2 endPosition;
    glm::vec2 cameraPosition;
    std::vector<SceneAction*>& sceneActions;
};

struct MoveUnitActionMenu : public Menu
{
    MoveUnitActionMenu(TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<SceneAction*>& sceneActions);
    virtual void Draw() override;
    virtual void SelectOption() override;
    virtual void CheckInput(class InputManager& inputManager, float deltaTime) override;
    int unitID;
    glm::vec2 endPosition;
    glm::vec2 cameraPosition;
    std::vector<SceneAction*>& sceneActions;
};

struct DialogueActionMenu : public Menu
{
    DialogueActionMenu(TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<SceneAction*>& sceneActions);
    virtual void Draw() override;
    virtual void SelectOption() override;
    virtual void CheckInput(class InputManager& inputManager, float deltaTime) override;
    int dialogueID;
    std::vector<SceneAction*>& sceneActions;
};

struct ItemActionMenu : public Menu
{
    ItemActionMenu(TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<SceneAction*>& sceneActions);
    virtual void Draw() override;
    virtual void SelectOption() override;
    int itemID;
    std::vector<SceneAction*>& sceneActions;
    std::vector<Item> items;
};