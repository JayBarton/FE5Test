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
	Menu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer);
	virtual ~Menu() {}
	virtual void Draw() = 0;
	virtual void SelectOption() = 0;
	virtual void CancelOption(int num = 1);
	virtual void GetOptions() {};
	virtual void CheckInput(class InputManager& inputManager, float deltaTime);
	virtual void DelayedExit();
	void NextOption();
	void NextOptionStop();
	void PreviousOption();
	void PreviousOptionStop();
	void EndUnitMove();
	void DrawBox(glm::ivec2 position, int width, int height);
	void DrawPattern(glm::vec2 scale, glm::vec2 pos, bool gray = false);
	void HandleFadeIn(float deltaTime);
	void HandleFadeOut(float deltaTime);
	void DrawFadeIn();
	void ClearMenu();

	Cursor* cursor = nullptr;
	TextRenderer* text = nullptr;
	Camera* camera = nullptr;
	SpriteRenderer* Renderer = nullptr;

	int shapeVAO; //Not sure I need this long term, as I will eventually replace shape drawing with sprites

	int currentOption = 0;
	int numberOfOptions = 0;
	std::vector<int> optionsVector;

	float fadeInAlpha = 0.0f;
	//If this menu covers the whole screen
	//Used in the main draw call, if it is true, we don't need to draw anything else but the menu
	bool fullScreen = false;
	//These are used for fading in a handful of fullscreen menus. Specifically, status, options, trade, and vendor
	bool fullFadeIn = false;
	bool fullFadeOut = false;
	bool exitMenu = false;
};

struct UnitOptionsMenu : public Menu
{
	UnitOptionsMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer);
	virtual void Draw() override;
	void DrawMenu(bool animate = false);
	virtual void SelectOption() override;
	virtual void CancelOption(int num = 1) override;
	virtual void CheckInput(class InputManager& inputManager, float deltaTime);
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
	const static int SEIZE = 14;
	const static int ANIMATION = 15;

	std::vector<Unit*> unitsInRange;
	std::vector<Unit*> unitsInCaptureRange;
	std::vector<Unit*> tradeUnits;
	std::vector<Unit*> talkUnits;
	std::vector<Unit*> rescueUnits;
	std::vector<Unit*> transferUnits;

	std::vector<glm::ivec2> dropPositions;

	bool hasItems = false;
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
	bool canSeize = false;

	bool heldFriendly = false;
	bool heldEnemy = false;
};

struct CantoOptionsMenu : public Menu
{
	CantoOptionsMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer);
	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void CancelOption(int num = 1) override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;
};

struct ItemOptionsMenu : public Menu
{
	ItemOptionsMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer);
	virtual void Draw() override;
	void DrawItemWindow(std::vector<Item*>& inventory, Unit* unit, bool animate = false);
	void DrawWeaponComparison(std::vector<Item*>& inventory);
	void DrawPortrait(Unit* unit);
	virtual void SelectOption() override;
	virtual void GetOptions() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;

	virtual void GetBattleStats();

	BattleStats currentStats;
	BattleStats selectedStats;

	std::vector<glm::vec4> itemIconUVs;
	std::vector<glm::vec4> proficiencyIconUVs;
};

struct ItemUseMenu : public Menu
{
	ItemUseMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer, Item* selectedItem, int inventoryIndex);
	virtual void Draw() override;
	void DrawMenu(bool animate = false);
	virtual void SelectOption() override;
	virtual void CancelOption(int num = 1) override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;

	virtual void  GetOptions() override;

	const static int USE = 0;
	const static int EQUIP = 1;
	const static int DROP = 2;

	int inventoryIndex;
	Item* item = nullptr;
	std::vector<glm::vec4> itemIconUVs;

	ItemOptionsMenu* previous;

	bool canUse = false;
	bool canEquip = false;
};

struct DropConfirmMenu : public Menu
{
	DropConfirmMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer, int index);
	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void CancelOption(int num = 1) override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;

	virtual void  GetOptions() override;

	int inventoryIndex;
	Item* item = nullptr;

	ItemUseMenu* previous;
};

struct AnimationOptionsMenu : public Menu
{
	AnimationOptionsMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer);
	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;

	virtual void  GetOptions() override;
	UnitOptionsMenu* previous;
	int yPosition;
	int yIndicator;
};

struct SelectWeaponMenu : public ItemOptionsMenu
{
	SelectWeaponMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer, std::vector<Item*>& validWeapons, std::vector<std::vector<Unit*>>& units, bool capturing = false);
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
	SelectEnemyMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer, std::vector<Unit*>& units, int selectedWeapon, bool capturing = false);
	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void GetOptions() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;
	virtual void CancelOption(int num = 1);
	void CanEnemyCounter(bool capturing = false);
	//Using these so I can handle formatting once rather than doing it repeatedly in draw
	//I think I am still going to need the normal battle stats for actual combat calculations
	BattleStats enemyNormalStats;
	BattleStats unitNormalStats;
	DisplayedBattleStats enemyStats;
	DisplayedBattleStats playerStats;

	int selectedWeapon;
	int attackDistance;
	std::vector<Unit*> unitsToAttack;
	bool enemyCanCounter = false;
	bool capturing = false;
};

