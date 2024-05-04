#include "MenuManager.h"
#include "Cursor.h"
#include "TextRenderer.h"
#include "Camera.h"
#include "InputManager.h"
#include "Items.h"
#include "BattleManager.h"
#include "Tile.h"
#include "SceneManager.h"

#include "Globals.h"

#include <SDL.h>


#include <sstream>
#include <algorithm> 

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

void Menu::EndUnitMove()
{
	if (cursor->selectedUnit->isMounted() && cursor->selectedUnit->mount->remainingMoves > 0)
	{
		cursor->GetRemainingMove();
	}
	else
	{
		cursor->Wait();
	}
	ClearMenu();

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
	auto playerUnit = cursor->selectedUnit;
	switch (optionsVector[currentOption])
	{
	case ATTACK:
	{
		std::vector<Item*> validWeapons;
		std::vector<std::vector<Unit*>> unitsToAttack;
		//Have a bug here where since the weapon array does not reorder on reequip, this can put things in an odd ordering
		//If I have weapon A equipped and it is a valid weapon, it should be the first in my list. But it is possible weapon B will be first if it is
		//First in this list. Need to resolve this.
		auto playerWeapons = playerUnit->GetOrderedWeapons();
		for (int i = 0; i < playerWeapons.size(); i++)
		{
			bool weaponInRange = false;
			auto weapon = playerUnit->GetWeaponData(playerWeapons[i]);
			if (playerUnit->canUse(weapon))
			{
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
							validWeapons.push_back(playerWeapons[i]);
							unitsToAttack.back().push_back(currentUnit);
						}
						else
						{
							unitsToAttack.back().push_back(currentUnit);
						}
					}
				}
			}
		}
		Menu* newMenu = new SelectWeaponMenu(cursor, text, camera, shapeVAO, validWeapons, unitsToAttack);
		MenuManager::menuManager.menus.push_back(newMenu);
		break;
	}
	case CAPTURE:
	{
		std::vector<Item*> validWeapons;
		std::vector<std::vector<Unit*>> unitsToAttack;
		for (int i = 0; i < playerUnit->weapons.size(); i++)
		{
			bool weaponInRange = false;
			auto weapon = playerUnit->GetWeaponData(playerUnit->weapons[i]);
			if (playerUnit->canUse(weapon))
			{
				if (1 <= weapon.maxRange && 1 >= weapon.minRange)
				{
					validWeapons.push_back(playerUnit->weapons[i]);
				}
			}
		}
		unitsToAttack.resize(validWeapons.size());
		for (int i = 0; i < unitsToAttack.size(); i++)
		{
			unitsToAttack[i] = unitsInCaptureRange;
		}
		Menu* newMenu = new SelectWeaponMenu(cursor, text, camera, shapeVAO, validWeapons, unitsToAttack, true);
		MenuManager::menuManager.menus.push_back(newMenu);
		break;
	}
	case DROP:
	{
		Menu* newMenu = new DropMenu(cursor, text, camera, shapeVAO, dropPositions, MenuManager::menuManager.renderer);
		MenuManager::menuManager.menus.push_back(newMenu);
	}
		break;
	case RELEASE:
	{
		playerUnit->carriedUnit->isDead = true;
		playerUnit->carriedUnit = nullptr;
		if (playerUnit->isMounted() && playerUnit->mount->remainingMoves > 0)
		{
			cursor->GetRemainingMove();
			MenuManager::menuManager.mustWait = true;
		}
		else
		{
			cursor->Wait();
		}
		playerUnit->carryingMalus = 1;
		heldEnemy = false;
		ClearMenu();
		break;
	}
	case RESCUE:
	{
		Menu* newMenu = new SelectRescueUnit(cursor, text, camera, shapeVAO, rescueUnits, MenuManager::menuManager.renderer);
		MenuManager::menuManager.menus.push_back(newMenu);
		break;
	}
	case TRANSFER:
	{
		Menu* newMenu = new SelectTransferUnit(cursor, text, camera, shapeVAO, transferUnits, MenuManager::menuManager.renderer);
		MenuManager::menuManager.menus.push_back(newMenu);
		break;
	}
	case TRADE:
	{
		Menu* newMenu = new SelectTradeUnit(cursor, text, camera, shapeVAO, tradeUnits, MenuManager::menuManager.renderer);
		MenuManager::menuManager.menus.push_back(newMenu);
		break;
	}
	case ITEMS:
		MenuManager::menuManager.AddMenu(1);
		break;
	case DISMOUNT:
	{
		playerUnit->MountAction(false);
		canDismount = false;
		optionsVector.erase(optionsVector.begin() + currentOption);
		numberOfOptions--;
		MenuManager::menuManager.mustWait = true;
		break;
	}
	case MOUNT:
	{
		playerUnit->MountAction(true);
		canMount = false;
		optionsVector.erase(optionsVector.begin() + currentOption);
		numberOfOptions--;
		MenuManager::menuManager.mustWait = true;
		break;
	}
	case TALK:
	{
		Menu* newMenu = new SelectTalkMenu(cursor, text, camera, shapeVAO, talkUnits, MenuManager::menuManager.renderer);
		MenuManager::menuManager.menus.push_back(newMenu);
		break;
	}
	case VISIT:
	{
		auto position = playerUnit->sprite.getPosition();
		auto visit = TileManager::tileManager.getVisit(position.x, position.y);
		int sceneID = -1;
		if (visit->sceneMap.count(playerUnit->sceneID))
		{
			sceneID = playerUnit->sceneID;
		}
		visit->sceneMap[sceneID]->activation->CheckActivation();
		ClearMenu();

		break;
	}
	//Wait
	default:
		cursor->Wait();
		ClearMenu();
		break;
	}
}

void UnitOptionsMenu::CancelOption()
{
	if (MenuManager::menuManager.mustWait)
	{
		EndUnitMove();
	}
	else
	{
		cursor->UndoMove();
		Menu::CancelOption();
	}
}

