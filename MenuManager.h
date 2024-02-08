#pragma once
#include <vector>
#include "Unit.h"
class Cursor;
class TextRenderer;
class Camera;
class Item;
class BattleStats;
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
	virtual void SelectOption() override;
	virtual void GetOptions() override;
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;

	void GetBattleStats();

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

struct SelectWeaponMenu : public Menu
{
	SelectWeaponMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<Item*>& validWeapons);
	virtual void Draw() override;
	virtual void SelectOption() override;

	virtual void GetOptions() override;

	std::vector<Item*> weapons;

	//Evil code duplication
	virtual void CheckInput(InputManager& inputManager, float deltaTime) override;

	void GetBattleStats();

	BattleStats currentStats;
	BattleStats selectedStats;

};

struct MenuManager
{
	void SetUp(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO);

	void AddMenu(int menuID);

	void PreviousMenu();
	void ClearMenu();

	std::vector<Menu*> menus;

	Cursor* cursor = nullptr;
	TextRenderer* text = nullptr;
	Camera* camera = nullptr;

	int shapeVAO; //Not sure I need this long term, as I will eventually replace shape drawing with sprites

	static MenuManager menuManager;
};