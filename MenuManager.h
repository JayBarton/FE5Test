#pragma once
#include <vector>
class Cursor;
class TextRenderer;
class Camera;
struct Menu
{
	Menu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO);
	virtual ~Menu() {}
	virtual void Draw() = 0;
	virtual void SelectOption() = 0;
	virtual void CancelOption();
	void CheckInput(class InputManager& inputManager, float deltaTime);
	void ClearMenu();
//	virtual void getAction() = 0;

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

	void GetUnitOptions();

	const static int ITEMS = 0;
	const static int WAIT = 1;
	const static int DISMOUNT = 2;
	const static int ATTACK = 3;

	bool canAttack = false;
	bool canDismount = false;
};

struct ItemOptionsMenu : public Menu
{
	ItemOptionsMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO);
	virtual void Draw() override;
	virtual void SelectOption() override;
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