void UnitOptionsMenu::Draw()
{
	int xText = 600;
	int xIndicator = 176;
	int yOffset = 100;
	glm::vec2 fixedPosition = camera->worldToScreen(cursor->position);
	if (fixedPosition.x >= camera->screenWidth * 0.5f)
	{
		xText = 72;
		xIndicator = 8;
	}
	//ResourceManager::GetShader("shape").Use().SetMatrix4("projection", glm::ortho(0.0f, 800.0f, 600.0f, 0.0f));
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(xIndicator, 32 + (12 * currentOption), 0.0f));

	model = glm::scale(model, glm::vec3(16, 16, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
	//Just a little test with new line
	std::string commands = "";
	if (canTalk)
	{
		text->RenderText("Talk", xText, yOffset, 1);
		yOffset += 30;
	}
	if (canAttack)
	{
		commands += "Attack\n";
		text->RenderText("Attack", xText, yOffset, 1);
		yOffset += 30;
	}
	if (canVisit)
	{
		text->RenderText("Visit", xText, yOffset, 1);
		yOffset += 30;
	}
	if (heldFriendly)
	{
		text->RenderText("Drop", xText, yOffset, 1);
		yOffset += 30;
	}
	else if (heldEnemy)
	{
		text->RenderText("Release", xText, yOffset, 1);
		yOffset += 30;
	}
	else
	{
		if (canCapture)
		{
			text->RenderText("Capture", xText, yOffset, 1);
			yOffset += 30;
		}
		if (canRescue)
		{
			text->RenderText("Rescue", xText, yOffset, 1);
			yOffset += 30;
		}
	}
	if (canTransfer)
	{
		text->RenderText("Transfer", xText, yOffset, 1);
		yOffset += 30;
	}
	commands += "Items\n";
	text->RenderText("Items", xText, yOffset, 1);
	yOffset += 30;
	if (canTrade)
	{
		text->RenderText("Trade", xText, yOffset, 1);
		yOffset += 30;
	}
	if (canDismount)
	{
		commands += "Dismount\n";
		text->RenderText("Dismount", xText, yOffset, 1);
		yOffset += 30;
	}
	else if (canMount)
	{
		commands += "Mount\n";
		text->RenderText("Mount", xText, yOffset, 1);
		yOffset += 30;
	}
	commands += "Wait";
	text->RenderText("Wait", xText, yOffset, 1);
	//Text->RenderText(commands, xText, 100, 1);
}

void UnitOptionsMenu::GetOptions()
{
	currentOption = 0;
	canAttack = false;
	canCapture = false;
	canDismount = false;
	heldFriendly = false;
	heldEnemy = false;
	canTransfer = false;
	canVisit = false;
	optionsVector.clear();
	optionsVector.reserve(5);
	auto playerUnit = cursor->selectedUnit;
	auto playerPosition = playerUnit->sprite.getPosition();

	unitsInRange = playerUnit->inRangeUnits(1);
	unitsInCaptureRange.clear();
	if (unitsInRange.size() > 0)
	{
		canAttack = true;
		optionsVector.push_back(ATTACK);
		if (!playerUnit->carriedUnit)
		{
			unitsInCaptureRange.reserve(unitsInRange.size());
			for (int i = 0; i < unitsInRange.size(); i++)
			{
				auto currentUnit = unitsInRange[i];
				if (!currentUnit->isMounted() && currentUnit->getBuild() < 20)
				{
					float distance = abs(currentUnit->sprite.getPosition().x - playerPosition.x) + abs(currentUnit->sprite.getPosition().y - playerPosition.y);
					distance /= TileManager::TILE_SIZE;
					if (distance == 1 && (playerUnit->isMounted() || currentUnit->getBuild() < playerUnit->getBuild()))
					{
						unitsInCaptureRange.push_back(currentUnit);
					}
				}
			}
			if (unitsInCaptureRange.size() > 0)
			{
				canCapture = true;
				optionsVector.push_back(CAPTURE);
			}
		}
	}
	if (TileManager::tileManager.getVisit(playerPosition.x, playerPosition.y))
	{
		canVisit = true;
		optionsVector.push_back(VISIT);
	}
	if (playerUnit->carriedUnit)
	{
		if (playerUnit->carriedUnit->team == playerUnit->team)
		{
			dropPositions = cursor->getDropPositions();
			if (dropPositions.size() > 0)
			{
				optionsVector.push_back(DROP);
				heldFriendly = true;
			}
		}
		else
		{
			optionsVector.push_back(RELEASE);
			heldEnemy = true;
		}
	}
	optionsVector.push_back(ITEMS);
	cursor->GetAdjacentUnits(tradeUnits, talkUnits);
	rescueUnits.clear();
	transferUnits.clear();
	if (tradeUnits.size() > 0)
	{
		canTrade = true;
		optionsVector.push_back(TRADE);
		transferUnits.reserve(tradeUnits.size());
		if (!playerUnit->carriedUnit)
		{
			rescueUnits.reserve(tradeUnits.size());
			for (int i = 0; i < tradeUnits.size(); i++)
			{
				auto currentUnit = tradeUnits[i];
				if (!currentUnit->isCarried)
				{
					if (!currentUnit->isMounted() && currentUnit->getBuild() < 20 && !currentUnit->carriedUnit)
					{
						if (currentUnit->getBuild() < playerUnit->getBuild())
						{
							rescueUnits.push_back(currentUnit);
						}
					}
					else if (currentUnit->carriedUnit)
					{
						if (currentUnit->carriedUnit->getBuild() < playerUnit->getBuild())
						{
							transferUnits.push_back(currentUnit);
						}
					}
				}
			}
			if (rescueUnits.size() > 0)
			{
				canRescue = true;
				optionsVector.insert(optionsVector.begin() + optionsVector.size() - 2, RESCUE);
			}
			if (transferUnits.size() > 0)
			{
				canTransfer = true;
				optionsVector.insert(optionsVector.begin() + optionsVector.size() - 2, TRANSFER);
			}
		}
		else
		{
			for (int i = 0; i < tradeUnits.size(); i++)
			{
				auto currentUnit = tradeUnits[i];
				if (!currentUnit->isCarried)
				{
					if (!currentUnit->carriedUnit)
					{
						if (playerUnit->carriedUnit->getBuild() < currentUnit->getBuild())
						{
							transferUnits.push_back(currentUnit);
						}
					}
				}
			}
			if (transferUnits.size() > 0)
			{
				canTransfer = true;
				optionsVector.insert(optionsVector.begin() + optionsVector.size() - 2, TRANSFER);
			}
		}
	}
	if (talkUnits.size() > 0)
	{
		canTalk = true;
		optionsVector.insert(optionsVector.begin(), TALK);
	}
	//if can dismount
	if (playerUnit->mount)
	{
		if (playerUnit->mount->mounted)
		{
			canDismount = true;
			optionsVector.push_back(DISMOUNT);
		}
		else
		{
			canMount = true;
			optionsVector.push_back(MOUNT);

		}
	}
	optionsVector.push_back(WAIT);
	numberOfOptions = optionsVector.size();
}

CantoOptionsMenu::CantoOptionsMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO) : Menu(Cursor, Text, camera, shapeVAO)
{
	numberOfOptions = 1;
}

