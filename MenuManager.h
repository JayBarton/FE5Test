#pragma once
#include <vector>
#include "Unit.h"
#include "TextAdvancer.h"
class Cursor;
class TextRenderer;
class Camera;
class Item;
class BattleStats;
class SpriteRenderer;
class BattleManager;

struct Menu
{
	Menu() {}
	Menu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO);
	virtual ~Menu() {}
	virtual void Draw() = 0;
	virtual void SelectOption() = 0;
	virtual void CancelOption();
	virtual void GetOptions() {};
	virtual void CheckInput(class InputManager& inputManager, float deltaTime);
	void EndUnitMove();
	void ClearMenu();

	Cursor* cursor = nullptr;
	TextRenderer* text = nullptr;
	Camera* camera = nullptr;

	int shapeVAO; //Not sure I need this long term, as I will eventually replace shape drawing with sprites

	int currentOption = 0;
	int numberOfOptions = 0;
	std::vector<int> optionsVector;

	//If this menu covers the whole screen
	//Used in the main draw call, if it is true, we don't need to draw anything else but the menu
	bool fullScreen = false;

};

struct UnitOptionsMenu : public Menu
{
	UnitOptionsMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO);
	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void CancelOption() override;

	virtual void GetOptions() override;

	const static int ITEMS = 0;
	const static int WAIT = 1;
	const static int DISMOUNT = 2;
	const static int ATTACK = 3;
	const static int TRADE = 4;
	const static int MOUNT = 5;
	const static int CAPTURE = 6;
	const static int DROP = 7;
	const static int RELEASE = 8;
	const static int RESCUE = 9;
	const static int TRANSFER = 10;
	const static int TALK = 11;
	const static int VISIT = 12;
	const static int VENDOR = 13;

	std::vector<Unit*> unitsInRange;
	std::vector<Unit*> unitsInCaptureRange;
	std::vector<Unit*> tradeUnits;
	std::vector<Unit*> talkUnits;
	std::vector<Unit*> rescueUnits;
	std::vector<Unit*> transferUnits;

	std::vector<glm::ivec2> dropPositions;

	bool canAttack = false;
	bool canDismount = false;
	bool canMount = false;
	bool canTrade = false;
	bool canCapture = false;
	bool canRescue = false;
	bool canTransfer = false;
	bool canTalk = false;
	bool canVisit = false;
	bool canBuy = false;

	bool heldFriendly = false;
	bool heldEnemy = false;
};

struct CantoOptionsMenu : public Menu
{
	CantoOptionsMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO);
	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void CancelOption() override;
};

struct ItemOptionsMenu : public Menu
{
	ItemOptionsMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO);
	virtual void Draw() override;
	void DrawWeaponComparison(std::vector<Item*>& inventory);
	virtual void SelectOption() override;
	virtual void GetOptions() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;

	virtual void GetBattleStats();

	BattleStats currentStats;
	BattleStats selectedStats;
};

struct ItemUseMenu : public Menu
{
	ItemUseMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, Item* selectedItem, int inventoryIndex);
	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void CancelOption() override;

	virtual void  GetOptions() override;

	const static int USE = 0;
	const static int EQUIP = 1;
	const static int DROP = 2;

	int inventoryIndex;
	Item* item = nullptr;
	bool canUse = false;
	bool canEquip = false;
};

struct SelectWeaponMenu : public ItemOptionsMenu
{
	SelectWeaponMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<Item*>& validWeapons, std::vector<std::vector<Unit*>>& units, bool capturing = false);
	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void GetOptions() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;

	bool capturing = false;
	std::vector<Item*> weapons;
	std::vector<std::vector<Unit*>> unitsToAttack;

	virtual void GetBattleStats() override;

};

struct DisplayedBattleStats
{
	std::string lvl;
	std::string hP;
	std::string atk;
	std::string def;
	std::string hit;
	std::string crit;
	std::string attackSpeed;
};

struct SelectEnemyMenu : public Menu
{
	SelectEnemyMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<Unit*>& units, SpriteRenderer* Renderer, int selectedWeapon, bool capturing = false);
	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void GetOptions() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;
	virtual void CancelOption();
	void CanEnemyCounter(bool capturing = false);
	//Using these so I can handle formatting once rather than doing it repeatedly in draw
	//I think I am still going to need the normal battle stats for actual combat calculations
	BattleStats enemyNormalStats;
	BattleStats unitNormalStats;
	DisplayedBattleStats enemyStats;
	DisplayedBattleStats playerStats;

	int selectedWeapon;
	std::vector<Unit*> unitsToAttack;
	SpriteRenderer* renderer = nullptr;
	bool enemyCanCounter = false;
	bool capturing = false;
};

struct SelectTradeUnit : public Menu
{
	SelectTradeUnit(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<Unit*>& units, SpriteRenderer* Renderer);

	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void GetOptions() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;

	std::vector<Unit*> tradeUnits;
	SpriteRenderer* renderer;
};

struct SelectTalkMenu : public Menu
{
	SelectTalkMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<Unit*>& units, SpriteRenderer* Renderer);

	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void GetOptions() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;

	std::vector<Unit*> talkUnits;
	SpriteRenderer* renderer;
};

