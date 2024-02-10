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
	}
	if (inputManager.isKeyPressed(SDLK_DOWN))
	{
		currentOption++;
		if (currentOption >= optionsVector.size())
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
		std::vector<std::vector<Unit*>> unitsToAttack;
		auto playerUnit = cursor->selectedUnit;
		//unitsToAttack.resize(validWeapons.size());
		for (int i = 0; i < playerUnit->weapons.size(); i++)
		{

			bool weaponInRange = false;
			auto weapon = playerUnit->GetWeaponData(playerUnit->weapons[i]);
			for (int c = 0; c < unitsInRange.size(); c++)
			{
				auto currentUnit = unitsInRange[c];

				float distance = abs(currentUnit->sprite.getPosition().x - playerUnit->sprite.getPosition().x) + abs(currentUnit->sprite.getPosition().y - playerUnit->sprite.getPosition().y);
				distance /= TileManager::TILE_SIZE;
				if (distance <= weapon.maxRange && distance >= weapon.minRange)
				{
					if (!weaponInRange)
					{
						weaponInRange = true;
						std::vector<Unit*> fuck;
						unitsToAttack.push_back(fuck);
						validWeapons.push_back(playerUnit->weapons[i]);
						unitsToAttack.back().push_back(currentUnit);
					}
					else
					{
						unitsToAttack.back().push_back(currentUnit);
					}
				}
			}
		}
		Menu* newMenu = new SelectWeaponMenu(cursor, text, camera, shapeVAO, validWeapons, unitsToAttack);
		MenuManager::menuManager.menus.push_back(newMenu);
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
		DrawWeaponComparison(inventory);
	}
	else
	{
		text->RenderText(inventory[currentOption]->description, 620, 100, 1);
	}
}

void ItemOptionsMenu::DrawWeaponComparison(std::vector<Item*>& inventory)
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
void MenuManager::SetUp(Cursor* Cursor, TextRenderer* Text, Camera* Camera, int shapeVAO, SpriteRenderer* Renderer)
{
	renderer = Renderer;
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

SelectWeaponMenu::SelectWeaponMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<Item*>& validWeapons, std::vector<std::vector<Unit*>>& units)
	: ItemOptionsMenu(Cursor, Text, camera, shapeVAO)
{
	weapons = validWeapons;
	unitsToAttack = units;
	GetOptions();
	GetBattleStats();
}

void SelectWeaponMenu::Draw()
{
	auto inventory = weapons;

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
	for (int i = 0; i < inventory.size(); i++)
	{
		int yPosition = 100 + i * 30;
		text->RenderText(inventory[i]->name, 96, yPosition, 1);
		text->RenderTextRight(intToString2(inventory[i]->remainingUses), 200, yPosition, 1, 14);
	}

	DrawWeaponComparison(weapons);
}

void SelectWeaponMenu::SelectOption()
{
	Unit* unit = cursor->selectedUnit;
	for (int i = 0; i < unit->inventory.size(); i++)
	{
		if (weapons[currentOption] == unit->inventory[i])
		{
			unit->equipWeapon(i);
			break;
		}
	}
	auto enemyUnits = unitsToAttack[currentOption];
	for (int i = 0; i < unitsToAttack[currentOption].size(); i++)
	{
		std::cout << enemyUnits[i]->name << std::endl;
	}
	Menu* newMenu = new SelectEnemyMenu(cursor, text, camera, shapeVAO, enemyUnits, MenuManager::menuManager.renderer);
	MenuManager::menuManager.menus.push_back(newMenu);
}

void SelectWeaponMenu::GetOptions()
{
	optionsVector.resize(weapons.size());
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

SelectEnemyMenu::SelectEnemyMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<Unit*>& units, SpriteRenderer* Renderer) : Menu(Cursor, Text, camera, shapeVAO)
{
	renderer = Renderer;
	unitsToAttack = units;
	CanEnemyCounter();

	GetOptions();
}