void CantoOptionsMenu::Draw()
{
	int xText = 600;
	int xIndicator = 176;
	int yOffset = 100;
	glm::vec2 fixedPosition = camera->worldToScreen(cursor->position);
	if (fixedPosition.x >= camera->screenWidth * 0.5f)
	{
		xText = 72;
		xIndicator = 8;
	}
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(xIndicator, 32 + (12 * currentOption), 0.0f));

	model = glm::scale(model, glm::vec3(16, 16, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
	text->RenderText("Wait", xText, yOffset, 1);
}

void CantoOptionsMenu::SelectOption()
{
	cursor->Wait();
	ClearMenu();
}

void CantoOptionsMenu::CancelOption()
{
	cursor->UndoRemainingMove();
	Menu::CancelOption();
}

ItemOptionsMenu::ItemOptionsMenu(Cursor* Cursor, TextRenderer* Text, Camera* Camera, int shapeVAO) : Menu(Cursor, Text, Camera, shapeVAO)
{
	GetOptions();
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

	//Duplicated this down in ItemUseMenu's Draw.
	Unit* unit = cursor->selectedUnit;
	auto inventory = unit->inventory;
	glm::vec3 color = glm::vec3(1);
	glm::vec3 grey = glm::vec3(0.64f);
	for (int i = 0; i < inventory.size(); i++)
	{
		color = glm::vec3(1);
		int yPosition = 100 + i * 30;
		auto item = inventory[i];
		if (item->isWeapon && !unit->canUse(unit->GetWeaponData(item)))
		{
			color = grey;
		}
		text->RenderText(item->name, 96, yPosition, 1, color);
		text->RenderTextRight(intToString(item->remainingUses), 200, yPosition, 1, 14, color);
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
	text->RenderTextRight(intToString(weaponData.type), xStatValue, yPosition, 1, 14);
	yPosition += offSet;
	text->RenderText("Atk", xStatName, yPosition, 1);
	text->RenderTextRight(intToString(selectedStats.attackDamage), xStatValue, yPosition, 1, 14);
	if (selectedStats.attackDamage > currentStats.attackDamage)
	{
		text->RenderTextRight("^^^", xStatValue + 60, yPosition, 1, 14);
	}
	yPosition += offSet;
	text->RenderText("Hit", xStatName, yPosition, 1);
	text->RenderTextRight(intToString(selectedStats.hitAccuracy), xStatValue, yPosition, 1, 14);
	if (selectedStats.hitAccuracy > currentStats.hitAccuracy)
	{
		text->RenderTextRight("^^^", xStatValue + 60, yPosition, 1, 14);
	}
	yPosition += offSet;
	text->RenderText("Crit", xStatName, yPosition, 1);
	text->RenderTextRight(intToString(selectedStats.hitCrit), xStatValue, yPosition, 1, 14);
	if (selectedStats.hitCrit > currentStats.hitCrit)
	{
		text->RenderTextRight("^^^", xStatValue + 60, yPosition, 1, 14);
	}
	yPosition += offSet;
	text->RenderText("Avo", xStatName, yPosition, 1);
	text->RenderTextRight(intToString(selectedStats.hitAvoid), xStatValue, yPosition, 1, 14);
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
	numberOfOptions = cursor->selectedUnit->inventory.size();
	GetBattleStats();
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

	currentStats = unit->CalculateBattleStats();
	selectedStats = unit->CalculateBattleStats(inventory[currentOption]->ID);
}

MenuManager MenuManager::menuManager;
void MenuManager::SetUp(Cursor* Cursor, TextRenderer* Text, Camera* Camera, int shapeVAO, SpriteRenderer* Renderer, BattleManager* battleManager)
{
	renderer = Renderer;
	cursor = Cursor;
	text = Text;
	camera = Camera;
	this->battleManager = battleManager;
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
	else if (ID == 3)
	{
		Menu* newMenu = new ExtraMenu(cursor, text, camera, shapeVAO);
		menus.push_back(newMenu);
	}
	else if (ID == 4)
	{
		Menu* newMenu = new CantoOptionsMenu(cursor, text, camera, shapeVAO);
		menus.push_back(newMenu);
	}
}

void MenuManager::AddUnitStatMenu(Unit* unit)
{
	Menu* newMenu = new UnitStatsViewMenu(cursor, text, camera, shapeVAO, unit, renderer);
	menus.push_back(newMenu);
}

void MenuManager::PreviousMenu()
{
	Menu* p = menus.back();
	menus.pop_back();
	delete p;
}

void MenuManager::ClearMenu()
{
	mustWait = false;
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
	glm::vec3 color = glm::vec3(1);
	glm::vec3 grey = glm::vec3(0.64f);
	for (int i = 0; i < inventory.size(); i++)
	{
		color = glm::vec3(1);
		int yPosition = 100 + i * 30;
		auto item = inventory[i];
		if (item->isWeapon && !unit->canUse(unit->GetWeaponData(item)))
		{
			color = grey;
		}
		text->RenderText(item->name, 96, yPosition, 1, color);
		text->RenderTextRight(intToString(item->remainingUses), 200, yPosition, 1, 14, color);
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
	color = glm::vec3(1);
	if (!item->canDrop)
	{
		color = grey;
	}
	text->RenderText("Drop", 600, yOffset, 1, color);
}

void ItemUseMenu::SelectOption()
{
	switch (optionsVector[currentOption])
	{
	case USE:
		ItemManager::itemManager.UseItem(cursor->selectedUnit, inventoryIndex);
		ClearMenu();
		break;
	case EQUIP:
		cursor->selectedUnit->equipWeapon(inventoryIndex);
		MenuManager::menuManager.PreviousMenu();
		MenuManager::menuManager.PreviousMenu();
		//swap equipment
		break;
	case DROP:
		if (item->canDrop)
		{
			cursor->selectedUnit->dropItem(inventoryIndex);
			//Going back to the main selection menu is how FE5 does it, not sure if I want to keep that.
			MenuManager::menuManager.PreviousMenu();
			MenuManager::menuManager.PreviousMenu();
			MenuManager::menuManager.menus.back()->GetOptions();
		}
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
		auto unit = cursor->selectedUnit;
		if (unit->canUse(unit->GetWeaponData(item)))
		{
			optionsVector.push_back(EQUIP);
			canEquip = true;
		}
	}
	optionsVector.push_back(DROP);
	numberOfOptions = optionsVector.size();
}

SelectWeaponMenu::SelectWeaponMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<Item*>& validWeapons, std::vector<std::vector<Unit*>>& units, bool capturing)
	: ItemOptionsMenu(Cursor, Text, camera, shapeVAO), capturing(capturing)
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
		text->RenderTextRight(intToString(inventory[i]->remainingUses), 200, yPosition, 1, 14);
	}

	DrawWeaponComparison(weapons);
}

void SelectWeaponMenu::SelectOption()
{
	Unit* unit = cursor->selectedUnit;
	int selectedWeapon = 0;
	for (int i = 0; i < unit->inventory.size(); i++)
	{
		if (weapons[currentOption] == unit->inventory[i])
		{
			selectedWeapon = i;
	//		unit->equipWeapon(i);
			break;
		}
	}
	auto enemyUnits = unitsToAttack[currentOption];
	for (int i = 0; i < unitsToAttack[currentOption].size(); i++)
	{
		std::cout << enemyUnits[i]->name << std::endl;
	}
	Menu* newMenu = new SelectEnemyMenu(cursor, text, camera, shapeVAO, enemyUnits, MenuManager::menuManager.renderer, selectedWeapon, capturing);
	MenuManager::menuManager.menus.push_back(newMenu);
}

void SelectWeaponMenu::GetOptions()
{
	numberOfOptions = weapons.size();
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

	currentStats = unit->CalculateBattleStats();
	selectedStats = unit->CalculateBattleStats(weapons[currentOption]->ID);
}

SelectEnemyMenu::SelectEnemyMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<Unit*>& units, SpriteRenderer* Renderer, int selectedWeapon, bool capturing) :
	Menu(Cursor, Text, camera, shapeVAO), selectedWeapon(selectedWeapon), capturing(capturing)
{
	renderer = Renderer;
	unitsToAttack = units;
	if (capturing)
	{
		cursor->selectedUnit->carryingMalus = 2;
	}
	CanEnemyCounter(capturing);

	GetOptions();
}