struct SelectRescueUnit : public Menu
{
	SelectRescueUnit(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<Unit*>& units, SpriteRenderer* Renderer);

	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void GetOptions() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;

	std::vector<Unit*> rescueUnits;
	SpriteRenderer* renderer;
};

struct SelectTransferUnit : public Menu
{
	SelectTransferUnit(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<Unit*>& units, SpriteRenderer* Renderer);

	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void GetOptions() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;

	std::vector<Unit*> transferUnits;
	SpriteRenderer* renderer;
};

struct DropMenu : public Menu
{
	DropMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<glm::ivec2>& positions, SpriteRenderer* Renderer);

	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void GetOptions() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;

	std::vector<glm::ivec2>& positions;
	SpriteRenderer* renderer;
};

struct TradeMenu : public Menu
{
	TradeMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, Unit* unit);

	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void GetOptions() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;
	virtual void CancelOption() override;

	Unit* tradeUnit = nullptr;
	int itemToMove;

	bool moving = false;
	bool firstInventory = true;
	bool moveFromFirst = true;
};

struct UnitStatsViewMenu : public Menu
{
	UnitStatsViewMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, Unit* unit, SpriteRenderer* Renderer);
	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;
	virtual void CancelOption() override;

	Unit* unit;
	BattleStats battleStats;
	SpriteRenderer* renderer;

	//Need these for swapping the view between units
	int unitIndex = 0;
	std::vector<Unit*>* unitList;

	//Just here as a proof of concept for now
	std::vector<glm::vec4> iconUVs;

	bool firstPage = true;
	bool examining = false;
	bool transition = false;

	float yOffset = 224;
	float start;
	float goal;

	float t = 0.0f;

};

//No idea what to call this one
struct ExtraMenu : public Menu
{
	const static int UNIT = 0;
	const static int STATUS = 1;
	const static int OPTIONS = 2;
	const static int SUSPEND = 3;
	const static int END = 4;
	ExtraMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO);
	virtual void Draw() override;
	virtual void SelectOption() override;
};

struct UnitListMenu : public Menu
{
	const static int GENERAL = 0;
	const static int EQUIPMENT = 1;
	const static int COMBAT_STATS = 2;
	const static int PERSONAL = 3;
	const static int WEAPON_RANKS = 4;
	const static int SKILLS = 5;

	UnitListMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO);
	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;

	void CloseAndSaveView();

	void SortView();

	int currentPage = 0;
	int numberOfPages = 6;
	int sortType = 0;
	int sortIndicator = 0;
	bool sortMode = false;
	std::vector<int> pageSortOptions;
	std::vector<std::string> sortNames;
	std::vector <std::pair<Unit*, BattleStats>> unitData;
};

struct StatusMenu : public Menu
{
	StatusMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO);
	virtual void Draw() override;
	virtual void SelectOption() override;
};

struct OptionsMenu : public Menu
{
	OptionsMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO);
	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;

	void RenderText(std::string toWrite, float x, float y, float scale, bool selected);

	int top = 117;
	int bottom = 501;
	int distance = 64;

	int indicatorY = 40;
	int indicatorIncrement = 24;
	float yOffset = 0;
	float goal;
	bool up = false;
	bool down = false;
};

struct FullInventoryMenu : public Menu
{
	FullInventoryMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, int newItem);
	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;

	int newItem = -1;

	BattleStats currentStats;
	BattleStats selectedStats;
};

enum VendorState
{
	GREETING,
	BUYING,
	SELLING,
	CONFIRMING,
	LEAVING
};

struct VendorMenu : public Menu
{
	VendorMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, Unit* buyer, class Vendor* vendor);
	virtual void Draw() override;
	virtual void SelectOption() override;
	void ActivateText();
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;
	virtual void CancelOption() override;

	Unit* buyer;
	Vendor* vendor;

	TextObjectManager textManager;
	TextObject testText;

	VendorState state;
	bool buying = true;
	bool confirm = true;
	bool delay = false;

	float delayTime = 0.15f;
	float delayTimer = 0.0f;
};

struct MenuManager
{
	void SetUp(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer, 
		BattleManager* battleManager, class PlayerManager* playerManager, class EnemyManager* enemyManager);

	void AddMenu(int menuID);

	void AddUnitStatMenu(Unit* unit);

	void AddFullInventoryMenu(int itemID);

	void PreviousMenu();
	void ClearMenu();

	std::vector<Menu*> menus;

	Cursor* cursor = nullptr;
	TextRenderer* text = nullptr;
	Camera* camera = nullptr;
	SpriteRenderer* renderer = nullptr;
	BattleManager* battleManager = nullptr;
	PlayerManager* playerManager = nullptr;
	EnemyManager* enemyManager = nullptr;

	Subject<int> subject;

	std::unordered_map<int, std::string> profcienciesMap;

	//Some actions, such as trading or dismounting will allow the menu to stay open but will force the unit to stay where they have been moved too
	bool mustWait = false;

	int shapeVAO; //Not sure I need this long term, as I will eventually replace shape drawing with sprites

	int unitViewSortType;

	static MenuManager menuManager;
};