void SelectEnemyMenu::Draw()
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(169, 10, 0.0f));

	model = glm::scale(model, glm::vec3(80, 209, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(1.0f, 0.0f, 0.2f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	renderer->setUVs(cursor->uvs[1]);
	Texture2D displayTexture = ResourceManager::GetTexture("cursor");
	auto enemy = unitsToAttack[currentOption];
	Unit* unit = cursor->selectedUnit;
	renderer->DrawSprite(displayTexture, enemy->sprite.getPosition(), 0.0f, cursor->dimensions);

	int enemyStatsX = 536;

	text->RenderText(enemy->name, enemyStatsX, 100, 1);
	text->RenderText(enemy->inventory[0]->name, enemyStatsX, 130, 1); //need error checking here

	int statsY = 180;
	text->RenderTextRight(intToString2(enemy->level), enemyStatsX, statsY, 1, 14);
	text->RenderText("LV", enemyStatsX + 80, statsY, 1);
	text->RenderTextRight(intToString2(unit->level), enemyStatsX + 160, statsY, 1, 14);

	statsY += 30;
	text->RenderTextRight(intToString2(enemy->currentHP), enemyStatsX, statsY, 1, 14);
	text->RenderText("HP", enemyStatsX + 80, statsY, 1);
	text->RenderTextRight(intToString2(unit->currentHP), enemyStatsX + 160, statsY, 1, 14);

	statsY += 30;
	text->RenderTextRight(enemyStats.atk, enemyStatsX, statsY, 1, 14);
	text->RenderText("Atk", enemyStatsX + 80, statsY, 1);
	text->RenderTextRight(playerStats.atk, enemyStatsX + 160, statsY, 1, 14);

	statsY += 30;
	text->RenderTextRight(enemyStats.def, enemyStatsX, statsY, 1, 14);
	text->RenderText("Def", enemyStatsX + 80, statsY, 1);
	text->RenderTextRight(playerStats.def, enemyStatsX + 160, statsY, 1, 14);

	statsY += 30;
	text->RenderTextRight(enemyStats.hit, enemyStatsX, statsY, 1, 14);
	text->RenderText("Hit", enemyStatsX + 80, statsY, 1);
	text->RenderTextRight(playerStats.hit, enemyStatsX + 160, statsY, 1, 14);

	statsY += 30;
	text->RenderTextRight(enemyStats.crit, enemyStatsX, statsY, 1, 14);
	text->RenderText("Crit", enemyStatsX + 80, statsY, 1);
	text->RenderTextRight(playerStats.crit, enemyStatsX + 160, statsY, 1, 14);

	statsY += 30;
	text->RenderTextRight(enemyStats.attackSpeed, enemyStatsX, statsY, 1, 14);
	text->RenderText("AS", enemyStatsX + 80, statsY, 1);
	text->RenderTextRight(playerStats.attackSpeed, enemyStatsX + 160, statsY, 1, 14);

	text->RenderText(unit->name, enemyStatsX + 160, 500, 1);
}

void SelectEnemyMenu::SelectOption()
{
	std::cout << unitsToAttack[currentOption]->name << std::endl;
}

void SelectEnemyMenu::GetOptions()
{
	optionsVector.resize(unitsToAttack.size());
}

void SelectEnemyMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	Menu::CheckInput(inputManager, deltaTime);
	if (inputManager.isKeyPressed(SDLK_LEFT))
	{
		currentOption--;
		if (currentOption < 0)
		{
			currentOption = optionsVector.size() - 1;
		}
	}
	if (inputManager.isKeyPressed(SDLK_RIGHT))
	{
		currentOption++;
		if (currentOption >= optionsVector.size())
		{
			currentOption = 0;
		}
	}
	if (inputManager.isKeyPressed(SDLK_UP) || inputManager.isKeyPressed(SDLK_DOWN) || inputManager.isKeyPressed(SDLK_RIGHT) || inputManager.isKeyPressed(SDLK_LEFT))
	{
		CanEnemyCounter();
	}
}

void SelectEnemyMenu::CanEnemyCounter()
{
	auto enemy = unitsToAttack[currentOption];
	auto unit = cursor->selectedUnit;
	float attackDistance = abs(enemy->sprite.getPosition().x - unit->sprite.getPosition().x) + abs(enemy->sprite.getPosition().y - unit->sprite.getPosition().y);
	attackDistance /= TileManager::TILE_SIZE;
	auto enemyWeapon = enemy->GetWeaponData(enemy->inventory[0]);
	if (enemyWeapon.maxRange >= attackDistance && enemyWeapon.minRange <= attackDistance)
	{
		enemyCanCounter = true;
	}

	auto enemyNormalStats = enemy->CalculateBattleStats();
	enemyStats = DisplayedBattleStats{ intToString2(enemy->level), intToString2(enemy->currentHP), intToString2(enemyNormalStats.attackDamage), intToString2(enemy->defense), intToString2(enemyNormalStats.hitAccuracy), intToString2(enemyNormalStats.hitCrit), intToString2(enemyNormalStats.attackSpeed) };
	if (!enemyCanCounter)
	{
		enemyStats.hit = "--";
		enemyStats.atk = "--";
		enemyStats.crit = "--";
	}

	auto unitNormalStats = unit->CalculateBattleStats();

	playerStats = DisplayedBattleStats{ intToString2(unit->level), intToString2(unit->currentHP), intToString2(unitNormalStats.attackDamage), intToString2(unit->defense), intToString2(unitNormalStats.hitAccuracy), intToString2(unitNormalStats.hitCrit), intToString2(unitNormalStats.attackSpeed) };

}
