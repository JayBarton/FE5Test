#include "MenuManager.h"
#include "Cursor.h"
#include "TextRenderer.h"
#include "Camera.h"
#include "InputManager.h"
#include "Items.h"

#include <SDL.h>


#include <sstream>


Menu::Menu(Cursor* Cursor, TextRenderer* Text, Camera* Camera, int shapeVAO) : cursor(Cursor), text(Text), camera(Camera), shapeVAO(shapeVAO)
{
}
void Menu::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (inputManager.isKeyPressed(SDLK_UP))
	{
		currentOption--;
		if (currentOption < 0)
		{
			currentOption = optionsVector.size() - 1;
		}
		std::cout << currentOption << std::endl;
	}
	if (inputManager.isKeyPressed(SDLK_DOWN))
	{
		currentOption++;
		if (currentOption >= optionsVector.size())
		{
			currentOption = 0;
		}
		std::cout << currentOption << std::endl;
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

UnitOptionsMenu::UnitOptionsMenu(Cursor* Cursor, TextRenderer* Text, Camera* Camera, int shapeVAO) : Menu(Cursor, Text, Camera, shapeVAO)
{
	GetOptions();
}

void UnitOptionsMenu::SelectOption()
{
	switch (optionsVector[currentOption])
	{
	case ATTACK:
	{
		std::vector<Item*> validWeapons;
		for (int i = 0; i < unitsInRange.size(); i++)
		{
			auto currentUnit = unitsInRange[i];
			auto playerUnit = cursor->selectedUnit;
			float distance = abs(currentUnit->sprite.getPosition().x - playerUnit->sprite.getPosition().x) + abs(currentUnit->sprite.getPosition().y - playerUnit->sprite.getPosition().y);
			distance /= TileManager::TILE_SIZE;
			for (int c = 0; c < playerUnit->weapons.size(); c++)
			{
				auto weapon = playerUnit->GetWeaponData(playerUnit->weapons[c]);
				if (distance <= weapon.maxRange && distance >= weapon.minRange)
				{
					//Really hate this, but need to make sure weapons don't get added more than once.
					//Want a better way of doing this.
					bool dupe = false;
					for (int j = 0; j < validWeapons.size(); j++)
					{
						if (validWeapons[j] == playerUnit->weapons[c])
						{
							dupe = true;
							break;
						}
					}
					if (!dupe)
					{
						validWeapons.push_back(playerUnit->weapons[c]);
					}
				}
			}
		}
			Menu* newMenu = new SelectWeaponMenu(cursor, text, camera, shapeVAO, validWeapons);
			MenuManager::menuManager.menus.push_back(newMenu);
		std::cout << "Attack here eventually\n";
		break;
	}
	case ITEMS:
		MenuManager::menuManager.AddMenu(1);
		break;
	case DISMOUNT:
		std::cout << "Dismount here eventually\n";
		break;
		//Wait
	default:
		cursor->Wait();
		ClearMenu();
		break;
	}
}

void UnitOptionsMenu::CancelOption()
{
	cursor->UndoMove();
	Menu::CancelOption();
}

void UnitOptionsMenu::Draw()
{
//	int xStart = SCREEN_WIDTH;
	int xStart = 800;
	int yOffset = 100;
	glm::vec2 fixedPosition = camera->worldToScreen(cursor->position);
	if (fixedPosition.x >= camera->screenWidth * 0.5f)
	{
		//	xStart = 178;
	}
	//ResourceManager::GetShader("shape").Use().SetMatrix4("projection", glm::ortho(0.0f, 800.0f, 600.0f, 0.0f));
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(176, 32 + (12 * currentOption), 0.0f));

	model = glm::scale(model, glm::vec3(16, 16, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
	//Just a little test with new line
	std::string commands = "";
	if (canAttack)
	{
		commands += "Attack\n";
		text->RenderText("Attack", xStart - 200, yOffset, 1);
		yOffset += 30;
	}
	commands += "Items\n";
	text->RenderText("Items", xStart - 200, yOffset, 1);
	yOffset += 30;
	if (canDismount)
	{
		commands += "Dismount\n";
		text->RenderText("Dismount", xStart - 200, yOffset, 1);
		yOffset += 30;
	}
	commands += "Wait";
	text->RenderText("Wait", xStart - 200, yOffset, 1);
	//Text->RenderText(commands, xStart - 200, 100, 1);
}

void UnitOptionsMenu::GetOptions()
{
	currentOption = 0;
	canAttack = false;
	canDismount = false;
	optionsVector.clear();
	optionsVector.reserve(5);
	unitsInRange = cursor->inRangeUnits();
	if (unitsInRange.size() > 0)
	{
		canAttack = true;
		optionsVector.push_back(ATTACK);
	}
	optionsVector.push_back(ITEMS);
	//if can dismount
	canDismount = true;
	optionsVector.push_back(DISMOUNT);
	optionsVector.push_back(WAIT);
}

ItemOptionsMenu::ItemOptionsMenu(Cursor* Cursor, TextRenderer* Text, Camera* Camera, int shapeVAO) : Menu(Cursor, Text, Camera, shapeVAO)
{
	GetOptions();
	GetBattleStats();
}
//Here for now aaaa
std::string intToString2(int i)
{
	std::stringstream s;
	s << i;
	return s.str();
}
void ItemOptionsMenu::Draw()
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(16, 32 + (12 * currentOption), 0.0f));

	model = glm::scale(model, glm::vec3(16, 16, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	Unit* unit = cursor->selectedUnit;
	auto inventory = unit->inventory;
	for (int i = 0; i < inventory.size(); i++)
	{
		int yPosition = 100 + i * 30;
		text->RenderText(inventory[i]->name, 96, yPosition, 1);
		text->RenderTextRight(intToString2(inventory[i]->remainingUses), 200, yPosition, 1, 14);
	}
	if (inventory[currentOption]->isWeapon)
	{
		int offSet = 30;
		int yPosition = 100;
		auto weaponData = ItemManager::itemManager.weaponData[inventory[currentOption]->ID];
		
		int xStatName = 620;
		int xStatValue = 660;
		text->RenderText("Type", xStatName, yPosition, 1);
		text->RenderTextRight(intToString2(weaponData.type), xStatValue, yPosition, 1, 14);
		yPosition += offSet;
		text->RenderText("Atk", xStatName, yPosition, 1);
		text->RenderTextRight(intToString2(selectedStats.attackDamage), xStatValue, yPosition, 1, 14);
		if (selectedStats.attackDamage > currentStats.attackDamage)
		{
			text->RenderTextRight("^^^", xStatValue + 60, yPosition, 1, 14);
		}
		yPosition += offSet;
		text->RenderText("Hit", xStatName, yPosition, 1);
		text->RenderTextRight(intToString2(selectedStats.hitAccuracy), xStatValue, yPosition, 1, 14);
		if (selectedStats.hitAccuracy > currentStats.hitAccuracy)
		{
			text->RenderTextRight("^^^", xStatValue + 60, yPosition, 1, 14);
		}
		yPosition += offSet;
		text->RenderText("Crit", xStatName, yPosition, 1);
		text->RenderTextRight(intToString2(selectedStats.hitCrit), xStatValue, yPosition, 1, 14);
		if (selectedStats.hitCrit > currentStats.hitCrit)
		{
			text->RenderTextRight("^^^", xStatValue + 60, yPosition, 1, 14);
		}
		yPosition += offSet;
		text->RenderText("Avo", xStatName, yPosition, 1);
		text->RenderTextRight(intToString2(selectedStats.hitAvoid), xStatValue, yPosition, 1, 14);
		if (selectedStats.hitAvoid > currentStats.hitAvoid)
		{
			text->RenderTextRight("^^^", xStatValue + 60, yPosition, 1, 14);
		}
	}
	else
	{
		text->RenderText(inventory[currentOption]->description, 620, 100, 1);
	}
}

void ItemOptionsMenu::SelectOption()
{
	MenuManager::menuManager.AddMenu(2);
}

void ItemOptionsMenu::GetOptions()
{
	optionsVector.resize(cursor->selectedUnit->inventory.size());
}

void ItemOptionsMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	Menu::CheckInput(inputManager, deltaTime);

	if (inputManager.isKeyPressed(SDLK_UP) || inputManager.isKeyPressed(SDLK_DOWN))
	{
		GetBattleStats();
	}
}

void ItemOptionsMenu::GetBattleStats()
{
	Unit* unit = cursor->selectedUnit;
	auto inventory = unit->inventory;
	auto weaponData = ItemManager::itemManager.weaponData[inventory[currentOption]->ID];

	currentStats = unit->CalculateBattleStats();
	selectedStats = unit->CalculateBattleStats(weaponData.ID);
}

MenuManager MenuManager::menuManager;
void MenuManager::SetUp(Cursor* Cursor, TextRenderer* Text, Camera* Camera, int shapeVAO)
{
	cursor = Cursor;
	text = Text;
	camera = Camera;
	this->shapeVAO = shapeVAO;
}

void MenuManager::AddMenu(int ID)
{
	//I don't know if this ID system will stick around
	if (ID == 0)
	{
		Menu* newMenu = new UnitOptionsMenu(cursor, text, camera, shapeVAO);
		menus.push_back(newMenu);
	}
	else if (ID == 1)
	{
		Menu* newMenu = new ItemOptionsMenu(cursor, text, camera, shapeVAO);
		menus.push_back(newMenu);
	}
	else if (ID == 2)
	{
		auto currentOption = menus.back()->currentOption;
		Menu* newMenu = new ItemUseMenu(cursor, text, camera, shapeVAO, cursor->selectedUnit->inventory[currentOption], currentOption);
		menus.push_back(newMenu);
	}
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

ItemUseMenu::ItemUseMenu(Cursor* Cursor, TextRenderer* Text, Camera* Camera, int shapeVAO, Item* selectedItem, int index) : Menu(Cursor, Text, Camera, shapeVAO)
{
	inventoryIndex = index;
	item = selectedItem;
	GetOptions();
}

void ItemUseMenu::Draw()
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(176, 32 + (12 * currentOption), 0.0f));

	model = glm::scale(model, glm::vec3(16, 16, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	Unit* unit = cursor->selectedUnit;
	auto inventory = unit->inventory;
	for (int i = 0; i < inventory.size(); i++)
	{
		int yPosition = 100 + i * 30;
		text->RenderText(inventory[i]->name, 96, yPosition, 1);
		text->RenderTextRight(intToString2(inventory[i]->remainingUses), 200, yPosition, 1, 14);
	}
	int yOffset = 100;
	if (canUse)
	{
		text->RenderText("Use", 600, yOffset, 1);
		yOffset += 30;
	}	
	if (canEquip)
	{
		text->RenderText("Equip", 600, yOffset, 1);
		yOffset += 30;
	}
	text->RenderText("Drop", 600, yOffset, 1);
}

void ItemUseMenu::SelectOption()
{
	switch (optionsVector[currentOption])
	{
	case USE:
		ItemManager::itemManager.UseItem(cursor->selectedUnit, inventoryIndex, item->useID);
		cursor->Wait();
		ClearMenu();
		break;
	case EQUIP:
		cursor->selectedUnit->equipWeapon(inventoryIndex);
		//swap equipment
		break;
	case DROP:
		cursor->selectedUnit->dropItem(inventoryIndex);
		//Going back to the main selection menu is how FE5 does it, not sure if I want to keep that.
		MenuManager::menuManager.PreviousMenu();
		MenuManager::menuManager.PreviousMenu();
		break;
	default:
		break;
	}
}

void ItemUseMenu::CancelOption()
{
	Menu::CancelOption();
	MenuManager::menuManager.menus.back()->GetOptions();
}

void ItemUseMenu::GetOptions()
{
	if (item->useID >= 0)
	{
		optionsVector.push_back(USE);
		canUse = true;
	}
	if (item->isWeapon)
	{
		optionsVector.push_back(EQUIP);
		canEquip = true;
	}
	optionsVector.push_back(DROP);
}

SelectWeaponMenu::SelectWeaponMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<Item*>& validWeapons) : Menu(Cursor, Text, camera, shapeVAO)
{
	weapons = validWeapons;
	GetOptions();
	GetBattleStats();
}

void SelectWeaponMenu::Draw()
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(16, 32 + (12 * currentOption), 0.0f));

	model = glm::scale(model, glm::vec3(16, 16, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	Unit* unit = cursor->selectedUnit;
	auto inventory = weapons;
	for (int i = 0; i < inventory.size(); i++)
	{
		int yPosition = 100 + i * 30;
		text->RenderText(inventory[i]->name, 96, yPosition, 1);
		text->RenderTextRight(intToString2(inventory[i]->remainingUses), 200, yPosition, 1, 14);
	}

	int offSet = 30;
	int yPosition = 100;
	auto weaponData = ItemManager::itemManager.weaponData[inventory[currentOption]->ID];

	int xStatName = 620;
	int xStatValue = 660;
	text->RenderText("Type", xStatName, yPosition, 1);
	text->RenderTextRight(intToString2(weaponData.type), xStatValue, yPosition, 1, 14);
	yPosition += offSet;
	text->RenderText("Atk", xStatName, yPosition, 1);
	text->RenderTextRight(intToString2(selectedStats.attackDamage), xStatValue, yPosition, 1, 14);
	if (selectedStats.attackDamage > currentStats.attackDamage)
	{
		text->RenderTextRight("^^^", xStatValue + 60, yPosition, 1, 14);
	}
	yPosition += offSet;
	text->RenderText("Hit", xStatName, yPosition, 1);
	text->RenderTextRight(intToString2(selectedStats.hitAccuracy), xStatValue, yPosition, 1, 14);
	if (selectedStats.hitAccuracy > currentStats.hitAccuracy)
	{
		text->RenderTextRight("^^^", xStatValue + 60, yPosition, 1, 14);
	}
	yPosition += offSet;
	text->RenderText("Crit", xStatName, yPosition, 1);
	text->RenderTextRight(intToString2(selectedStats.hitCrit), xStatValue, yPosition, 1, 14);
	if (selectedStats.hitCrit > currentStats.hitCrit)
	{
		text->RenderTextRight("^^^", xStatValue + 60, yPosition, 1, 14);
	}
	yPosition += offSet;
	text->RenderText("Avo", xStatName, yPosition, 1);
	text->RenderTextRight(intToString2(selectedStats.hitAvoid), xStatValue, yPosition, 1, 14);
	if (selectedStats.hitAvoid > currentStats.hitAvoid)
	{
		text->RenderTextRight("^^^", xStatValue + 60, yPosition, 1, 14);
	}
}

void SelectWeaponMenu::SelectOption()
{
	Unit* unit = cursor->selectedUnit;
	std::cout << weapons[currentOption]->name << std::endl;
}

void SelectWeaponMenu::GetOptions()
{
	optionsVector.resize(cursor->selectedUnit->weapons.size());
}

void SelectWeaponMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	Menu::CheckInput(inputManager, deltaTime);

	if (inputManager.isKeyPressed(SDLK_UP) || inputManager.isKeyPressed(SDLK_DOWN))
	{
		GetBattleStats();
	}
}

void SelectWeaponMenu::GetBattleStats()
{
	Unit* unit = cursor->selectedUnit;
	auto weaponData = ItemManager::itemManager.weaponData[weapons[currentOption]->ID];

	currentStats = unit->CalculateBattleStats();
	selectedStats = unit->CalculateBattleStats(weaponData.ID);
}
