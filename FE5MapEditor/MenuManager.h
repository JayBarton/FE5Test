#include <vector>
#include "../Unit.h"
#include "../Items.h"

class TextRenderer;
class Camera;
class SpriteRenderer;

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
    int activationType = 0;

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
    int currentItem = -1;
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