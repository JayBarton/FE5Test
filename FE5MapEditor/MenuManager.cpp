#include "MenuManager.h"
#include "TextRenderer.h"
#include "Camera.h"
#include "../InputManager.h"
#include "../csv.h"
#include "PlacementModes.h"
#include <SDL.h>

#include <sstream>
Menu::Menu(TextRenderer* Text, Camera* Camera, int shapeVAO) : text(Text), camera(Camera), shapeVAO(shapeVAO)
{
}

void Menu::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (inputManager.isKeyPressed(SDLK_UP))
	{
		currentOption--;
		if (currentOption < 0)
		{
			currentOption = numberOfOptions - 1;
		}
	}
	if (inputManager.isKeyPressed(SDLK_DOWN))
	{
		currentOption++;
		if (currentOption >= numberOfOptions)
		{
			currentOption = 0;
		}
	}
	if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
	if (inputManager.isKeyPressed(SDLK_z))
	{
		CancelOption();
		currentOption = 0;
	}
}

void Menu::CancelOption()
{
	MenuManager::menuManager.PreviousMenu();
}

void Menu::ClearMenu()
{
	MenuManager::menuManager.ClearMenu();
}

MenuManager MenuManager::menuManager;
void MenuManager::SetUp(TextRenderer* Text, Camera* Camera, int shapeVAO, SpriteRenderer* Renderer)
{
	renderer = Renderer;
	text = Text;
	camera = Camera;
	this->shapeVAO = shapeVAO;
}

void MenuManager::AddMenu(int ID)
{
}

void MenuManager::AddEnemyMenu(EnemyMode* mode, Object* obj)
{
	Menu* newMenu = new EnemyMenu(text, camera, shapeVAO, mode, obj);
	MenuManager::menuManager.menus.push_back(newMenu);
}

void MenuManager::AddInventoryMenu(EnemyMode* mode, Object* obj, std::vector<int>& inventory, std::vector<Item>& items)
{
	Menu* newMenu = new InventoryMenu(text, camera, shapeVAO, mode, obj, inventory, items);
	MenuManager::menuManager.menus.push_back(newMenu);
}

void MenuManager::PreviousMenu()
{
	Menu* p = menus.back();
	menus.pop_back();
	delete p;
}

void MenuManager::ClearMenu()
{
	while (menus.size() > 0)
	{
		PreviousMenu();
	}
}

EnemyMenu::EnemyMenu(TextRenderer* Text, Camera* camera, int shapeVAO, EnemyMode* mode, Object* object) : Menu(Text, camera, shapeVAO)
{
	numberOfOptions = 3;
	currentOption = 0;
	this->mode = mode;
	this->object = object;
	int ID;
	int HP;
	int str;
	int mag;
	int skl;
	int spd;
	int lck;
	int def;
	int bld;
	int currentUnit = 0;

	io::CSVReader<9, io::trim_chars<' '>, io::no_quote_escape<':'>> in2("../EnemyGrowths.csv");
	in2.read_header(io::ignore_extra_column, "ID", "HP", "Str", "Mag", "Skl", "Spd", "Lck", "Def", "Bld");
	unitGrowths.resize(6);
	while (in2.read_row(ID, HP, str, mag, skl, spd, lck, def, bld)) {
		unitGrowths[currentUnit] = StatGrowths{ HP, str, mag, skl, spd, lck, def, bld, 0 };
		currentUnit++;
	}

	growthNames[0] = "Physical 1";
	growthNames[1] = "Magical 1";
	growthNames[2] = "Fighter 1";
	growthNames[3] = "Physical 2";
	growthNames[4] = "Magical 2";
	growthNames[5] = "Fighter 2";

	io::CSVReader<6, io::trim_chars<' '>, io::no_quote_escape<':'>> in("../items.csv");
	in.read_header(io::ignore_extra_column, "ID", "name", "maxUses", "useID", "isWeapon", "description");
	std::string name;
	int maxUses;
	int useID;
	int isWeapon;
	std::string description;
	while (in.read_row(ID, name, maxUses, useID, isWeapon, description)) {
		items.push_back({ ID, name, maxUses, maxUses, useID, bool(isWeapon), description });
	}

	if (object)
	{
		level = object->level;
		growthRateID = object->growthRateID;
		inventory = object->inventory;
	}
}
std::string intToString2(int i)
{
	std::stringstream s;
	s << i;
	return s.str();
}
void EnemyMenu::Draw()
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(32 + 64 * currentOption, 56, 0.0f));

	model = glm::scale(model, glm::vec3(16, 16, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	text->RenderText("Level : " + intToString2(level), 100, 200, 1);
	auto growths = unitGrowths[growthRateID];
	text->RenderText("Growth Pattern: " + growthNames[growthRateID], 200, 200, 1);

	text->RenderText("HP: " + intToString2(growths.maxHP), 200, 230, 1);
	text->RenderText("Str: " + intToString2(growths.strength), 200, 260, 1);
	text->RenderText("Mag: " + intToString2(growths.magic), 200, 290, 1);
	text->RenderText("Skl: " + intToString2(growths.skill), 200, 320, 1);
	text->RenderText("Spd: " + intToString2(growths.speed), 200, 350, 1);
	text->RenderText("Lck: " + intToString2(growths.luck), 200, 380, 1);
	text->RenderText("Def: " + intToString2(growths.defense), 200, 410, 1);
	text->RenderText("Bld: " + intToString2(growths.build), 200, 440, 1);

	text->RenderText("Inventory", 500, 200, 1);
	if (!inInventory)
	{
		for (int i = 0; i < inventory.size(); i++)
		{
			auto item = items[inventory[i]];
			text->RenderText(item.name, 500, 230 + 30 * i + 1, 1);
		}
	}
}

void EnemyMenu::SelectOption()
{
	if (object)
	{
		mode->updateEnemy(level, growthRateID, inventory, object->type);
	}
	else
	{
		mode->placeEnemy(level, growthRateID, inventory);
	}
	CancelOption();
}

void EnemyMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (inputManager.isKeyPressed(SDLK_UP))
	{
		if (currentOption == CHANGE_LEVEL)
		{
			level++;
			if (level > 20)
			{
				level = 1;
			}
		}
		else if(currentOption == CHANGE_GROWTH)
		{
			growthRateID++;
			if (growthRateID > 5)
			{
				growthRateID = 0;
			}
		}

	}
	if (inputManager.isKeyPressed(SDLK_DOWN))
	{
		if (currentOption == CHANGE_LEVEL)
		{
			level--;
			if (level < 1)
			{
				level = 20;
			}
		}
		else if (currentOption == CHANGE_GROWTH)
		{
			growthRateID--;
			if (growthRateID < 0)
			{
				growthRateID = 5;
			}
		}
		else if	(currentOption == OPEN_INVENTORY)
		{
			MenuManager::menuManager.AddInventoryMenu(mode, object, inventory, items);
			inInventory = true;
		}
	}
	if (inputManager.isKeyPressed(SDLK_RIGHT))
	{
		currentOption++;
		if (currentOption >= numberOfOptions)
		{
			currentOption = 0;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_LEFT))
	{
		currentOption--;
		if (currentOption < 0)
		{
			currentOption = numberOfOptions -1;
		}
	}
	if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
	if (inputManager.isKeyPressed(SDLK_z))
	{
		CancelOption();
		currentOption = 0;
	}
}