struct SelectTradeUnit : public Menu
{
	SelectTradeUnit(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer, std::vector<Unit*>& units);

	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void GetOptions() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;

	std::vector<Unit*> tradeUnits;
};

struct SelectTalkMenu : public Menu
{
	SelectTalkMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer, std::vector<Unit*>& units);

	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void GetOptions() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;

	std::vector<Unit*> talkUnits;
};

struct SelectRescueUnit : public Menu
{
	SelectRescueUnit(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer, std::vector<Unit*>& units);

	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void GetOptions() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;

	std::vector<Unit*> rescueUnits;
};

struct SelectTransferUnit : public Menu
{
	SelectTransferUnit(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer, std::vector<Unit*>& units);

	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void GetOptions() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;

	std::vector<Unit*> transferUnits;
};

struct DropMenu : public Menu
{
	DropMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer, std::vector<glm::ivec2>& positions);

	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void GetOptions() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;

	std::vector<glm::ivec2>& positions;
};

struct TradeMenu : public Menu
{
	TradeMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer, Unit* unit);

	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void GetOptions() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;
	virtual void CancelOption(int num = 1) override; //
	virtual void DelayedExit();

	Unit* tradeUnit = nullptr;
	int itemToMove;

	bool moving = false;
	bool firstInventory = true;
	bool moveFromFirst = true;
	std::vector<glm::vec4> itemIconUVs;
};

struct SkillInfo
{
	std::string name;
	std::string description;
};

struct UnitStatsViewMenu : public Menu
{
	UnitStatsViewMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer, Unit* unit);
	virtual void Draw() override;
	void DrawUpperSection();
	void DrawPage2();
	void DrawPage1();
	virtual void SelectOption() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;
	virtual void CancelOption(int num = 1) override;

	Unit* unit;
	BattleStats battleStats;

	//Need these for swapping the view between units
	int unitIndex = 0;
	std::vector<Unit*>* unitList;

	//Just here as a proof of concept for now
	std::vector<glm::vec4> proficiencyIconUVs;
	std::vector<glm::vec4> itemIconUVs;
	std::vector<glm::vec4> skillIconUVs;
	std::vector<glm::vec4> carryUVs;

	std::vector<SkillInfo> skillInfo;

	bool firstPage = true;
	bool examining = false;
	bool transition = false;

	float yOffset = 224;
	float start;
	float goal;

	float t = 0.0f;

	float skillAniamteTimer = 0;

	int skillAnimateFrame = 0;
};

//No idea what to call this one
struct ExtraMenu : public Menu
{
	const static int UNIT = 0;
	const static int STATUS = 1;
	const static int OPTIONS = 2;
	const static int SUSPEND = 3;
	const static int END = 4;
	ExtraMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer);
	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;
};

struct UnitListMenu : public Menu
{
	const static int GENERAL = 0;
	const static int EQUIPMENT = 1;
	const static int COMBAT_STATS = 2;
	const static int PERSONAL = 3;
	const static int WEAPON_RANKS = 4;
	const static int SKILLS = 5;

	UnitListMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer);
	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;

	void CloseAndSaveView();

	void SortView();

	int currentPage = 0;
	int numberOfPages = 6;
	int sortType = 0;
	int currentSort = 0;
	int sortIndicator = 0;
	bool sortMode = false;
	std::vector<int> pageSortOptions;
	std::vector<std::string> sortNames;
	std::vector<glm::ivec2> sortIndicatorLocations;
	std::vector <std::pair<Unit*, BattleStats>> unitData;

	std::vector<glm::vec4> proficiencyIconUVs;
	std::vector<glm::vec4> skillIconUVs;
	int profOrder[10];
};

struct StatusMenu : public Menu
{
	StatusMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer);
	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;
};

struct OptionsMenu : public Menu
{
	OptionsMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer);
	virtual void Draw() override;
	void DrawIndicators();
	virtual void SelectOption() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;

	void CheckColorChange(std::vector<int>& inColor, std::vector<int>& defaultColor, int& patternID);

	void RenderText(std::string toWrite, float x, float y, float scale, bool selected);

	int top = 117;
	int bottom = 501;
	int distance = 64;

	int indicatorY = 39;
	int indicatorY2 = 39;
	int indicatorIncrement = 24;
	float yOffset = 0;
	float goal = 0;
	bool up = false;
	bool down = false;
	bool hitBottom = false;
	bool moveToBottom = false;

	std::string optionDescriptions[17];

//	glm::vec3 topColor;
//	glm::vec3 bottomColor;
};