void SelectEnemyMenu::Draw()
{
	auto enemy = unitsToAttack[currentOption];
	auto targetPosition = enemy->sprite.getPosition();
	int enemyStatsTextX = 536;
	int statsDisplay = 169;

	int yOffset = 100;
	glm::vec2 fixedPosition = camera->worldToScreen(targetPosition);
	if (fixedPosition.x >= camera->screenWidth * 0.5f)
	{
		enemyStatsTextX = 32;
		statsDisplay = 7;
	}
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(statsDisplay, 10, 0.0f));

	model = glm::scale(model, glm::vec3(80, 209, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(1.0f, 0.0f, 0.2f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	renderer->setUVs(cursor->uvs[1]);
	Texture2D displayTexture = ResourceManager::GetTexture("cursor");
	
	Unit* unit = cursor->selectedUnit;
	renderer->DrawSprite(displayTexture, targetPosition, 0.0f, cursor->dimensions);


	text->RenderText(enemy->name, enemyStatsTextX, 100, 1);
	if (auto enemyWeapon = enemy->GetEquippedItem())
	{
		text->RenderText(enemyWeapon->name, enemyStatsTextX, 130, 1); //need error checking here
	}

	int statsY = 180;
	text->RenderTextRight(intToString(enemy->level), enemyStatsTextX, statsY, 1, 14);
	text->RenderText("LV", enemyStatsTextX + 80, statsY, 1);
	text->RenderTextRight(intToString(unit->level), enemyStatsTextX + 160, statsY, 1, 14);

	statsY += 30;
	text->RenderTextRight(intToString(enemy->currentHP), enemyStatsTextX, statsY, 1, 14);
	text->RenderText("HP", enemyStatsTextX + 80, statsY, 1);
	text->RenderTextRight(intToString(unit->currentHP), enemyStatsTextX + 160, statsY, 1, 14);

	statsY += 30;
	text->RenderTextRight(enemyStats.atk, enemyStatsTextX, statsY, 1, 14);
	text->RenderText("Atk", enemyStatsTextX + 80, statsY, 1);
	text->RenderTextRight(playerStats.atk, enemyStatsTextX + 160, statsY, 1, 14);

	statsY += 30;
	text->RenderTextRight(enemyStats.def, enemyStatsTextX, statsY, 1, 14);
	text->RenderText("Def", enemyStatsTextX + 80, statsY, 1);
	text->RenderTextRight(playerStats.def, enemyStatsTextX + 160, statsY, 1, 14);

	statsY += 30;
	text->RenderTextRight(enemyStats.hit, enemyStatsTextX, statsY, 1, 14);
	text->RenderText("Hit", enemyStatsTextX + 80, statsY, 1);
	text->RenderTextRight(playerStats.hit, enemyStatsTextX + 160, statsY, 1, 14);

	statsY += 30;
	text->RenderTextRight(enemyStats.crit, enemyStatsTextX, statsY, 1, 14);
	text->RenderText("Crit", enemyStatsTextX + 80, statsY, 1);
	text->RenderTextRight(playerStats.crit, enemyStatsTextX + 160, statsY, 1, 14);

	statsY += 30;
	text->RenderTextRight(enemyStats.attackSpeed, enemyStatsTextX, statsY, 1, 14);
	text->RenderText("AS", enemyStatsTextX + 80, statsY, 1);
	text->RenderTextRight(playerStats.attackSpeed, enemyStatsTextX + 160, statsY, 1, 14);

	text->RenderText(unit->name, enemyStatsTextX + 160, 500, 1);
}

void SelectEnemyMenu::SelectOption()
{
	std::cout << unitsToAttack[currentOption]->name << std::endl;
	cursor->selectedUnit->equipWeapon(selectedWeapon);
	MenuManager::menuManager.battleManager->SetUp(cursor->selectedUnit, unitsToAttack[currentOption], unitNormalStats, enemyNormalStats, enemyCanCounter, *camera, false, capturing);
	cursor->MoveUnitToTile();
	if (cursor->selectedUnit->isMounted() && cursor->selectedUnit->mount->remainingMoves > 0)
	{
	//	cursor->GetRemainingMove();
	}
	else
	{
	//	cursor->Wait();
	}
//	MenuManager::menuManager.camera->SetMove(cursor->position); Not sure about this, need to have the camera move so it centers the units
	ClearMenu();
}

void SelectEnemyMenu::GetOptions()
{
	numberOfOptions = unitsToAttack.size();
}

void SelectEnemyMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	Menu::CheckInput(inputManager, deltaTime);
	if (inputManager.isKeyPressed(SDLK_LEFT))
	{
		currentOption--;
		if (currentOption < 0)
		{
			currentOption = numberOfOptions - 1;
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
	if (inputManager.isKeyPressed(SDLK_UP) || inputManager.isKeyPressed(SDLK_DOWN) || inputManager.isKeyPressed(SDLK_RIGHT) || inputManager.isKeyPressed(SDLK_LEFT))
	{
		CanEnemyCounter();
	}
}

void SelectEnemyMenu::CancelOption()
{
	//Obvious but stupid solution to resetting this malus after cancelling a capture action
	if (capturing)
	{
		cursor->selectedUnit->carryingMalus = 1;
	}
	Menu::CancelOption();
	Menu::CancelOption();
}

void SelectEnemyMenu::CanEnemyCounter(bool capturing /*= false */)
{
	auto enemy = unitsToAttack[currentOption];
	auto unit = cursor->selectedUnit;
	float attackDistance = abs(enemy->sprite.getPosition().x - unit->sprite.getPosition().x) + abs(enemy->sprite.getPosition().y - unit->sprite.getPosition().y);
	attackDistance /= TileManager::TILE_SIZE;
	auto enemyWeapon = enemy->GetEquippedWeapon();
	enemyCanCounter = false;
	if (enemyWeapon.maxRange >= attackDistance && enemyWeapon.minRange <= attackDistance)
	{
		enemyCanCounter = true;
	}

	enemyNormalStats = enemy->CalculateBattleStats();

	unitNormalStats = unit->CalculateBattleStats(unit->inventory[selectedWeapon]->ID);
	auto unitWeapon = unit->GetWeaponData(unit->inventory[selectedWeapon]);
	//int playerDefense = unit->defense;
	//int enemyDefense = enemy->defense;	
	enemy->CalculateMagicDefense(enemyWeapon, enemyNormalStats, attackDistance);
	unit->CalculateMagicDefense(unitWeapon, unitNormalStats, attackDistance);
	int playerDefense = enemyNormalStats.attackType == 0 ? unit->getDefense() : unit->getMagic();
	int enemyDefense = unitNormalStats.attackType == 0 ? enemy->getDefense() : enemy->getMagic();

	
	//Magic resistance stat is just the unit's magic stat
	/*if (unitWeapon.isMagic)
	{
		//Magic swords such as the Light Brand do physical damage when used in close range
		//so what I'm doing is just negating the previous damage calculation and using strength instead
		if (attackDistance == 1 && !unitWeapon.isTome)
		{
			unitNormalStats.attackDamage -= unit->magic;
			unitNormalStats.attackDamage += unit->strength;
		}
		else
		{
			enemyDefense = enemy->magic;
		}
	}

	if (enemyWeapon.isMagic)
	{
		if (attackDistance == 1 && !enemyWeapon.isTome)
		{
			enemyNormalStats.attackDamage -= enemy->magic;
			enemyNormalStats.attackDamage += enemy->strength;
		}
		else
		{
			playerDefense = unit->magic;
		}
	}*/

	auto playerPosition = unit->sprite.getPosition();
	auto playerTile = TileManager::tileManager.getTile(playerPosition.x, playerPosition.y);
	playerDefense += playerTile->properties.defense;
	unitNormalStats.hitAvoid += playerTile->properties.avoid;
	auto enemyPosition = enemy->sprite.getPosition();
	auto enemyTile = TileManager::tileManager.getTile(enemyPosition.x, enemyPosition.y);
	enemyDefense += enemyTile->properties.defense;
	enemyNormalStats.hitAvoid += enemyTile->properties.avoid;

	//Physical weapon triangle bonus
	if (unitWeapon.type == WeaponData::TYPE_SWORD)
	{
		if (enemyWeapon.type == WeaponData::TYPE_AXE)
		{
			unitNormalStats.hitAccuracy += 5;
			enemyNormalStats.hitAccuracy -= 5;
		}
		else if (enemyWeapon.type == WeaponData::TYPE_LANCE)
		{
			unitNormalStats.hitAccuracy -= 5;
			enemyNormalStats.hitAccuracy += 5;
		}
	}
	else if (unitWeapon.type == WeaponData::TYPE_AXE)
	{
		if (enemyWeapon.type == WeaponData::TYPE_LANCE)
		{
			unitNormalStats.hitAccuracy += 5;
			enemyNormalStats.hitAccuracy -= 5;
		}
		else if (enemyWeapon.type == WeaponData::TYPE_SWORD)
		{
			unitNormalStats.hitAccuracy -= 5;
			enemyNormalStats.hitAccuracy += 5;
		}
	}
	else if (unitWeapon.type == WeaponData::TYPE_LANCE)
	{
		if (enemyWeapon.type == WeaponData::TYPE_SWORD)
		{
			unitNormalStats.hitAccuracy += 5;
			enemyNormalStats.hitAccuracy -= 5;
		}
		else if (enemyWeapon.type == WeaponData::TYPE_AXE)
		{
			unitNormalStats.hitAccuracy -= 5;
			enemyNormalStats.hitAccuracy += 5;
		}
	}
	unitNormalStats.hitAccuracy -= enemyNormalStats.hitAvoid;
	enemyNormalStats.hitAccuracy -= unitNormalStats.hitAvoid;

	unitNormalStats.hitAccuracy = std::max(0, unitNormalStats.hitAccuracy);

	enemyNormalStats.hitAccuracy = std::max(0, enemyNormalStats.hitAccuracy);

	int unitCritEvade = unit->getLuck() / 2;
	int enemyCritEvade = enemy->getLuck() / 2;

	unitNormalStats.hitCrit -= enemyCritEvade;
	enemyNormalStats.hitCrit -= unitCritEvade;
	unitNormalStats.hitCrit = std::min(unitNormalStats.hitCrit, 25);
	unitNormalStats.hitCrit = std::max(0, unitNormalStats.hitCrit);

	enemyNormalStats.hitCrit = std::min(enemyNormalStats.hitCrit, 25);
	enemyNormalStats.hitCrit = std::max(0, enemyNormalStats.hitCrit);

	playerStats = DisplayedBattleStats{ intToString(unit->level), intToString(unit->currentHP), intToString(unitNormalStats.attackDamage), intToString(playerDefense), intToString(unitNormalStats.hitAccuracy), intToString(unitNormalStats.hitCrit), intToString(unitNormalStats.attackSpeed) };

	enemyStats = DisplayedBattleStats{ intToString(enemy->level), intToString(enemy->currentHP), intToString(enemyNormalStats.attackDamage), intToString(enemyDefense), intToString(enemyNormalStats.hitAccuracy), intToString(enemyNormalStats.hitCrit), intToString(enemyNormalStats.attackSpeed) };
	if (!enemyCanCounter)
	{
		enemyStats.hit = "--";
		enemyStats.atk = "--";
		enemyStats.crit = "--";
	}
}

SelectTradeUnit::SelectTradeUnit(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<Unit*>& units, SpriteRenderer* Renderer) : Menu(Cursor, Text, camera, shapeVAO)
{
	renderer = Renderer;
	tradeUnits = units;
	GetOptions();
}

void SelectTradeUnit::Draw()
{
	auto tradeUnit = tradeUnits[currentOption];
	int inventorySize = tradeUnit->inventory.size();
	int boxHeight = 30;
	if (inventorySize > 0)
	{
		boxHeight += (inventorySize + 1) * 30;
	}
	auto targetPosition = tradeUnit->sprite.getPosition();
	int xText = 536;
	int xIndicator = 169;
	glm::vec2 fixedPosition = camera->worldToScreen(targetPosition);
	if (fixedPosition.x >= camera->screenWidth * 0.5f)
	{
		xText = 32;
		xIndicator = 7;
	}

	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(xIndicator, 10, 0.0f));

	model = glm::scale(model, glm::vec3(80, boxHeight, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.2f, 0.0f, 1.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	renderer->setUVs(cursor->uvs[1]);
	Texture2D displayTexture = ResourceManager::GetTexture("cursor");
	Unit* unit = cursor->selectedUnit;
	renderer->DrawSprite(displayTexture, targetPosition, 0.0f, cursor->dimensions);

	int textHeight = 100;
	text->RenderText(tradeUnit->name, xText, textHeight, 1);
	textHeight += 60;
	for (int i = 0; i < inventorySize; i++)
	{
		text->RenderText(tradeUnit->inventory[i]->name, xText, textHeight, 1);
		text->RenderTextRight(intToString(tradeUnit->inventory[i]->remainingUses), xText + 100, textHeight, 1, 14);
		textHeight += 30;
	}
}

void SelectTradeUnit::SelectOption()
{
	Menu* newMenu = new TradeMenu(cursor, text, camera, shapeVAO, tradeUnits[currentOption]);
	MenuManager::menuManager.menus.push_back(newMenu);
}

void SelectTradeUnit::GetOptions()
{
	numberOfOptions = tradeUnits.size();
}

void SelectTradeUnit::CheckInput(InputManager& inputManager, float deltaTime)
{
	Menu::CheckInput(inputManager, deltaTime);
	if (inputManager.isKeyPressed(SDLK_LEFT))
	{
		currentOption--;
		if (currentOption < 0)
		{
			currentOption = numberOfOptions - 1;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_RIGHT))
	{
		currentOption++;
		if (currentOption >= numberOfOptions)
		{
			currentOption = 0;
		}
	}
}

SelectTalkMenu::SelectTalkMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<Unit*>& units, SpriteRenderer* Renderer) : Menu(Cursor, Text, camera, shapeVAO)
{
	renderer = Renderer;
	talkUnits = units;
	GetOptions();
}

void SelectTalkMenu::Draw()
{
	auto talkUnit = talkUnits[currentOption];

	auto targetPosition = talkUnit->sprite.getPosition();
	int xText = 536;
	int xIndicator = 169;
	int boxHeight = 96;
	glm::vec2 fixedPosition = camera->worldToScreen(targetPosition);
	if (fixedPosition.x >= camera->screenWidth * 0.5f)
	{
		xText = 32;
		xIndicator = 7;
	}

	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(xIndicator, 10, 0.0f));

	model = glm::scale(model, glm::vec3(80, boxHeight, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.2f, 0.0f, 1.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	renderer->setUVs(cursor->uvs[1]);
	Texture2D displayTexture = ResourceManager::GetTexture("cursor");
	Unit* unit = cursor->selectedUnit;
	renderer->DrawSprite(displayTexture, targetPosition, 0.0f, cursor->dimensions);

	int textHeight = 100;
	text->RenderText(talkUnit->name, xText, textHeight, 1);
	textHeight += 30;
	text->RenderText(talkUnit->unitClass, xText, textHeight, 1);
	textHeight += 30;
	if (auto weapon = talkUnit->GetEquippedItem())
	{
		text->RenderText(weapon->name, xText, textHeight, 1);
	}
	textHeight += 30;
	text->RenderText(intToString(talkUnit->level), xText + 40, textHeight, 1);
	textHeight += 30;
	text->RenderText("HP" + intToString(talkUnit->maxHP) + "/" + intToString(talkUnit->currentHP), xText, textHeight, 1);
}

void SelectTalkMenu::SelectOption()
{
	cursor->selectedUnit->talkData[0].scene->activation->CheckActivation();
	ClearMenu();
}

void SelectTalkMenu::GetOptions()
{
	numberOfOptions = talkUnits.size();
}

void SelectTalkMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	Menu::CheckInput(inputManager, deltaTime);
	if (inputManager.isKeyPressed(SDLK_LEFT))
	{
		currentOption--;
		if (currentOption < 0)
		{
			currentOption = numberOfOptions - 1;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_RIGHT))
	{
		currentOption++;
		if (currentOption >= numberOfOptions)
		{
			currentOption = 0;
		}
	}
}

TradeMenu::TradeMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, Unit* unit) : Menu(Cursor, Text, camera, shapeVAO)
{
	tradeUnit = unit;
	GetOptions();
}

void TradeMenu::Draw()
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(0, 0, 0.0f));

	model = glm::scale(model, glm::vec3(256, 224, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.2f, 0.0f, 1.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	model = glm::mat4();
	int x = 0;
	if (firstInventory)
	{
		x = 32;
	}
	else
	{
		x = 208;
	}
	model = glm::translate(model, glm::vec3(x, 64 + (12 * currentOption), 0.0f));

	model = glm::scale(model, glm::vec3(16, 16, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	if (moving)
	{
		model = glm::mat4();
		if (moveFromFirst)
		{
			x = 32;
		}
		else
		{
			x = 208;
		}
		model = glm::translate(model, glm::vec3(x, 64 + (12 * itemToMove), 0.0f));

		model = glm::scale(model, glm::vec3(16, 16, 0.0f));

		ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

		ResourceManager::GetShader("shape").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
	}
	auto firstUnit = cursor->selectedUnit;
	text->RenderText(firstUnit->name, 100, 30, 1);
	text->RenderText(tradeUnit->name, 700, 30, 1);


	for (int i = 0; i < firstUnit->inventory.size(); i++)
	{
		text->RenderText(firstUnit->inventory[i]->name, 130, 180 + i * 30, 1);
		text->RenderTextRight(intToString(firstUnit->inventory[i]->remainingUses), 230, 180 + i * 30, 1, 14);
	}

	for (int i = 0; i < tradeUnit->inventory.size(); i++)
	{
		text->RenderText(tradeUnit->inventory[i]->name, 670, 180 + i * 30, 1);
		text->RenderTextRight(intToString(tradeUnit->inventory[i]->remainingUses), 770, 180 + i * 30, 1, 14);
	}
}

void TradeMenu::SelectOption()
{
	if (moving == false)
	{
		if (firstInventory)
		{
			moveFromFirst = true;
			firstInventory = false;
		}
		else
		{
			firstInventory = true;
			moveFromFirst = false;
		}
		moving = true;
		itemToMove = currentOption;		
		GetOptions();
		currentOption = numberOfOptions - 1;
	}
	else
	{
		MenuManager::menuManager.mustWait = true;
		moving = false;
		bool emptyInventory = false;
		auto firstUnit = cursor->selectedUnit;
		if (moveFromFirst && !firstInventory)
		{
			tradeUnit->swapItem(firstUnit, itemToMove, currentOption);
			if (firstUnit->inventory.size() > 0)
			{
				firstInventory = true;
			}
			else
			{
				emptyInventory = true;
			}
		}
		else if (moveFromFirst && firstInventory)
		{
			firstUnit->swapItem(firstUnit, itemToMove, currentOption);
			if (itemToMove == firstUnit->equippedWeapon)
			{
				firstUnit->equippedWeapon = currentOption;
			}
			else if (currentOption == firstUnit->equippedWeapon)
			{
				firstUnit->equippedWeapon = itemToMove;
			}
		}
		else if (!moveFromFirst && firstInventory)
		{
			firstUnit->swapItem(tradeUnit, itemToMove, currentOption);
			if (tradeUnit->inventory.size() > 0)
			{
				firstInventory = false;
			}
			else
			{
				emptyInventory = true;
			}
		}
		else if (!moveFromFirst && !firstInventory)
		{
			tradeUnit->swapItem(tradeUnit, itemToMove, currentOption);
			if (itemToMove == tradeUnit->equippedWeapon)
			{
				tradeUnit->equippedWeapon = currentOption;
			}
			else if (currentOption == tradeUnit->equippedWeapon)
			{
				tradeUnit->equippedWeapon = itemToMove;
			}
		}
		GetOptions();
		if (emptyInventory)
		{
			currentOption = 0;
		}
		else
		{
			currentOption = numberOfOptions - 1;
		}
	}
}

void TradeMenu::GetOptions()
{
	if (firstInventory)
	{
		numberOfOptions = cursor->selectedUnit->inventory.size();
		if (moving && !moveFromFirst)
		{
			if (numberOfOptions < 8)
			{
				numberOfOptions++;
			}
		}
	}
	else
	{
		numberOfOptions = tradeUnit->inventory.size();
		if (moving && moveFromFirst)
		{
			if (numberOfOptions < 8)
			{
				numberOfOptions++;
			}
		}
	}
}

void TradeMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	Menu::CheckInput(inputManager, deltaTime);
	if (inputManager.isKeyPressed(SDLK_LEFT) && !firstInventory)
	{
		firstInventory = true;
		auto firstInv = cursor->selectedUnit->inventory;
		if (tradeUnit->inventory.size() > firstInv.size())
		{
			currentOption = firstInv.size() - 1;
		}
		GetOptions();
	}
	if (inputManager.isKeyPressed(SDLK_RIGHT) && firstInventory)
	{
		auto firstInv = cursor->selectedUnit->inventory;
		firstInventory = false;
		if (firstInv.size() > tradeUnit->inventory.size())
		{
			currentOption = tradeUnit->inventory.size() - 1;
		}
		GetOptions();
	}
}

void TradeMenu::CancelOption()
{
	if (moving)
	{
		moving = false;
		if (moveFromFirst)
		{
			firstInventory = true;
		}
		else
		{
			firstInventory = false;
		}
		GetOptions();
		currentOption = itemToMove; //This doesn't work properly right now
	}
	else
	{
		MenuManager::menuManager.PreviousMenu();
		MenuManager::menuManager.PreviousMenu();
		MenuManager::menuManager.menus.back()->GetOptions();
	}
}

UnitStatsViewMenu::UnitStatsViewMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, Unit* unit, SpriteRenderer* Renderer) : Menu(Cursor, Text, camera, shapeVAO)
{
	this->unit = unit;
	renderer = Renderer;
	battleStats = unit->CalculateBattleStats();
	fullScreen = true;
}

void UnitStatsViewMenu::Draw()
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(0, 0, 0.0f));

	model = glm::scale(model, glm::vec3(256, 224, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.0f, 1.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
	ResourceManager::GetShader("sprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());

	renderer->setUVs(unit->sprite.getUV());
	Texture2D displayTexture = ResourceManager::GetTexture("sprites");
	renderer->DrawSprite(displayTexture, glm::vec2(16), 0.0f, cursor->dimensions);

	text->RenderText(unit->name, 110, 64, 1);
	text->RenderText(unit->unitClass, 48, 96, 1); 
	text->RenderText("Lv", 48, 128, 1);
	text->RenderText("HP", 48, 160, 1);

	text->RenderTextRight(intToString(unit->level), 90, 128, 1, 14);
	text->RenderTextRight(intToString(unit->currentHP), 90, 160, 1, 14);

	text->RenderText("E", 110, 128, 1, glm::vec3(0.69f, 0.62f, 0.49f));
	text->RenderText("/", 110, 160, 1, glm::vec3(0.69f, 0.62f, 0.49f));

	text->RenderTextRight(intToString(unit->experience), 130, 128, 1, 14);
	text->RenderTextRight(intToString(unit->maxHP), 130, 160, 1, 14);

	text->RenderText("ATK", 500, 64, 1);
	text->RenderText("HIT", 500, 96, 1);
	text->RenderText("RNG", 500, 128, 1);
	if (unit->equippedWeapon >= 0)
	{
		text->RenderTextRight(intToString(battleStats.attackDamage), 542, 64, 1, 14);
		text->RenderTextRight(intToString(battleStats.hitAccuracy), 542, 96, 1, 14);
		auto weapon = unit->GetEquippedWeapon();
		if (weapon.maxRange == weapon.minRange)
		{
			text->RenderTextRight(intToString(weapon.maxRange), 542, 128, 1, 14);
		}
		else
		{
			text->RenderTextRight(intToString(weapon.minRange) + " ~ " + intToString(weapon.maxRange), 542, 128, 1, 30);
		}
	}
	else
	{
		text->RenderText("--", 542, 64, 1);
		text->RenderText("--", 542, 96, 1);
		text->RenderText("--", 542, 128, 1);

	}
	text->RenderText("CRT", 600, 64, 1);
	text->RenderTextRight(intToString(battleStats.hitCrit), 642, 64, 1, 14);
	text->RenderText("AVO", 600, 96, 1);
	text->RenderTextRight(intToString(battleStats.hitAvoid), 642, 96, 1, 14);

	//page 1
	if (firstPage)
	{
		text->RenderText("Inventory", 500, 180, 1);

		auto inventory = unit->inventory;
		glm::vec3 color = glm::vec3(1);
		glm::vec3 grey = glm::vec3(0.64f);
		for (int i = 0; i < inventory.size(); i++)
		{
			color = glm::vec3(1);
			int yPosition = 220 + i * 30;
			auto item = inventory[i];
			if (item->isWeapon && !unit->canUse(unit->GetWeaponData(item)))
			{
				color = grey;
			}
			text->RenderText(item->name, 480, yPosition, 1, color);
			text->RenderTextRight(intToString(item->remainingUses), 680, yPosition, 1, 14, color);
			if (i == unit->equippedWeapon)
			{
				text->RenderText("E", 700, yPosition, 1);
			}
		}
		if (!examining)
		{
			//Going to need an indication of what stats are affected by modifiers
			text->RenderText("Combat Stats", 54, 190, 1);
			text->RenderText("STR", 48, 220, 0.8f);
			text->RenderTextRight(intToString(unit->getStrength()), 148, 220, 1, 14, glm::vec3(0.78f, 0.92f, 1.0f));
			text->RenderText("MAG", 48, 252, 0.8f);
			text->RenderTextRight(intToString(unit->getMagic()), 148, 252, 1, 14, glm::vec3(0.78f, 0.92f, 1.0f));
			text->RenderText("SKL", 48, 284, 0.8f);
			text->RenderTextRight(intToString(unit->getSkill()), 148, 284, 1, 14, glm::vec3(0.78f, 0.92f, 1.0f));
			text->RenderText("SPD", 48, 316, 0.8f);
			text->RenderTextRight(intToString(unit->getSpeed()), 148, 316, 1, 14, glm::vec3(0.78f, 0.92f, 1.0f));
			text->RenderText("LCK", 48, 348, 0.8f);
			text->RenderTextRight(intToString(unit->getLuck()), 148, 348, 1, 14, glm::vec3(0.78f, 0.92f, 1.0f));
			text->RenderText("DEF", 48, 380, 0.8f);
			text->RenderTextRight(intToString(unit->getDefense()), 148, 380, 1, 14, glm::vec3(0.78f, 0.92f, 1.0f));
			text->RenderText("CON", 48, 412, 0.8f);
			text->RenderTextRight(intToString(unit->getBuild()), 148, 412, 1, 14, glm::vec3(0.78f, 0.92f, 1.0f));
		}
		else
		{
			ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
			ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
			glm::mat4 model = glm::mat4();
			model = glm::translate(model, glm::vec3(128, 75 + (12 * currentOption), 0.0f));

			model = glm::scale(model, glm::vec3(16, 16, 0.0f));

			ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

			ResourceManager::GetShader("shape").SetMatrix4("model", model);
			glBindVertexArray(shapeVAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			glBindVertexArray(0);
			int yPosition = 220;

			if (inventory[currentOption]->isWeapon)
			{
				int offSet = 30;
				auto weaponData = ItemManager::itemManager.weaponData[inventory[currentOption]->ID];

				int xStatName = 48;
				int xStatValue = 88;
				text->RenderText("Type", xStatName, yPosition, 1);
				text->RenderTextRight(intToString(weaponData.type), xStatValue, yPosition, 1, 14);
				yPosition += offSet;
				text->RenderText("Atk", xStatName, yPosition, 1);
				text->RenderTextRight(intToString(battleStats.attackDamage), xStatValue, yPosition, 1, 14);
				yPosition += offSet;
				text->RenderText("Hit", xStatName, yPosition, 1);
				text->RenderTextRight(intToString(battleStats.hitAccuracy), xStatValue, yPosition, 1, 14);
				yPosition += offSet;
				text->RenderText("Crit", xStatName, yPosition, 1);
				text->RenderTextRight(intToString(battleStats.hitCrit), xStatValue, yPosition, 1, 14);
				yPosition += offSet;
				text->RenderText("Avo", xStatName, yPosition, 1);
				text->RenderTextRight(intToString(battleStats.hitAvoid), xStatValue, yPosition, 1, 14);
				yPosition += offSet;
			}
			if (true)
			{
				int a = 2;
			}
			text->RenderText(inventory[currentOption]->description, 48, yPosition, 1);

		}

	}
	else
	{
		text->RenderText("COMING SOON", 700, 0, 1);

	}

}

void UnitStatsViewMenu::SelectOption()
{
}

void UnitStatsViewMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (!examining)
	{
		if (firstPage)
		{
			if (inputManager.isKeyPressed(SDLK_DOWN))
			{
				firstPage = false;
			}
		}
		else
		{
			if (inputManager.isKeyPressed(SDLK_UP))
			{
				firstPage = true;
			}
		}
		if (inputManager.isKeyPressed(SDLK_SPACE))
		{
			if (firstPage)
			{
				if (unit->inventory.size() > 0)
				{
					examining = true;
					currentOption = 0;
					numberOfOptions = unit->inventory.size();
				}
			}
			else
			{
				if (unit->skills.size() > 0)
				{
					examining = true;
					currentOption = 0;
					numberOfOptions = unit->skills.size();
				}
			}
		}
		else if (inputManager.isKeyPressed(SDLK_z))
		{
			CancelOption();
		}
	}
	else
	{
		bool moved = false;
		if (inputManager.isKeyPressed(SDLK_UP))
		{
			moved = true;
			currentOption--;
			if (currentOption < 0)
			{
				currentOption = numberOfOptions - 1;
			}
		}
		else if (inputManager.isKeyPressed(SDLK_DOWN))
		{
			moved = true;
			currentOption++;
			if (currentOption >= numberOfOptions)
			{
				currentOption = 0;
			}
		}
		if (inputManager.isKeyPressed(SDLK_z))
		{
			examining = false;
		}

		if (firstPage)
		{
			if (moved)
			{
				auto inventory = unit->inventory;
				battleStats = unit->CalculateBattleStats(inventory[currentOption]->ID);
			}
		}
	}
}

ExtraMenu::ExtraMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO) : Menu(Cursor, Text, camera, shapeVAO)
{
	numberOfOptions = 5;
}

void ExtraMenu::Draw()
{
	int xText = 600;
	int xIndicator = 176;
	int yOffset = 100;
	glm::vec2 fixedPosition = camera->worldToScreen(cursor->position);
	if (fixedPosition.x >= camera->screenWidth * 0.5f)
	{
		xText = 72;
		xIndicator = 8;
	}
	//ResourceManager::GetShader("shape").Use().SetMatrix4("projection", glm::ortho(0.0f, 800.0f, 600.0f, 0.0f));
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(xIndicator, 32 + (12 * currentOption), 0.0f));

	model = glm::scale(model, glm::vec3(16, 16, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
	
	text->RenderText("Unit", xText, 100, 1);
	text->RenderText("Status", xText, 130, 1);
	text->RenderText("Options", xText, 160, 1);
	text->RenderText("Suspend", xText, 190, 1);
	text->RenderText("End", xText, 220, 1);
}

void ExtraMenu::SelectOption()
{
	switch (currentOption)
	{
	case UNIT:
		std::cout << "Unit menu\n";
		break;
	case STATUS:
		std::cout << "Status menu\n";

		break;
	case OPTIONS:
		if (unitSpeed < 5)
		{
			unitSpeed = 5.0f;
			std::cout << "Speed up\n";
		}
		else
		{
			std::cout << "Speed down\n";
			unitSpeed = 2.5f;
		}
		break;
	case SUSPEND:
		std::cout << "Suspend menu\n";

		break;
	case END:
		std::cout << "End turn menu\n";
		MenuManager::menuManager.subject.notify(0);
		ClearMenu();
		break;
	}
}

SelectRescueUnit::SelectRescueUnit(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<Unit*>& units, SpriteRenderer* Renderer) 
	: Menu(Cursor, Text, camera, shapeVAO)
{
	renderer = Renderer;
	rescueUnits = units;
	GetOptions();
}

void SelectRescueUnit::Draw()
{
	auto rescueUnit = rescueUnits[currentOption];
	int boxHeight = 98;

	auto targetPosition = rescueUnit->sprite.getPosition();
	int xText = 536;
	int xIndicator = 169;
	glm::vec2 fixedPosition = camera->worldToScreen(targetPosition);
	if (fixedPosition.x >= camera->screenWidth * 0.5f)
	{
		xText = 32;
		xIndicator = 7;
	}

	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(xIndicator, 10, 0.0f));

	model = glm::scale(model, glm::vec3(82, boxHeight, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.2f, 0.0f, 1.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	renderer->setUVs(cursor->uvs[1]);
	Texture2D displayTexture = ResourceManager::GetTexture("cursor");
	Unit* unit = cursor->selectedUnit;
	renderer->DrawSprite(displayTexture, targetPosition, 0.0f, cursor->dimensions);

	int textHeight = 100;
	text->RenderText(rescueUnit->name, xText, textHeight, 1);
	textHeight += 30;
	text->RenderText(rescueUnit->unitClass, xText, textHeight, 1);
	textHeight += 30;
	if (auto weapon = rescueUnit->GetEquippedItem())
	{
		text->RenderText(weapon->name, xText, textHeight, 1);
	}
	textHeight += 30;
	text->RenderText(intToString(rescueUnit->level), xText + 40, textHeight, 1);
	textHeight += 30;
	text->RenderText("HP" + intToString(rescueUnit->maxHP) + "/" + intToString(rescueUnit->currentHP), xText, textHeight, 1);
//	text->RenderTextRight(intToString(tradeUnit->inventory[i]->remainingUses), xText + 100, textHeight, 1, 14);

}

void SelectRescueUnit::SelectOption()
{
	auto rescuedUnit = rescueUnits[currentOption];
	auto playerUnit = cursor->selectedUnit;
	playerUnit->carriedUnit = rescuedUnit;
	rescuedUnit->isCarried = true;
	TileManager::tileManager.removeUnit(rescuedUnit->sprite.getPosition().x, rescuedUnit->sprite.getPosition().y);
	if (playerUnit->isMounted() && playerUnit->mount->remainingMoves > 0)
	{
		cursor->GetRemainingMove();
		MenuManager::menuManager.mustWait = true;
	}
	else
	{
		cursor->Wait();
	}
	playerUnit->carryingMalus = 2;
	ClearMenu();
}

void SelectRescueUnit::GetOptions()
{
	numberOfOptions = rescueUnits.size();
}

void SelectRescueUnit::CheckInput(InputManager& inputManager, float deltaTime)
{
	Menu::CheckInput(inputManager, deltaTime);
	if (inputManager.isKeyPressed(SDLK_LEFT))
	{
		currentOption--;
		if (currentOption < 0)
		{
			currentOption = numberOfOptions - 1;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_RIGHT))
	{
		currentOption++;
		if (currentOption >= numberOfOptions)
		{
			currentOption = 0;
		}
	}
}

DropMenu::DropMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<glm::ivec2>& positions, SpriteRenderer* Renderer) : 
	Menu(Cursor, Text, camera, shapeVAO), positions(positions)
{
	renderer = Renderer;
	GetOptions();
}

void DropMenu::Draw()
{
	auto targetPosition = positions[currentOption];
	renderer->setUVs(cursor->uvs[1]);
	Texture2D displayTexture = ResourceManager::GetTexture("cursor");
	Unit* unit = cursor->selectedUnit;
	renderer->DrawSprite(displayTexture, targetPosition, 0.0f, cursor->dimensions);
}

void DropMenu::SelectOption()
{
	auto playerUnit = cursor->selectedUnit;
	auto heldUnit = playerUnit->carriedUnit;
	heldUnit->placeUnit(positions[currentOption].x, positions[currentOption].y);
	heldUnit->isCarried = false;
	playerUnit->carriedUnit = nullptr;
	if (playerUnit->isMounted() && playerUnit->mount->remainingMoves > 0)
	{
		cursor->GetRemainingMove();
		MenuManager::menuManager.mustWait = true;
	}
	else
	{
		cursor->Wait();
	}
	playerUnit->carryingMalus = 1;
	ClearMenu();
}

void DropMenu::GetOptions()
{
	numberOfOptions = positions.size();
}

void DropMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	Menu::CheckInput(inputManager, deltaTime);
	if (inputManager.isKeyPressed(SDLK_LEFT))
	{
		currentOption--;
		if (currentOption < 0)
		{
			currentOption = numberOfOptions - 1;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_RIGHT))
	{
		currentOption++;
		if (currentOption >= numberOfOptions)
		{
			currentOption = 0;
		}
	}
}

SelectTransferUnit::SelectTransferUnit(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<Unit*>& units, SpriteRenderer* Renderer)
	: Menu(Cursor, Text, camera, shapeVAO)
{
	renderer = Renderer;
	transferUnits = units;
	GetOptions();
}

void SelectTransferUnit::Draw()
{
	auto transferUnit = transferUnits[currentOption];
	int boxHeight = 98;

	auto targetPosition = transferUnit->sprite.getPosition();
	int xText = 536;
	int xIndicator = 169;
	glm::vec2 fixedPosition = camera->worldToScreen(targetPosition);
	if (fixedPosition.x >= camera->screenWidth * 0.5f)
	{
		xText = 32;
		xIndicator = 7;
	}

	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(xIndicator, 10, 0.0f));

	model = glm::scale(model, glm::vec3(82, boxHeight, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.2f, 0.0f, 1.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	renderer->setUVs(cursor->uvs[1]);
	Texture2D displayTexture = ResourceManager::GetTexture("cursor");
	Unit* unit = cursor->selectedUnit;
	renderer->DrawSprite(displayTexture, targetPosition, 0.0f, cursor->dimensions);

	int textHeight = 100;
	text->RenderText(transferUnit->name, xText, textHeight, 1);
	textHeight += 30;
	text->RenderText(transferUnit->unitClass, xText, textHeight, 1);
	textHeight += 30;
	if (auto weapon = transferUnit->GetEquippedItem())
	{
		text->RenderText(weapon->name, xText, textHeight, 1);
	}
	textHeight += 30;
	text->RenderText(intToString(transferUnit->level), xText + 40, textHeight, 1);
	textHeight += 30;
	text->RenderText("HP" + intToString(transferUnit->maxHP) + "/" + intToString(transferUnit->currentHP), xText, textHeight, 1);
	//	text->RenderTextRight(intToString(tradeUnit->inventory[i]->remainingUses), xText + 100, textHeight, 1, 14);
}

void SelectTransferUnit::SelectOption()
{
	auto transferUnit = transferUnits[currentOption];
	auto playerUnit = cursor->selectedUnit;
	if (playerUnit->carriedUnit != nullptr)
	{
		auto heldUnit = playerUnit->carriedUnit;
		transferUnit->carriedUnit = heldUnit;
		playerUnit->carriedUnit = nullptr;
		transferUnit->carryingMalus = 2;
		playerUnit->carryingMalus = 1;
	}
	else
	{
		auto heldUnit = transferUnit->carriedUnit;
		playerUnit->carriedUnit = heldUnit;
		transferUnit->carriedUnit = nullptr;
		transferUnit->carryingMalus = 1;
		playerUnit->carryingMalus = 2;
	}


	Menu::CancelOption();
	MenuManager::menuManager.menus.back()->GetOptions();
	MenuManager::menuManager.mustWait = true;
}

void SelectTransferUnit::GetOptions()
{
	numberOfOptions = transferUnits.size();
}

void SelectTransferUnit::CheckInput(InputManager& inputManager, float deltaTime)
{
	Menu::CheckInput(inputManager, deltaTime);
	if (inputManager.isKeyPressed(SDLK_LEFT))
	{
		currentOption--;
		if (currentOption < 0)
		{
			currentOption = numberOfOptions - 1;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_RIGHT))
	{
		currentOption++;
		if (currentOption >= numberOfOptions)
		{
			currentOption = 0;
		}
	}
}