InventoryMenu::InventoryMenu(TextRenderer* Text, Camera* camera, int shapeVAO,
	class EnemyMode* mode, class Object* object, std::vector<int>& inventory, std::vector<Item>& items)
	: Menu(Text, camera, shapeVAO), inventory(inventory), items(items)
{
	currentSlot = 0;
	if (currentSlot < inventory.size())
	{
		currentItem = inventory[currentSlot];
	}
}

void InventoryMenu::SelectOption()
{
	if (inventory.size() > currentSlot)
	{
		//update inventory slot
		inventory[currentSlot] = currentItem;
	}
	else
	{
		inventory.push_back(currentItem);
		currentSlot++;
	}
}

void InventoryMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (inputManager.isKeyPressed(SDLK_UP))
	{
		if (inventory.size() > currentSlot)
		{
			//update inventory slot
			inventory[currentSlot] = currentItem;
		}
		currentSlot--;
		if (currentSlot < 0)
		{
			currentSlot = inventory.size();
		}
		if (currentSlot < inventory.size())
		{
			currentItem = inventory[currentSlot];
		}
		else
		{
			currentItem = 0;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_DOWN))
	{
		if (inventory.size() > currentSlot)
		{
			//update inventory slot
			inventory[currentSlot] = currentItem;
		}
		currentSlot++;
		if (currentSlot > inventory.size())
		{
			currentSlot = 0;
		}
		if (currentSlot < inventory.size())
		{
			currentItem = inventory[currentSlot];
		}
		else
		{
			currentItem = 0;
		}
	}

	else if (inputManager.isKeyPressed(SDLK_RIGHT))
	{
		currentItem++;
		if (currentItem > items.size() - 1)
		{
			currentItem = 0;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_LEFT))
	{
		currentItem--;
		if (currentItem < 0)
		{
			currentItem = items.size() - 1;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
	else if (inputManager.isKeyPressed(SDLK_BACKSPACE))
	{
		if (inventory.size() > currentSlot)
		{
			inventory.erase(inventory.begin() + currentSlot);
			if (currentSlot < inventory.size())
			{
				currentItem = inventory[currentSlot];
			}
		}
	}
	else if (inputManager.isKeyPressed(SDLK_z))
	{
		CancelOption();
		currentOption = 0;
	}
}

void InventoryMenu::CancelOption()
{
	Menu::CancelOption();
	static_cast<EnemyMenu*>(MenuManager::menuManager.menus.back())->inInventory = false;
}

void InventoryMenu::Draw()
{
	MenuManager::menuManager.menus[MenuManager::menuManager.menus.size() - 2]->Draw();

	if (currentSlot >= 0)
	{
		ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
		ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
		glm::mat4 model = glm::mat4();
		model = glm::translate(model, glm::vec3(144, 80 + 12 * currentSlot, 0.0f));

		model = glm::scale(model, glm::vec3(16, 16, 0.0f));

		ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

		ResourceManager::GetShader("shape").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
	}

	for (int i = 0; i < inventory.size(); i++)
	{
		if (i != currentSlot)
		{
			auto item = items[inventory[i]];
			text->RenderText(item.name, 500, 230 + 30 * i + 1, 1);
			text->RenderTextRight(intToString2(item.maxUses), 628, 230 + 30 * i + 1, 1, 14);
		}
	}
	if (currentItem >= 0)
	{
		auto item = items[currentItem];
		text->RenderText(item.name, 500, 230 + 30 * currentSlot + 1, 1, glm::vec3(1.0f, 1.0f, 0.0f));
		text->RenderTextRight(intToString2(item.maxUses), 628, 230 + 30 * currentSlot + 1, 1, 14, glm::vec3(1.0f, 1.0f, 0.0f));

	}

}