struct FullInventoryMenu : public Menu
{
	FullInventoryMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer, int newItem);
	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;

	int newItem = -1;

	BattleStats currentStats;
	BattleStats selectedStats;

	std::vector<glm::vec4> itemIconUVs;
	std::vector<glm::vec4> proficiencyIconUVs;
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
	VendorMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer, Unit* buyer, class Vendor* vendor);
	virtual void Draw() override;
	virtual void SelectOption() override;
	void ActivateText();
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;
	virtual void CancelOption(int num = 1) override;
	virtual void DelayedExit() override;

	Unit* buyer;
	Vendor* vendor;

	TextObjectManager textManager;

	VendorState state;
	bool buying = true;
	bool confirm = true;
	bool delay = false;
	//Need this to delay displaying item descriptions
	bool shopDelay = false;

	float delayTime = 0.15f;
	float delayTimer = 0.0f;
};

//Not actually a menu, makes unit rescue/release/etc animations easier
struct UnitMovement :public Menu
{
	const static int RESCUE = 0;
	const static int TRANSFER = 1;
	const static int DROP = 2;
	const static int RELEASE = 3;

	UnitMovement(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer, Unit* movingUnit, Unit* receivingUnit, int operation, glm::ivec2 dropPosition = glm::ivec2(0));
	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;

	Unit* receivingUnit = nullptr;
	Unit* movingUnit = nullptr;
	float moveTimer = 0.0f;
	float moveDelay = 0.15f;
	float removeTimer = 0.0f;

	glm::ivec2 dropPosition;

	int operation = 0;

	bool doneHere = false;
	bool enemyFading = false;
};

struct SuspendMenu : public Menu
{
	SuspendMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer);
	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;

	bool suspended = false;
	bool opening = false;
	bool closing = false;
	bool returningToMenu = false;

	float delay = 0.0f;
	float fadeValue = 0.0f;
	float fadeValueMax = 0.525f;
	float menuReturnTimer = 0.0f;
	
	float fadeOut = 0.0f;
};

struct TitleMenu : public Menu
{
	TitleMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer, Subject<int>* subject, bool foundSuspend);
	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void CancelOption(int num = 1) override;
	virtual void CheckInput(class InputManager& inputManager, float deltaTime);
	virtual void GetOptions() override;

	Subject<int>* subject;
	float fadeAlpha = 0.0f;
	bool foundSuspend = false;
	bool fadingOut = false;
};

struct MenuManager
{
	void SetUp(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer, 
		BattleManager* battleManager, class PlayerManager* playerManager, class EnemyManager* enemyManager);

	void AddMenu(int menuID);
	void AddUnitStatMenu(Unit* unit);
	void AddFullInventoryMenu(int itemID);
	void AddTitleMenu(Subject<int>* subject, bool foundSuspend);

	void PreviousMenu();
	void ClearMenu();

	void AnimateArrow(float deltaTime);
	void DrawArrow(glm::ivec2 position, bool down = true);

	void AnimateIndicator(float deltaTime);
	void DrawIndicator(glm::ivec2 position, bool animated = true, float rot = 0);

	void AnimateArrowIndicator(float deltaTime);
	void DrawArrowIndicator(glm::ivec2 position);

	Menu* GetCurrent();
	Menu* GetPrevious();

	std::vector<Menu*> menus;

	Cursor* cursor = nullptr;
	TextRenderer* text = nullptr;
	Camera* camera = nullptr;
	SpriteRenderer* renderer = nullptr;
	BattleManager* battleManager = nullptr;
	PlayerManager* playerManager = nullptr;
	EnemyManager* enemyManager = nullptr;

	std::vector<glm::vec4> proficiencyIconUVs;
	std::vector<glm::vec4> itemIconUVs;
	std::vector<glm::vec4> skillIconUVs;
	std::vector<glm::vec4> optionIconUVs;
	std::vector<glm::vec4> arrowAnimUVs;
	std::vector<glm::vec4> arrowUV;
	std::vector<glm::vec4> carryingIconsUVs;
	std::vector<glm::vec4> boxesUVs;
	std::vector<glm::vec4> skillHighlightUVs;

	std::vector<glm::vec4> patternUVs;
	
	glm::vec4 indicatorUV;
	glm::vec4 statBarUV;
	glm::vec4 malusArrowUV;

	glm::vec4 colorBarsUV;
	glm::vec4 colorIndicatorUV;

	glm::vec2 indicatorPosition;
	float indicatorDrawX;
	float indicatorT;

	glm::vec2 indicatorArrowPosition;
	float indicatorArrowDrawX;
	float indicatorArrowT;

	Sprite arrowSprite;

	Subject<int> subject;
	Subject<> endingSubject;
	Subject<int> suspendSubject;
	Subject<Unit*> unitDiedSubject;

	std::unordered_map<int, std::string> profcienciesMap;

	//Some actions, such as trading or dismounting will allow the menu to stay open but will force the unit to stay where they have been moved too
	bool mustWait = false;
	bool mountActionTaken = false;

	int shapeVAO; //Not sure I need this long term, as I will eventually replace shape drawing with sprites

	int unitViewSortType;
	int unitViewPage;
	int unitViewIndicator;
	int unitViewCurrentSort;

	static MenuManager menuManager;
};