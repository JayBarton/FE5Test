#pragma once
#include <vector>
#include "Unit.h"
class Cursor;
class TextRenderer;
class Camera;
class Item;
class BattleStats;
class SpriteRenderer;
class BattleManager;

//Code duplication from Unit.h here. I don't have a generic messaging system, so this will have to do for now.
//I just need to commincate to main that a battle has started. Perhaps I could make some sort of GameManager class that main handles but is otherwise
//independent of it. But that's later.
struct BattleObserver
{
	virtual ~BattleObserver() {}
	virtual void onNotify(class Unit* attacker, class Unit* defender) = 0;
};

struct BattleSubject
{
	std::vector<BattleObserver*> observers;

	void addObserver(BattleObserver* observer)
	{
		observers.push_back(observer);
	}
	void removeObserver(BattleObserver* observer)
	{
		auto it = std::find(observers.begin(), observers.end(), observer);
		if (it != observers.end())
		{
			delete* it;
			*it = observers.back();
			observers.pop_back();
		}
	}
	void notify(class Unit* attacker, class Unit* defender)
	{
		for (int i = 0; i < observers.size(); i++)
		{
			observers[i]->onNotify(attacker, defender);
		}
	}
};

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
	void ClearMenu();

	Cursor* cursor = nullptr;
	TextRenderer* text = nullptr;
	Camera* camera = nullptr;

	int shapeVAO; //Not sure I need this long term, as I will eventually replace shape drawing with sprites

	int currentOption = 0;
	std::vector<int> optionsVector;

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

	std::vector<Unit*> unitsInRange;

	bool canAttack = false;
	bool canDismount = false;
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
	SelectWeaponMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<Item*>& validWeapons, std::vector<std::vector<Unit*>>& units);
	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void GetOptions() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;

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
	SelectEnemyMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<Unit*>& units, SpriteRenderer* Renderer);
	virtual void Draw() override;
	virtual void SelectOption() override;
	virtual void GetOptions() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;

	void CanEnemyCounter();
	//Using these so I can handle formatting once rather than doing it repeatedly in draw
	//I think I am still going to need the normal battle stats for actual combat calculations
	BattleStats enemyNormalStats;
	BattleStats unitNormalStats;
	DisplayedBattleStats enemyStats;
	DisplayedBattleStats playerStats;

	std::vector<Unit*> unitsToAttack;
	SpriteRenderer* renderer = nullptr;
	bool enemyCanCounter = false;
};

struct MenuManager
{
	void SetUp(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer, BattleManager* battleManager);

	void AddMenu(int menuID);

	void PreviousMenu();
	void ClearMenu();

	std::vector<Menu*> menus;

	Cursor* cursor = nullptr;
	TextRenderer* text = nullptr;
	Camera* camera = nullptr;
	SpriteRenderer* renderer = nullptr;
	BattleManager* battleManager = nullptr;

	BattleSubject subject;

	int shapeVAO; //Not sure I need this long term, as I will eventually replace shape drawing with sprites

	static MenuManager menuManager;
};