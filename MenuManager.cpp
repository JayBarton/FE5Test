#include "MenuManager.h"
#include "Cursor.h"
#include "TextRenderer.h"
#include "Camera.h"
#include "InputManager.h"
#include "Items.h"
#include "BattleManager.h"
#include "Tile.h"
#include "SceneManager.h"
#include "PlayerManager.h"
#include "EnemyManager.h"
#include "Vendor.h"

#include "Globals.h"
#include "SBatch.h"
#include "Settings.h"

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
	else if (inputManager.isKeyPressed(SDLK_DOWN))
	{
		currentOption++;
		if (currentOption >= numberOfOptions)
		{
			currentOption = 0;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
	else if (inputManager.isKeyPressed(SDLK_z))
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
		Menu* newMenu = new SelectWeaponMenu(cursor, text, camera, shapeVAO, validWeapons, unitsToAttack, MenuManager::menuManager.renderer);
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
		Menu* newMenu = new SelectWeaponMenu(cursor, text, camera, shapeVAO, validWeapons, unitsToAttack, MenuManager::menuManager.renderer, true);
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
		visit->sceneMap[sceneID]->initiator = playerUnit;
		visit->sceneMap[sceneID]->activation->CheckActivation();
		ClearMenu();

		break;
	}
	case VENDOR:
	{
		auto position = playerUnit->sprite.getPosition();
		auto vendor = TileManager::tileManager.getVendor(position.x, position.y);
		Menu* newMenu = new VendorMenu(cursor, text, camera, shapeVAO, playerUnit, vendor);
		MenuManager::menuManager.menus.push_back(newMenu);
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
	else if (canBuy)
	{
		text->RenderText("Vendor", xText, yOffset, 1);
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
	canBuy = false;
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
	else if (TileManager::tileManager.getVendor(playerPosition.x, playerPosition.y))
	{
		canBuy = true;
		optionsVector.push_back(VENDOR);
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
						//Not accounting for mounted selected unit, should be able to pickup without this check
						if (playerUnit->isMounted() || currentUnit->getBuild() < playerUnit->getBuild())
						{
							rescueUnits.push_back(currentUnit);
						}
					}
					else if (currentUnit->carriedUnit)
					{
						if (playerUnit->isMounted() || currentUnit->carriedUnit->getBuild() < playerUnit->getBuild())
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
						if (currentUnit->isMounted() || playerUnit->carriedUnit->getBuild() < currentUnit->getBuild())
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

ItemOptionsMenu::ItemOptionsMenu(Cursor* Cursor, TextRenderer* Text, Camera* Camera, int shapeVAO, SpriteRenderer* Renderer) : Menu(Cursor, Text, Camera, shapeVAO)
{
	GetOptions();
	renderer = Renderer;
	itemIconUVs = MenuManager::menuManager.itemIconUVs;
	proficiencyIconUVs = MenuManager::menuManager.proficiencyIconUVs;
}

void ItemOptionsMenu::Draw()
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(24, 82 + 16 * currentOption, 0.0f));

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
		int yPosition = 225 + i * 42;
		auto item = inventory[i];
		if (item->isWeapon && !unit->canUse(unit->GetWeaponData(item)))
		{
			color = grey;
		}
		text->RenderText(item->name, 175, yPosition, 1, color);
		text->RenderTextRight(intToString(item->remainingUses), 375, yPosition, 1, 14, color);

		ResourceManager::GetShader("Nsprite").Use();
		ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());
		auto texture = ResourceManager::GetTexture("icons");

		renderer->setUVs(itemIconUVs[item->ID]);
		renderer->DrawSprite(texture, glm::vec2(40, 82 + 16 * i), 0.0f, cursor->dimensions);
	}
	if (inventory[currentOption]->isWeapon)
	{
		DrawWeaponComparison(inventory);
	}
	else
	{
		text->RenderText(inventory[currentOption]->description, 525, 225, 1);
	}
}

void ItemOptionsMenu::DrawWeaponComparison(std::vector<Item*>& inventory)
{
	int offSet = 42;
	int yPosition = 225;
	auto weaponData = ItemManager::itemManager.weaponData[inventory[currentOption]->ID];

	int xStatName = 525;
	int xStatValue = 575;
	text->RenderText("Type", xStatName, yPosition, 1);
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

	ResourceManager::GetShader("Nsprite").Use();
	ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());
	auto texture = ResourceManager::GetTexture("icons");

	renderer->setUVs(proficiencyIconUVs[weaponData.type]);
	renderer->DrawSprite(texture, glm::vec2(193, 83), 0.0f, cursor->dimensions);
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
void MenuManager::SetUp(Cursor* Cursor, TextRenderer* Text, Camera* Camera, int shapeVAO, 
	SpriteRenderer* Renderer, BattleManager* battleManager, PlayerManager* playerManager, EnemyManager* enemyManager)
{
	renderer = Renderer;
	cursor = Cursor;
	text = Text;
	camera = Camera;
	this->battleManager = battleManager;
	this->shapeVAO = shapeVAO;
	this->playerManager = playerManager;
	this->enemyManager = enemyManager;

	profcienciesMap[0] = "-";
	profcienciesMap[1] = "E";
	profcienciesMap[2] = "D";
	profcienciesMap[3] = "C";
	profcienciesMap[4] = "B";
	profcienciesMap[5] = "A";

	proficiencyIconUVs = ResourceManager::GetTexture("icons").GetUVs(0, 0, 16, 16, 10, 1);
	itemIconUVs = ResourceManager::GetTexture("icons").GetUVs(0, 16, 16, 16, 10, 2, 16);
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
		Menu* newMenu = new ItemOptionsMenu(cursor, text, camera, shapeVAO, renderer);
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

void MenuManager::AddFullInventoryMenu(int itemID)
{
	Menu* newMenu = new FullInventoryMenu(cursor, text, camera, shapeVAO, itemID, renderer);
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

SelectWeaponMenu::SelectWeaponMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<Item*>& validWeapons, std::vector<std::vector<Unit*>>& units, SpriteRenderer* Renderer, bool capturing)
	: ItemOptionsMenu(Cursor, Text, camera, shapeVAO, Renderer), capturing(capturing)
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
	model = glm::translate(model, glm::vec3(24, 82 + 16 * currentOption, 0.0f));

	model = glm::scale(model, glm::vec3(16, 16, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	Unit* unit = cursor->selectedUnit;
	for (int i = 0; i < inventory.size(); i++)
	{
		int yPosition = 225 + i * 42;
		text->RenderText(inventory[i]->name, 175, yPosition, 1);
		text->RenderTextRight(intToString(inventory[i]->remainingUses), 375, yPosition, 1, 14);
		ResourceManager::GetShader("Nsprite").Use();
		ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());
		auto texture = ResourceManager::GetTexture("icons");

		renderer->setUVs(itemIconUVs[inventory[i]->ID]);
		renderer->DrawSprite(texture, glm::vec2(40, 82 + 16 * i), 0.0f, cursor->dimensions);
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
	Menu* newMenu = new TradeMenu(cursor, text, camera, shapeVAO, tradeUnits[currentOption], MenuManager::menuManager.renderer);
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
	auto sceneID = talkUnits[currentOption]->sceneID;
	auto talkData = cursor->selectedUnit->talkData;
	int index = 0;
	//would really like a better way of doing this
	for (int i = 0; i < talkData.size(); i++)
	{
		if (sceneID == talkData[i].talkTarget)
		{
			index = i;
			break;
		}
	}
	auto playerUnit = cursor->selectedUnit;
	playerUnit->talkData[index].scene->initiator = playerUnit;
	playerUnit->talkData[index].scene->activation->CheckActivation();
	playerUnit->talkData[index] = playerUnit->talkData.back();
	playerUnit->talkData.pop_back();
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

TradeMenu::TradeMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, Unit* unit, SpriteRenderer* Renderer) 
	: Menu(Cursor, Text, camera, shapeVAO)
{
	renderer = Renderer;
	tradeUnit = unit;
	itemIconUVs = MenuManager::menuManager.itemIconUVs;
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
		x = 8;
	}
	else
	{
		x = 136;
	}
	model = glm::translate(model, glm::vec3(x, 98 + 16 * currentOption, 0.0f));

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
			x = 8;
		}
		else
		{
			x = 136;
		}
		model = glm::translate(model, glm::vec3(x, 98 + 16 * itemToMove, 0.0f));

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
	ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());

	for (int i = 0; i < firstUnit->inventory.size(); i++)
	{
		text->RenderText(firstUnit->inventory[i]->name, 125, 267 + i * 42, 1);
		text->RenderTextRight(intToString(firstUnit->inventory[i]->remainingUses), 325, 267 + i * 30, 1, 14);
		ResourceManager::GetShader("Nsprite").Use();
		auto texture = ResourceManager::GetTexture("icons");

		renderer->setUVs(itemIconUVs[firstUnit->inventory[i]->ID]);
		renderer->DrawSprite(texture, glm::vec2(24, 98 + 16 * i), 0.0f, cursor->dimensions);
	}

	for (int i = 0; i < tradeUnit->inventory.size(); i++)
	{
		text->RenderText(tradeUnit->inventory[i]->name, 525, 267 + i * 42, 1);
		text->RenderTextRight(intToString(tradeUnit->inventory[i]->remainingUses), 725, 267 + i * 42, 1, 14);

		ResourceManager::GetShader("Nsprite").Use();
		auto texture = ResourceManager::GetTexture("icons");

		renderer->setUVs(itemIconUVs[tradeUnit->inventory[i]->ID]);
		renderer->DrawSprite(texture, glm::vec2(152, 98 + 16 * i), 0.0f, cursor->dimensions);
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
		MenuManager::menuManager.menus.back()->currentOption = 0;
		MenuManager::menuManager.menus.back()->GetOptions();
	}
}

UnitStatsViewMenu::UnitStatsViewMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, Unit* unit, SpriteRenderer* Renderer) :
	Menu(Cursor, Text, camera, shapeVAO)
{
	this->unit = unit;
	renderer = Renderer;
	battleStats = unit->CalculateBattleStats();
	fullScreen = true;
	if (unit->team == 0)
	{
		unitList = &MenuManager::menuManager.playerManager->playerUnits;
	}
	else if (unit->team == 1)
	{
		unitList = &MenuManager::menuManager.enemyManager->enemies;
	}

	for (int i = 0; i < unitList->size(); i++)
	{
		if (unit == (*unitList)[i])
		{
			unitIndex = i;
			break;
		}
	}

	proficiencyIconUVs = MenuManager::menuManager.proficiencyIconUVs;
	itemIconUVs = MenuManager::menuManager.itemIconUVs;
}

void UnitStatsViewMenu::Draw()
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(0, 79, 0.0f));

	model = glm::scale(model, glm::vec3(256, 145, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.0f, 0.9f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	//page 1
	if (firstPage || transition)
	{
		float adjustedOffset = ((224 - yOffset) / 224.0f) * 600;
		text->RenderText("Inventory", 500, 180 - adjustedOffset, 1);

		auto inventory = unit->inventory;
		glm::vec3 color = glm::vec3(1);
		glm::vec3 grey = glm::vec3(0.64f);
		for (int i = 0; i < inventory.size(); i++)
		{
			color = glm::vec3(1);
			int yPosition = (270 + i * 42) - adjustedOffset;
			auto item = inventory[i];
			if (item->isWeapon && !unit->canUse(unit->GetWeaponData(item)))
			{
				color = grey;
			}
			text->RenderText(item->name, 425, yPosition, 1, color);
			text->RenderTextRight(intToString(item->remainingUses) + "/", 625, yPosition, 1, 14, color);
			text->RenderTextRight(intToString(item->maxUses), 700, yPosition, 1, 14, color);
			if (i == unit->equippedWeapon)
			{
				text->RenderText("E", 750, yPosition, 1);
			}
			ResourceManager::GetShader("Nsprite").Use();
			ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());
			auto texture = ResourceManager::GetTexture("icons");

			renderer->setUVs(itemIconUVs[item->ID]);
			renderer->DrawSprite(texture, glm::vec2(120, (95 + 16 * i) - (224 -yOffset)), 0.0f, cursor->dimensions);
		}
		if (!examining)
		{
			//Going to need an indication of what stats are affected by modifiers
			text->RenderText("Combat Stats", 54, 190 - adjustedOffset, 1);
			text->RenderText("STR", 48, 220 - adjustedOffset, 0.8f);
			text->RenderTextRight(intToString(unit->getStrength()), 148, 220 - adjustedOffset, 1, 14, glm::vec3(0.78f, 0.92f, 1.0f));
			text->RenderText("MAG", 48, 252 - adjustedOffset, 0.8f);
			text->RenderTextRight(intToString(unit->getMagic()), 148, 252 - adjustedOffset, 1, 14, glm::vec3(0.78f, 0.92f, 1.0f));
			text->RenderText("SKL", 48, 284 - adjustedOffset, 0.8f);
			text->RenderTextRight(intToString(unit->getSkill()), 148, 284 - adjustedOffset, 1, 14, glm::vec3(0.78f, 0.92f, 1.0f));
			text->RenderText("SPD", 48, 316 - adjustedOffset, 0.8f);
			text->RenderTextRight(intToString(unit->getSpeed()), 148, 316 - adjustedOffset, 1, 14, glm::vec3(0.78f, 0.92f, 1.0f));
			text->RenderText("LCK", 48, 348 - adjustedOffset, 0.8f);
			text->RenderTextRight(intToString(unit->getLuck()), 148, 348 - adjustedOffset, 1, 14, glm::vec3(0.78f, 0.92f, 1.0f));
			text->RenderText("DEF", 48, 380 - adjustedOffset, 0.8f);
			text->RenderTextRight(intToString(unit->getDefense()), 148, 380 - adjustedOffset, 1, 14, glm::vec3(0.78f, 0.92f, 1.0f));
			text->RenderText("CON", 48, 412 - adjustedOffset, 0.8f);
			text->RenderTextRight(intToString(unit->getBuild()), 148, 412 - adjustedOffset, 1, 14, glm::vec3(0.78f, 0.92f, 1.0f));
		}
		else
		{
			ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
			ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
			glm::mat4 model = glm::mat4();
			model = glm::translate(model, glm::vec3(104, 95 + (16 * currentOption), 0.0f));

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

				int xStatName = 100;
				int xStatValue = 200;
				text->RenderText("Type", xStatName, yPosition - adjustedOffset, 1);
				text->RenderTextRight(intToString(weaponData.type), xStatValue, yPosition - adjustedOffset, 1, 14);
				yPosition += offSet;
				text->RenderText("Hit", xStatName, yPosition - adjustedOffset, 1);
				text->RenderTextRight(intToString(weaponData.hit), xStatValue, yPosition - adjustedOffset, 1, 14);
				yPosition += offSet;
				text->RenderText("Might", xStatName, yPosition - adjustedOffset, 1);
				text->RenderTextRight(intToString(weaponData.might), xStatValue, yPosition - adjustedOffset, 1, 14);
				yPosition += offSet;
				text->RenderText("Crit", xStatName, yPosition - adjustedOffset, 1);
				text->RenderTextRight(intToString(weaponData.crit), xStatValue, yPosition - adjustedOffset, 1, 14);
				yPosition += offSet;
				text->RenderText("Range", xStatName, yPosition - adjustedOffset, 1);
				text->RenderTextRight(intToString(weaponData.minRange), xStatValue, yPosition - adjustedOffset, 1, 14);
				yPosition += offSet;
				text->RenderText("Weight", xStatName, yPosition - adjustedOffset, 1);
				text->RenderTextRight(intToString(weaponData.weight), xStatValue, yPosition - adjustedOffset, 1, 14);
				yPosition += offSet;
			}
			text->RenderText(inventory[currentOption]->description, 50, yPosition - adjustedOffset, 1);
		}

	}
	if(!firstPage || transition)
	{
		ResourceManager::GetShader("Nsprite").Use();
		ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());
		auto texture = ResourceManager::GetTexture("icons");

		renderer->setUVs(proficiencyIconUVs[WeaponData::TYPE_SWORD]);
		renderer->DrawSprite(texture, glm::vec2(129, 106 + yOffset), 0.0f, cursor->dimensions);
		renderer->setUVs(proficiencyIconUVs[WeaponData::TYPE_LANCE]);
		renderer->DrawSprite(texture, glm::vec2(129, 122 + yOffset), 0.0f, cursor->dimensions);
		renderer->setUVs(proficiencyIconUVs[WeaponData::TYPE_AXE]);
		renderer->DrawSprite(texture, glm::vec2(129, 138 + yOffset), 0.0f, cursor->dimensions);
		renderer->setUVs(proficiencyIconUVs[WeaponData::TYPE_BOW]);
		renderer->DrawSprite(texture, glm::vec2(129, 154 + yOffset), 0.0f, cursor->dimensions);
		renderer->setUVs(proficiencyIconUVs[WeaponData::TYPE_STAFF]);
		renderer->DrawSprite(texture, glm::vec2(129, 170 + yOffset), 0.0f, cursor->dimensions);

		renderer->setUVs(proficiencyIconUVs[WeaponData::TYPE_FIRE]);
		renderer->DrawSprite(texture, glm::vec2(193, 106 + yOffset), 0.0f, cursor->dimensions);
		renderer->setUVs(proficiencyIconUVs[WeaponData::TYPE_THUNDER]);
		renderer->DrawSprite(texture, glm::vec2(193, 122 + yOffset), 0.0f, cursor->dimensions);
		renderer->setUVs(proficiencyIconUVs[WeaponData::TYPE_WIND]);
		renderer->DrawSprite(texture, glm::vec2(193, 138 + yOffset), 0.0f, cursor->dimensions);
		renderer->setUVs(proficiencyIconUVs[WeaponData::TYPE_LIGHT]);
		renderer->DrawSprite(texture, glm::vec2(193, 154 + yOffset), 0.0f, cursor->dimensions);
		renderer->setUVs(proficiencyIconUVs[WeaponData::TYPE_DARK]);
		renderer->DrawSprite(texture, glm::vec2(193, 170 + yOffset), 0.0f, cursor->dimensions);

		auto& profMap = MenuManager::menuManager.profcienciesMap;

		int adjustedOffset = (yOffset / 224.0f) * 600;
		text->RenderText(profMap[unit->weaponProficiencies[WeaponData::TYPE_SWORD]], 500, 291 + adjustedOffset, 1);
		text->RenderText(profMap[unit->weaponProficiencies[WeaponData::TYPE_LANCE]], 500, 333 + adjustedOffset, 1);
		text->RenderText(profMap[unit->weaponProficiencies[WeaponData::TYPE_AXE]], 500, 375 + adjustedOffset, 1);
		text->RenderText(profMap[unit->weaponProficiencies[WeaponData::TYPE_BOW]], 500, 417 + adjustedOffset, 1);
		text->RenderText(profMap[unit->weaponProficiencies[WeaponData::TYPE_STAFF]], 500, 459 + adjustedOffset, 1);

		text->RenderText(profMap[unit->weaponProficiencies[WeaponData::TYPE_FIRE]], 700, 291 + adjustedOffset, 1);
		text->RenderText(profMap[unit->weaponProficiencies[WeaponData::TYPE_THUNDER]], 700, 333 + adjustedOffset, 1);
		text->RenderText(profMap[unit->weaponProficiencies[WeaponData::TYPE_WIND]], 700, 375 + adjustedOffset, 1);
		text->RenderText(profMap[unit->weaponProficiencies[WeaponData::TYPE_LIGHT]], 700, 417 + adjustedOffset, 1);
		text->RenderText(profMap[unit->weaponProficiencies[WeaponData::TYPE_DARK]], 700, 459 + adjustedOffset, 1);

		text->RenderText("Personal Data", 37, 246 + adjustedOffset, 1);
		text->RenderText("Leader", 25, 291 + adjustedOffset, 1);
		text->RenderText("Lead*", 25, 334 + adjustedOffset, 1);
		text->RenderText("Move*", 25, 377 + adjustedOffset, 1);
		text->RenderText("Trvlr.", 25, 420 + adjustedOffset, 1);
		text->RenderText("Move", 25, 463 + adjustedOffset, 1);
		text->RenderText("Fatg.", 25, 506 + adjustedOffset, 1);
		text->RenderText("Status", 25, 549 + adjustedOffset, 1);

		//Leader
		text->RenderText("Leif", 150, 291 + adjustedOffset, 1);

		if (unit->carriedUnit)
		{
			text->RenderText(unit->carriedUnit->name, 150, 420 + adjustedOffset, 1);
			text->RenderText("V", 100, 466 + adjustedOffset, 1); //replace with arrow sprite later
		}
		else
		{
			text->RenderText("----", 153, 420 + adjustedOffset, 1);
		}

		text->RenderText(intToString(unit->getMove()), 200, 466 + adjustedOffset, 1);
		//Fatigue
		text->RenderText(intToString(0), 200, 508 + adjustedOffset, 1);
		//Status
		text->RenderText("----", 153, 549 + adjustedOffset, 1);

		text->RenderText("Weapon Ranks", 434, 246 + adjustedOffset, 1);

		text->RenderText("Skills", 434, 503 + adjustedOffset, 1);
	}

	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(0, 0, 0.0f));

	model = glm::scale(model, glm::vec3(256, 79, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.0f, 0.8f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	//Would like to not use batch here it's just easier without writing a new function/shader
	ResourceManager::GetShader("sprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	SBatch Batch;
	Batch.init();
	Batch.begin();
	unit->Draw(&Batch, glm::vec2(16, 10), true);
	Batch.end();
	Batch.renderBatch();

	text->RenderText(unit->name, 100, 29, 1);
	text->RenderText(unit->unitClass, 50, 72, 1);
	text->RenderText("Lv", 50, 120, 1);
	text->RenderText("HP", 50, 163, 1);

	text->RenderTextRight(intToString(unit->level), 125, 120, 1, 14);
	text->RenderTextRight(intToString(unit->currentHP), 125, 163, 1, 14);

	text->RenderText("E", 175, 120, 1, glm::vec3(0.69f, 0.62f, 0.49f));
	text->RenderText("/", 175, 163, 1, glm::vec3(0.69f, 0.62f, 0.49f));

	text->RenderTextRight(intToString(unit->experience), 200, 120, 1, 14);
	text->RenderTextRight(intToString(unit->maxHP), 200, 163, 1, 14);

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
		ResourceManager::GetShader("Nsprite").Use();
		ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());
		auto texture = ResourceManager::GetTexture("icons");

		renderer->setUVs(proficiencyIconUVs[unit->GetEquippedWeapon().type]);
		renderer->DrawSprite(texture, glm::vec2(233, 59), 0.0f, cursor->dimensions);
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

}

void UnitStatsViewMenu::SelectOption()
{
}

void UnitStatsViewMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (transition)
	{
		int rate = 960;
		float direction;
		if (firstPage == false)
		{
			direction = -1;
		}
		else
		{
			direction = 1;
		}

		float distance = glm::length(goal - start);

		// Calculate easing factor based on current time
		float easingFactor = EaseOut(t);

		// Calculate current position based on easing factor
		float currentPosition = start + direction * (distance * easingFactor);

		// Update camera position
		yOffset = currentPosition;

		// Increment time based on speed
		t += rate * deltaTime / distance;

		// Break loop if reached the destination
		if (t >= 1.0f)
		{
			yOffset = goal;
			transition = false;
		}
	}
	else
	{
		if (!examining)
		{
			if (firstPage)
			{
				if (inputManager.isKeyPressed(SDLK_DOWN))
				{
					firstPage = false;
					transition = true;
					t = 0.0f;
					goal = 0;
					start = 224;
					yOffset = 224;
				}
			}
			else
			{
				if (inputManager.isKeyPressed(SDLK_UP))
				{
					firstPage = true;
					transition = true;
					t = 0.0f;
					goal = 224;
					start = 0;
					yOffset = 0;
				}
			}
			if (inputManager.isKeyPressed(SDLK_LEFT))
			{
				unitIndex--;
				if (unitIndex < 0)
				{
					unitIndex = unitList->size() - 1;
				}
				unit = (*unitList)[unitIndex];
				battleStats = unit->CalculateBattleStats();
			}
			else if (inputManager.isKeyPressed(SDLK_RIGHT))
			{
				unitIndex++;
				if (unitIndex >= unitList->size())
				{
					unitIndex = 0;
				}
				unit = (*unitList)[unitIndex];
				battleStats = unit->CalculateBattleStats();
			}
			else if (inputManager.isKeyPressed(SDLK_SPACE))
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
					//	battleStats = unit->CalculateBattleStats(inventory[currentOption]->ID);
				}
			}
		}
	}
}

void UnitStatsViewMenu::CancelOption()
{
	cursor->position = (*unitList)[unitIndex]->sprite.getPosition();
	cursor->focusedUnit = (*unitList)[unitIndex];
	camera->SetMove(cursor->position);
	Menu::CancelOption();
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
	{
		Menu* newMenu = new UnitListMenu(cursor, text, camera, shapeVAO);
		MenuManager::menuManager.menus.push_back(newMenu);
		break;
	}
	case STATUS:
	{
		Menu* newMenu = new StatusMenu(cursor, text, camera, shapeVAO);
		MenuManager::menuManager.menus.push_back(newMenu);
		break;
	}
	case OPTIONS:
	{
		Menu* newMenu = new OptionsMenu(cursor, text, camera, shapeVAO);
		MenuManager::menuManager.menus.push_back(newMenu);
		/*	if (unitSpeed < 5)
			{
				unitSpeed = 5.0f;
				std::cout << "Speed up\n";
			}
			else
			{
				std::cout << "Speed down\n";
				unitSpeed = 2.5f;
			}*/
		break;
	}
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

UnitListMenu::UnitListMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO) : Menu(Cursor, Text, camera, shapeVAO)
{
	numberOfOptions = MenuManager::menuManager.playerManager->playerUnits.size();
	fullScreen = true;
	unitData.resize(numberOfOptions);
	for (int i = 0; i < numberOfOptions; i++)
	{
		auto unit = MenuManager::menuManager.playerManager->playerUnits[i];
		unitData[i] = std::make_pair(unit, unit->CalculateBattleStats());
	}
	if (MenuManager::menuManager.unitViewSortType > 0)
	{
		sortType = MenuManager::menuManager.unitViewSortType;
		SortView();
	}

	pageSortOptions.resize(6);
	pageSortOptions[GENERAL] = 6;
	pageSortOptions[EQUIPMENT] = 5;
	pageSortOptions[COMBAT_STATS] = 8;
	pageSortOptions[PERSONAL] = 5;
	pageSortOptions[WEAPON_RANKS] = 11;
	pageSortOptions[SKILLS] = 2;

	sortNames.resize(35);
	sortNames[0] = "Name";
	sortNames[1] = "Class";
	sortNames[2] = "LV";
	sortNames[3] = "EX";
	sortNames[4] = "HP";
	sortNames[5] = "MHP";
	sortNames[6] = "Name";
	sortNames[7] = "Equip";
	sortNames[8] = "Attack";
	sortNames[9] = "Hit";
	sortNames[10] = "Avoid";
	sortNames[11] = "Name";
	sortNames[12] = "Str";
	sortNames[13] = "Mag";
	sortNames[14] = "Skill";
	sortNames[15] = "Speed";
	sortNames[16] = "Luck";
	sortNames[17] = "Def";
	sortNames[18] = "Con";
	sortNames[19] = "Name";
	sortNames[20] = "Move";
	sortNames[21] = "Fatg";
	sortNames[22] = "Status";
	sortNames[23] = "Trvlr";
	sortNames[24] = "Name";

	//Weapon Profs will actually be represented by images
	sortNames[25] = "Sword";
	sortNames[26] = "Axe";
	sortNames[27] = "Lance";
	sortNames[28] = "Bow";
	sortNames[29] = "Staff";
	sortNames[30] = "Fire";
	sortNames[31] = "Thunder";
	sortNames[32] = "Wind";
	sortNames[33] = "Light";
	sortNames[34] = "Dark";
}

void UnitListMenu::Draw()
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(0, 0, 0.0f));

	model = glm::scale(model, glm::vec3(256, 224, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.0f, 0.8f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	model = glm::mat4();
	if (!sortMode)
	{
		model = glm::translate(model, glm::vec3(0, 56 + (16 * currentOption), 0.0f));
	}
	else
	{
		//This sucks dude in FE5 it varies by page
		model = glm::translate(model, glm::vec3(16 + (16 * sortIndicator), 40, 0.0f));
	}

	model = glm::scale(model, glm::vec3(16, 16, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	ResourceManager::GetShader("sprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	SBatch Batch;
	Batch.init();
	Batch.begin();

	std::string pageName = "";
	switch (currentPage)
	{
	case GENERAL:
		pageName = "General Data";
		text->RenderText("Class", 202, 96, 1);
		text->RenderText("LV", 480, 96, 1);
		text->RenderText("EX", 560, 96, 1);
		text->RenderText("HP/MHP", 690, 96, 1);

		for (int i = 0; i < numberOfOptions; i++)
		{
			auto unit = unitData[i].first;
			unit->Draw(&Batch, glm::vec2(16, 56 + 16 * i), true);

			int textY = 162 + 42 * i;
			text->RenderText(unit->name, 106, textY, 1.0f);

			text->RenderText(unit->unitClass, 202, textY, 1.0f);
			text->RenderTextRight(intToString(unit->level), 480, textY, 1.0f, 14);
			text->RenderTextRight(intToString(unit->experience), 560, textY, 1.0f, 14, glm::vec3(0.8f, 1.0f, 0.8f));
			text->RenderText(intToString(unit->currentHP) + "/", 690, textY, 1.0f);
			text->RenderText(intToString(unit->maxHP), 720, textY, 1.0f, glm::vec3(0.8f, 1.0f, 0.8f));
		}
		break;
	case EQUIPMENT:
		pageName = "Equipment";
		text->RenderText("Equip", 300, 96, 1);
		text->RenderText("Attack", 475, 96, 1);
		text->RenderText("Hit", 600, 96, 1);
		text->RenderText("Avoid", 700, 96, 1);

		for (int i = 0; i < numberOfOptions; i++)
		{
			auto unit = unitData[i].first;
			unit->Draw(&Batch, glm::vec2(16, 56 + 16 * i), true);

			int textY = 162 + 42 * i;
			text->RenderText(unit->name, 106, textY, 1.0f);
			//Should probably just save the battle stats to an array rather than recalculating them here over and over...
			auto battleStats = unitData[i].second;
			text->RenderText(unit->GetEquippedItem()->name, 300, textY, 1.0f); //need to account for no equipped
			text->RenderTextRight(intToString(battleStats.attackDamage), 500, textY, 1, 14);
			text->RenderTextRight(intToString(battleStats.hitAccuracy), 575, textY, 1, 14);
			text->RenderTextRight(intToString(battleStats.hitAvoid), 700, textY, 1.0f, 14, glm::vec3(0.8f, 1.0f, 0.8f));
		}
		break;
	case COMBAT_STATS:
		pageName = "Combat Stats";
		text->RenderText("Str", 250, 96, 1);
		text->RenderText("Mag", 325, 96, 1);
		text->RenderText("Skill", 400, 96, 1);
		text->RenderText("Speed", 475, 96, 1);
		text->RenderText("Luck", 550, 96, 1);
		text->RenderText("Def", 625, 96, 1);
		text->RenderText("Con", 700, 96, 1);

		for (int i = 0; i < numberOfOptions; i++)
		{
			auto unit = unitData[i].first;
			unit->Draw(&Batch, glm::vec2(16, 56 + 16 * i), true);

			int textY = 166 + 42 * i;
			text->RenderText(unit->name, 106, textY, 1.0f);
			text->RenderTextRight(intToString(unit->getStrength()), 275, textY, 1, 14);
			text->RenderTextRight(intToString(unit->getMagic()), 350, textY, 1, 14);
			text->RenderTextRight(intToString(unit->getSkill()), 425, textY, 1.0f, 14, glm::vec3(0.8f, 1.0f, 0.8f));
			text->RenderTextRight(intToString(unit->getSpeed()), 500, textY, 1.0f, 14, glm::vec3(0.8f, 1.0f, 0.8f));
			text->RenderTextRight(intToString(unit->getLuck()), 575, textY, 1.0f, 14, glm::vec3(0.8f, 1.0f, 0.8f));
			text->RenderTextRight(intToString(unit->getDefense()), 650, textY, 1.0f, 14, glm::vec3(0.8f, 1.0f, 0.8f));
			text->RenderTextRight(intToString(unit->getBuild()), 725, textY, 1.0f, 14, glm::vec3(0.8f, 1.0f, 0.8f));
		}
		break;
	case PERSONAL:
		pageName = "Personal Data";
		text->RenderText("Move", 275, 96, 1);
		text->RenderText("Fatg", 350, 96, 1);
		text->RenderText("Status", 450, 96, 1);
		text->RenderText("Trvlr", 600, 96, 1);

		for (int i = 0; i < numberOfOptions; i++)
		{
			auto unit = unitData[i].first;
			unit->Draw(&Batch, glm::vec2(16, 56 + 16 * i), true);

			int textY = 166 + 42 * i;
			text->RenderText(unit->name, 106, textY, 1.0f);
			text->RenderTextRight(intToString(unit->getMove()), 300, textY, 1, 14);
			text->RenderTextRight("--", 350, textY, 1, 14);
			text->RenderText("----", 450, textY, 1);
			if (unit->carriedUnit)
			{
				text->RenderText(unit->carriedUnit->name, 600, textY, 1);
			}
			else
			{
				text->RenderText("-----", 600, textY, 1);
			}
		}
		break;
	case WEAPON_RANKS:
		pageName = "Weapon Ranks";
		break;
	case SKILLS:
		pageName = "Skills";
		break;
	}
	Batch.end();
	Batch.renderBatch();
	text->RenderTextCenter(pageName, 106, 29, 1, 40, glm::vec3(0.9f, 0.9f, 1.0f));
	text->RenderText("Name", 106, 96, 1);
	text->RenderText(sortNames[sortType], 471, 29, 1); //sort type
	text->RenderText(intToString(currentPage + 1) , 700, 29, 1);
	text->RenderText("/", 711, 29, 1);
	text->RenderText(intToString(numberOfPages), 723, 29, 1);
}

void UnitListMenu::SelectOption()
{
	if (!sortMode)
	{
		cursor->position = unitData[currentOption].first->sprite.getPosition();
		cursor->focusedUnit = unitData[currentOption].first;
		CloseAndSaveView();
	}
	else
	{
		//Mysteriously, sorting by name in FE5 actually sorts by the default unit order
		if (sortIndicator == 0)
		{
			for (int i = 0; i < unitData.size(); i++)
			{
				auto unit = MenuManager::menuManager.playerManager->playerUnits[i];
				unitData[i] = std::make_pair(unit, unit->CalculateBattleStats());
			}
		}
		else
		{
			SortView();
		}
	}
}

void UnitListMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (inputManager.isKeyPressed(SDLK_UP))
	{
		currentOption--;
		if (currentOption < 0)
		{
			sortMode = true;
			//Set to sort mode
		}
	}
	else if (inputManager.isKeyPressed(SDLK_DOWN))
	{
		currentOption++;
		if (sortMode)
		{
			sortMode = false;
		}
		if (currentOption >= numberOfOptions)
		{
			currentOption = numberOfOptions;
		}
	}
	if (sortMode)
	{
		if (inputManager.isKeyPressed(SDLK_RIGHT))
		{
			if (currentPage < numberOfPages)
			{
				sortIndicator++;
				if (sortIndicator > pageSortOptions[currentPage] - 1)
				{
					sortIndicator = 0;
					currentPage++;
				}
				sortType++;
			}
		}
		else if (inputManager.isKeyPressed(SDLK_LEFT))
		{
			if (currentPage >= 0)
			{
				sortIndicator--;
				if (sortIndicator < 0)
				{
					if (currentPage > 0)
					{
						currentPage--;
						sortIndicator = pageSortOptions[currentPage] - 1;
						sortType--;
					}
					else
					{
						sortIndicator = 0;
					}
				}
				else
				{
					sortType--;
				}
			}
			std::cout << sortType << std::endl;
		}
	}
	else
	{
		if (inputManager.isKeyPressed(SDLK_RIGHT))
		{
			sortType -= sortIndicator;
			sortType += pageSortOptions[currentPage];
			currentPage++;
			if (currentPage >= numberOfPages - 1)
			{
				currentPage = numberOfPages - 1;
			}
			sortIndicator = 0;
		}
		else if (inputManager.isKeyPressed(SDLK_LEFT))
		{
			currentPage--;
			if (currentPage < 0)
			{
				currentPage = 0;
			}
			else
			{
				sortType -= sortIndicator;
				sortType -= pageSortOptions[currentPage];
				sortIndicator = 0;
			}
		}
	}
	if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
	else if (inputManager.isKeyPressed(SDLK_SPACE))
	{
		MenuManager::menuManager.AddUnitStatMenu(unitData[currentOption].first);
	}
	else if (inputManager.isKeyPressed(SDLK_z))
	{
		CloseAndSaveView();
	}
}

void UnitListMenu::CloseAndSaveView()
{
	if (sortIndicator > 0)
	{
		MenuManager::menuManager.unitViewSortType = sortType;
	}
	else
	{
		MenuManager::menuManager.unitViewSortType = 0;
	}
	ClearMenu();
}

void UnitListMenu::SortView()
{
	switch (sortType)
	{
	case 1:
		std::sort(unitData.begin(), unitData.end(), [](const auto& a, const auto& b)
			{
				return a.first->unitClass < b.first->unitClass;
			});
		break;
	case 2:
		std::sort(unitData.begin(), unitData.end(), [](const auto& a, const auto& b)
			{
				return a.first->level > b.first->level;
			});
		break;
	case 3:
		std::sort(unitData.begin(), unitData.end(), [](const auto& a, const auto& b)
			{
				return a.first->experience > b.first->experience;
			});
		break;
	case 4:
		std::sort(unitData.begin(), unitData.end(), [](const auto& a, const auto& b)
			{
				return a.first->currentHP > b.first->currentHP;
			});
		break;
	case 5:
		std::sort(unitData.begin(), unitData.end(), [](const auto& a, const auto& b)
			{
				return a.first->maxHP < b.first->maxHP;
			});
		break;
	case 7:
		std::sort(unitData.begin(), unitData.end(), [](const auto& a, const auto& b)
			{
				return a.first->GetEquippedItem()->name < b.first->GetEquippedItem()->name;
			});
		break;
	case 8:
		std::sort(unitData.begin(), unitData.end(), [](const auto& a, const auto& b)
			{
				return a.second.attackDamage > b.second.attackDamage;
			});
		break;
	case 9:
		std::sort(unitData.begin(), unitData.end(), [](const auto& a, const auto& b)
			{
				return a.second.hitAccuracy > b.second.hitAccuracy;
			});
		break;
	case 10:
		std::sort(unitData.begin(), unitData.end(), [](const auto& a, const auto& b)
			{
				return a.second.hitAvoid > b.second.hitAvoid;
			});
		break;
	case 12:
		std::sort(unitData.begin(), unitData.end(), [](const auto& a, const auto& b) {
			return a.first->getStrength() > b.first->getStrength();
			});
		break;
	case 13:
		std::sort(unitData.begin(), unitData.end(), [](const auto& a, const auto& b) {
			return a.first->getMagic() > b.first->getMagic();
			});
		break;
	case 14:
		std::sort(unitData.begin(), unitData.end(), [](const auto& a, const auto& b) {
			return a.first->getSkill() > b.first->getSkill();
			});
		break;
	case 15:
		std::sort(unitData.begin(), unitData.end(), [](const auto& a, const auto& b) {
			return a.first->getSpeed() > b.first->getSpeed();
			});
		break;
	case 16:
		std::sort(unitData.begin(), unitData.end(), [](const auto& a, const auto& b) {
			return a.first->getLuck() > b.first->getLuck();
			});
		break;
	case 17:
		std::sort(unitData.begin(), unitData.end(), [](const auto& a, const auto& b) {
			return a.first->getDefense() > b.first->getDefense();
			});
		break;
	case 18:
		std::sort(unitData.begin(), unitData.end(), [](const auto& a, const auto& b) {
			return a.first->getBuild() > b.first->getBuild();
			});
		break;
	case 20:
		std::sort(unitData.begin(), unitData.end(), [](const auto& a, const auto& b) {
			return a.first->getMove() > b.first->getMove();
			});
		break;
	case 25:
		std::sort(unitData.begin(), unitData.end(), [](const auto& a, const auto& b) {
			return a.first->weaponProficiencies[WeaponData::TYPE_SWORD] > b.first->weaponProficiencies[WeaponData::TYPE_SWORD];
			});
		break;
	case 26:
		std::sort(unitData.begin(), unitData.end(), [](const auto& a, const auto& b) {
			return a.first->weaponProficiencies[WeaponData::TYPE_AXE] > b.first->weaponProficiencies[WeaponData::TYPE_AXE];
			});
		break;
	case 27:
		std::sort(unitData.begin(), unitData.end(), [](const auto& a, const auto& b) {
			return a.first->weaponProficiencies[WeaponData::TYPE_LANCE] > b.first->weaponProficiencies[WeaponData::TYPE_LANCE];
			});
		break;
	case 28:
		std::sort(unitData.begin(), unitData.end(), [](const auto& a, const auto& b) {
			return a.first->weaponProficiencies[WeaponData::TYPE_BOW] > b.first->weaponProficiencies[WeaponData::TYPE_BOW];
			});
		break;
	case 29:
		std::sort(unitData.begin(), unitData.end(), [](const auto& a, const auto& b) {
			return a.first->weaponProficiencies[WeaponData::TYPE_STAFF] > b.first->weaponProficiencies[WeaponData::TYPE_STAFF];
			});
		break;
	case 30:
		std::sort(unitData.begin(), unitData.end(), [](const auto& a, const auto& b) {
			return a.first->weaponProficiencies[WeaponData::TYPE_FIRE] > b.first->weaponProficiencies[WeaponData::TYPE_FIRE];
			});
		break;
	case 31:
		std::sort(unitData.begin(), unitData.end(), [](const auto& a, const auto& b) {
			return a.first->weaponProficiencies[WeaponData::TYPE_THUNDER] > b.first->weaponProficiencies[WeaponData::TYPE_THUNDER];
			});
		break;
	case 32:
		std::sort(unitData.begin(), unitData.end(), [](const auto& a, const auto& b) {
			return a.first->weaponProficiencies[WeaponData::TYPE_WIND] > b.first->weaponProficiencies[WeaponData::TYPE_WIND];
			});
		break;
	case 33:
		std::sort(unitData.begin(), unitData.end(), [](const auto& a, const auto& b) {
			return a.first->weaponProficiencies[WeaponData::TYPE_LIGHT] > b.first->weaponProficiencies[WeaponData::TYPE_LIGHT];
			});
		break;
	case 34:
		std::sort(unitData.begin(), unitData.end(), [](const auto& a, const auto& b) {
			return a.first->weaponProficiencies[WeaponData::TYPE_DARK] > b.first->weaponProficiencies[WeaponData::TYPE_DARK];
			});
		break;
	}
}

StatusMenu::StatusMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO) : Menu(Cursor, Text, camera, shapeVAO)
{
	numberOfOptions = 2;
	fullScreen = true;
}

void StatusMenu::Draw()
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(0, 0, 0.0f));

	model = glm::scale(model, glm::vec3(256, 224, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.0f, 0.8f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	model = glm::mat4();
	model = glm::translate(model, glm::vec3(0, 200 + (16 * currentOption), 0.0f));
	model = glm::scale(model, glm::vec3(16, 16, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	text->RenderText("Objective", 25, 96, 1, glm::vec3(0.69f, 0.62f, 0.49f));
	text->RenderText("Captures/Wins", 25, 182, 1, glm::vec3(0.69f, 0.62f, 0.49f));
	text->RenderText("Turn", 150, 225, 1, glm::vec3(0.69f, 0.62f, 0.49f));
	text->RenderText("Funds", 425, 225, 1, glm::vec3(0.69f, 0.62f, 0.49f));

	text->RenderTextCenter("Chapter 1: The Warrior of Fiana", 0, 26, 1, 744); //Chapter Tile Goes here
	text->RenderTextCenter("Sieze the manor's gate", 171, 91, 1, 40); //Objective goes here

	text->RenderText("Combatants", 100, 310, 1);
	text->RenderText("Commander", 400, 310, 1);

	text->RenderText(MenuManager::menuManager.playerManager->playerUnits[0]->name, 100, 375, 1);
	text->RenderText("-----", 100, 431, 1);

	text->RenderText(MenuManager::menuManager.playerManager->playerUnits[0]->name, 450, 396, 1);
	text->RenderText(MenuManager::menuManager.playerManager->playerUnits[0]->unitClass, 400, 439, 1);
	text->RenderText("HP", 400, 530, 1, glm::vec3(0.69f, 0.62f, 0.49f));
	text->RenderText(intToString(MenuManager::menuManager.playerManager->playerUnits[0]->currentHP), 475, 530, 1);
	text->RenderText("/", 500, 530, 1);
	text->RenderText(intToString(MenuManager::menuManager.playerManager->playerUnits[0]->maxHP), 515, 530, 1);
	text->RenderTextRight(intToString(MenuManager::menuManager.playerManager->playerUnits[0]->level), 575, 487, 1, 14);


	ResourceManager::GetShader("sprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	SBatch Batch;
	Batch.init();
	Batch.begin();
	MenuManager::menuManager.playerManager->playerUnits[0]->Draw(&Batch, glm::vec2(128, 144), true);
	Batch.end();
	Batch.renderBatch();

}

void StatusMenu::SelectOption()
{
}

OptionsMenu::OptionsMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO) : Menu(Cursor, Text, camera, shapeVAO)
{
	numberOfOptions = 9;
	fullScreen = true;
}

void OptionsMenu::Draw()
{
	//Appears to be a bit of black around/under the main background on the edges, keep in mind
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(0, 0, 0.0f));

	model = glm::scale(model, glm::vec3(256, 224, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.0f, 0.8f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	model = glm::mat4();
	model = glm::translate(model, glm::vec3(0, 0, 0.0f));

	model = glm::scale(model, glm::vec3(256, 32, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.0f, 0.3f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	model = glm::mat4();
	model = glm::translate(model, glm::vec3(8, indicatorY, 0.0f));
	model = glm::scale(model, glm::vec3(16, 16, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	int optionNameX = 125;
	int selectionXStart = 400;
	//Distance of about 64 for each option
	text->RenderText("Animations", optionNameX, 117 - (yOffset), 1);
	RenderText("Normal", selectionXStart, 117 - (yOffset), 1, Settings::settings.mapAnimations == 0);
	int xOffset = 0;
	xOffset += selectionXStart + text->GetTextWidth("Normal", 1) + 50;
	RenderText("Map", xOffset, 117 - (yOffset), 1, Settings::settings.mapAnimations == 1);
	xOffset += text->GetTextWidth("Map", 1) + 50;
	RenderText("By Unit", xOffset, 117 - (yOffset), 1, Settings::settings.mapAnimations == 2);

	text->RenderText("Terrain Window", optionNameX, 181 - (yOffset), 1);
	RenderText("On", selectionXStart, 181 - (yOffset), 1, Settings::settings.showTerrain);
	RenderText("Off", selectionXStart + text->GetTextWidth("On", 1) + 50, 181 - (yOffset), 1, !Settings::settings.showTerrain);

	text->RenderText("Autocursor", optionNameX, 245 - (yOffset), 1);
	RenderText("On", selectionXStart, 245 - (yOffset), 1, Settings::settings.autoCursor);
	RenderText("Off", selectionXStart + text->GetTextWidth("On", 1) + 50, 245 - (yOffset), 1, !Settings::settings.autoCursor);

	text->RenderText("Text Speed", optionNameX, 309 - (yOffset), 1);
	RenderText("Slow", selectionXStart, 309 - (yOffset), 1, Settings::settings.textSpeed == 0);
	xOffset = 0;
	xOffset += selectionXStart + text->GetTextWidth("Slow", 1) + 50;
	RenderText("Normal", xOffset, 309 - (yOffset), 1, Settings::settings.textSpeed == 1);
	xOffset += text->GetTextWidth("Normal", 1) + 50;
	RenderText("Fast", xOffset, 309 - (yOffset), 1, Settings::settings.textSpeed == 2);

	text->RenderText("Unit Speed", optionNameX, 373 - (yOffset), 1);
	RenderText("Normal", selectionXStart, 373 - (yOffset), 1, Settings::settings.unitSpeed < 5);
	RenderText("Fast", selectionXStart + text->GetTextWidth("Normal", 1) + 50, 373 - (yOffset), 1, Settings::settings.unitSpeed >= 5);

	text->RenderText("Audio", optionNameX, 437 - (yOffset), 1);
	RenderText("Stereo", selectionXStart, 437 - (yOffset), 1, Settings::settings.sterero);
	RenderText("Mono", selectionXStart + text->GetTextWidth("Stereo", 1) + 50, 437 - (yOffset), 1, !Settings::settings.sterero);

	text->RenderText("Music", optionNameX, 501 - (yOffset), 1);
	text->RenderText("Volume", optionNameX, 565 - (yOffset), 1);
	text->RenderText("Window Tile", optionNameX, 629 - (yOffset), 1);
	text->RenderText("Window Color", optionNameX, 693 - (yOffset), 1);

	model = glm::mat4();
	model = glm::translate(model, glm::vec3(0, 193, 0.0f));

	model = glm::scale(model, glm::vec3(256, 31, 0.0f));

	ResourceManager::GetShader("shape").Use().SetVector3f("shapeColor", glm::vec3(0.2f, 0.2f, 1.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	//Drawing this again so it is drawn over the text; this is temporary, I'll find a better way of handling this later
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(0, 0, 0.0f));

	model = glm::scale(model, glm::vec3(256, 32, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.0f, 0.3f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	text->RenderText("Configuration", 159, 26, 1);

}

void OptionsMenu::SelectOption()
{
}

void OptionsMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (inputManager.isKeyPressed(SDLK_UP))
	{
		currentOption--;
		if (currentOption < 0)
		{
			currentOption = 0;
		}
		else
		{
			indicatorY -= indicatorIncrement;
			if (indicatorY < 40)
			{
				indicatorY = 40;
				up = true;
				goal = yOffset - 64;
			}
		}
	}
	else if (inputManager.isKeyPressed(SDLK_DOWN))
	{
		currentOption++;
		if (currentOption > numberOfOptions)
		{
			currentOption = numberOfOptions;
		}
		else
		{
			indicatorY += indicatorIncrement;
			if (indicatorY > 160)
			{
				indicatorY = 160;
				down = true;
				goal = yOffset + 64;
			}
		}
	}

	if (inputManager.isKeyPressed(SDLK_RIGHT))
	{
		switch (currentOption)
		{
		case 0:
			Settings::settings.mapAnimations++;
			if (Settings::settings.mapAnimations > 2)
			{
				Settings::settings.mapAnimations = 2;
			}
			break;
		case 1:
			Settings::settings.showTerrain = false;
			break;
		case 2:
			Settings::settings.autoCursor = false;
			break;
		case 3:
			Settings::settings.textSpeed++;
			if (Settings::settings.textSpeed > 2)
			{
				Settings::settings.textSpeed = 2;
			}
			break;
		case 4:
			Settings::settings.unitSpeed = 5;
			break;
		case 5:
			Settings::settings.sterero = false;
			break;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_LEFT))
	{
		switch (currentOption)
		{
		case 0:
			Settings::settings.mapAnimations--;
			if (Settings::settings.mapAnimations < 0)
			{
				Settings::settings.mapAnimations = 0;
			}
			break;
		case 1:
			Settings::settings.showTerrain = true;
			break;
		case 2:
			Settings::settings.autoCursor = true;
			break;
		case 3:
			Settings::settings.textSpeed--;
			if (Settings::settings.textSpeed < 0)
			{
				Settings::settings.textSpeed = 0;
			}
			break;
		case 4:
			Settings::settings.unitSpeed = 2.5f;
			break;
		case 5:
			Settings::settings.sterero = true;
			break;
		}
	}
	int rate = 960;
	if (down)
	{
		yOffset += rate * deltaTime;
		if (yOffset >= goal)
		{
			yOffset = goal;
			down = false;
		}
	}
	else if (up)
	{
		yOffset -= rate * deltaTime;
		if (yOffset <= goal)
		{
			yOffset = goal;
			up = false;
		}
	}

	if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
	else if (inputManager.isKeyPressed(SDLK_z))
	{
		ClearMenu();
	}
}

void OptionsMenu::RenderText(std::string toWrite, float x, float y, float scale, bool selected)
{
	glm::vec3 grey = glm::vec3(0.64f);
	glm::vec3 selectedColor = glm::vec3(0.77, 0.92, 1.0f);
	glm::vec3 color = selected ? selectedColor : grey;
	text->RenderText(toWrite, x, y, 1, color);
}

VendorMenu::VendorMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, Unit* buyer, Vendor* vendor) : 
	Menu(Cursor, Text, camera, shapeVAO), buyer(buyer), vendor(vendor)
{
	fullScreen = true;
	numberOfOptions = vendor->items.size();

	textManager.textLines.clear();

	textManager.textLines.push_back(SpeakerText{ nullptr, 0, "Got a selection of GOOD things on sale, stranger.<2"});
	textManager.textLines.push_back(SpeakerText{ nullptr, 0, "What are ya buyin'?<2"});
	textManager.textLines.push_back(SpeakerText{ nullptr, 0, "What are ya sellin'?<2"});
	textManager.textLines.push_back(SpeakerText{ nullptr, 0, "Is that all stranger?<2"});
	textManager.textLines.push_back(SpeakerText{ nullptr, 0, "Come back any time<3"});
	textManager.textLines.push_back(SpeakerText{ nullptr, 0, "Ahhh, I'll buy it at a HIGH price.<2" });
	textManager.textLines.push_back(SpeakerText{ nullptr, 0, "Hehehe, thank you.<2" });
	textManager.textLines.push_back(SpeakerText{ nullptr, 0, "Good choice, stranger.<2" });
	textManager.textLines.push_back(SpeakerText{ nullptr, 0, "Not enough cash, stranger.<2" });
	textManager.textLines.push_back(SpeakerText{ nullptr, 0, "Not enough space, stranger.<2" });
	textManager.textLines.push_back(SpeakerText{ nullptr, 0, "You've got nothing to sell, stranger.<2" });

	testText.position = glm::vec2(275.0f, 48.0f);
	testText.displayedPosition = testText.position;
	testText.charsPerLine = 55;
	testText.nextIndex = 55;

	textManager.textObjects.clear();
	textManager.textObjects.push_back(testText);
	textManager.init();
	textManager.active = true;

	state = GREETING;
}

void VendorMenu::Draw()
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(0, 0, 0.0f));

	model = glm::scale(model, glm::vec3(256, 80, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.2f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	model = glm::mat4();
	model = glm::translate(model, glm::vec3(0, 80, 0.0f));

	model = glm::scale(model, glm::vec3(80, 144, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.258f, 0.188f, 0.16f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	model = glm::mat4();
	model = glm::translate(model, glm::vec3(80, 80, 0.0f));

	model = glm::scale(model, glm::vec3(176, 144, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.358f, 0.188f, 0.16f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	model = glm::mat4();
	if (!textManager.active && !delay)
	{
		switch (state)
		{
		case GREETING:
			model = glm::translate(model, glm::vec3(144 + (46 * !buying), 50, 0.0f));
			text->RenderText("Buy", 500, 133, 1);
			text->RenderText("Sell", 646, 133, 1);
			break;
		case BUYING:
			model = glm::translate(model, glm::vec3(88, 92 + 16 * currentOption, 0.0f));
			break;
		case SELLING:
			model = glm::translate(model, glm::vec3(88, 92 + 16 * currentOption, 0.0f));
			break;
		case CONFIRMING:
			model = glm::translate(model, glm::vec3(144 + (46 * !confirm), 50, 0.0f));
			text->RenderText("Yes", 500, 133, 1);
			text->RenderText("No", 646, 133, 1);
			break;
		}
		model = glm::scale(model, glm::vec3(16, 16, 0.0f));

		ResourceManager::GetShader("shape").Use().SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

		ResourceManager::GetShader("shape").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
	}

	int xPos = 375;
	int yPos = 246;
	auto items = ItemManager::itemManager.items;
	if (buying)
	{
		for (int i = 0; i < vendor->items.size(); i++)
		{
			auto item = items[vendor->items[i]];
			text->RenderText(item.name, xPos, yPos + 43 * i + 1, 1);
			text->RenderTextRight(intToString(item.maxUses), 575, yPos + 43 * i + 1, 1, 14);
			text->RenderTextRight(intToString(item.value), 675, yPos + 43 * i + 1, 1, 40); //price
		}
	}
	else
	{
		for (int i = 0; i < buyer->inventory.size(); i++)
		{
			auto item = buyer->inventory[i];
			text->RenderText(item->name, xPos, yPos + 43 * i + 1, 1);
			text->RenderTextRight(intToString(item->remainingUses), 575, yPos + 43 * i + 1, 1, 14);
			if (item->value > 0)
			{
				text->RenderTextRight(intToString(item->value * 0.5f), 675, yPos + 43 * i + 1, 1, 40);
			}
			else
			{
				text->RenderTextRight("----", 675, yPos + 43 * i + 1, 1, 40);
			}
		}
	}
	text->RenderTextRight(intToString(MenuManager::menuManager.playerManager->funds), 75, 551, 1, 40);
	text->RenderText("G", 175, 551, 1);

	if (state == BUYING || state == SELLING)
	{
		Item currentItem;
		if (buying)
		{
			currentItem = items[vendor->items[currentOption]];
		}
		else
		{
			currentItem = *buyer->inventory[currentOption]; //Need a way to handle 
		}
		int yPosition = 246;
		if (currentItem.isWeapon)
		{
			int offSet = 30;
			auto weaponData = ItemManager::itemManager.weaponData[currentItem.ID];

			int xStatName = 50;
			int xStatValue = 150;
			text->RenderText("Type", xStatName, yPosition, 1);
			text->RenderTextRight(intToString(weaponData.type), xStatValue, yPosition, 1, 14);
			yPosition += offSet;
			text->RenderText("Hit", xStatName, yPosition, 1);
			text->RenderTextRight(intToString(weaponData.hit), xStatValue, yPosition, 1, 14);
			yPosition += offSet;
			text->RenderText("Might", xStatName, yPosition, 1);
			text->RenderTextRight(intToString(weaponData.might), xStatValue, yPosition, 1, 14);
			yPosition += offSet;
			text->RenderText("Crit", xStatName, yPosition, 1);
			text->RenderTextRight(intToString(weaponData.crit), xStatValue, yPosition, 1, 14);
			yPosition += offSet;
			text->RenderText("Range", xStatName, yPosition, 1);
			text->RenderTextRight(intToString(weaponData.minRange), xStatValue, yPosition, 1, 14);
			yPosition += offSet;
			text->RenderText("Weight", xStatName, yPosition, 1);
			text->RenderTextRight(intToString(weaponData.weight), xStatValue, yPosition, 1, 14);
			yPosition += offSet;
		}
		else
		{
			text->RenderText(currentItem.description, 50, yPosition, 1);
		}
	}

	if (textManager.ShowText())
	{
		textManager.Draw(text);
	}
}

void VendorMenu::SelectOption()
{
	switch (state)
	{
	case GREETING:
		if (buying)
		{
			textManager.init(1);
			state = BUYING;
		}
		else
		{
			if (buyer->inventory.size() > 0)
			{
				textManager.init(2);
				state = SELLING;
			}
			else
			{
				textManager.init(10);
			}
		}
		ActivateText();
		break;
	case BUYING:
	{
		auto items = ItemManager::itemManager.items;
		if (MenuManager::menuManager.playerManager->funds < items[vendor->items[currentOption]].value)
		{
			textManager.init(8);
		}
		else if (buyer->inventory.size() == 8)
		{
			textManager.init(9);
		}
		else
		{
			state = CONFIRMING;
			confirm = true;
			textManager.init(7);
		}
		break;
	}
	case SELLING:
		textManager.init(5);
		state = CONFIRMING;
		confirm = true;
		break;
	case CONFIRMING:
		if (confirm)
		{
			if (buying)
			{
				auto items = ItemManager::itemManager.items;
				MenuManager::menuManager.playerManager->funds -= items[vendor->items[currentOption]].value;
				buyer->addItem(vendor->items[currentOption]);
				textManager.init(6);
				state = BUYING;
			}
			else
			{
				MenuManager::menuManager.playerManager->funds += buyer->inventory[currentOption]->value * 0.5f;
				buyer->dropItem(currentOption);
				numberOfOptions--;
				if (numberOfOptions == 0)
				{
					state = GREETING;
					textManager.init(3);
				}
				else
				{
					textManager.init(6);
					if (currentOption == numberOfOptions)
					{
						currentOption--;
					}
					state = SELLING;
				}
			}
			MenuManager::menuManager.mustWait = true;
		}
		else
		{
			if (buying)
			{
				state = BUYING;
			}
			else
			{
				state = SELLING;
			}
			textManager.init(3);
		}

	//	confirming = false;
		break;
	}
	if (state != GREETING)
	{
		ActivateText();
	}
}

void VendorMenu::ActivateText()
{
	textManager.textObjects[0].displayedText = "";
	textManager.active = true;
	delay = true;
}

void VendorMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (textManager.active)
	{
		textManager.Update(deltaTime, inputManager, *cursor);
	}
	else if (delay)
	{
		delayTimer += deltaTime;
		if (delayTimer >= delayTime)
		{
			delayTimer = 0.0f;
			delay = false;
		}
	}
	else if (state == LEAVING)
	{
		Menu::CancelOption();
	}
	else
	{
		if (state == CONFIRMING)
		{
			if (inputManager.isKeyPressed(SDLK_RIGHT))
			{
				confirm = false;
			}
			else if (inputManager.isKeyPressed(SDLK_LEFT))
			{
				confirm = true;
			}
		}
		else if (state == BUYING || state == SELLING)
		{
			if (inputManager.isKeyPressed(SDLK_UP))
			{
				currentOption--;
				if (currentOption < 0)
				{
					currentOption = numberOfOptions - 1;
				}
			}
			else if (inputManager.isKeyPressed(SDLK_DOWN))
			{
				currentOption++;
				if (currentOption >= numberOfOptions)
				{
					currentOption = 0;
				}
			}
		}
		else if(state == GREETING)
		{
			if (inputManager.isKeyPressed(SDLK_RIGHT))
			{
				buying = false;
				numberOfOptions = buyer->inventory.size();
			}
			else if (inputManager.isKeyPressed(SDLK_LEFT))
			{
				buying = true;
				numberOfOptions = vendor->items.size();
			}
		}
		if (inputManager.isKeyPressed(SDLK_RETURN))
		{
			SelectOption();
		}
		else if (inputManager.isKeyPressed(SDLK_z))
		{
			CancelOption();
		}
	}
}

void VendorMenu::CancelOption()
{
	if (state == GREETING)
	{
		state = LEAVING;
		textManager.init(4);
	}
	else if (state == BUYING || state == SELLING)
	{
		state = GREETING;
		textManager.init(3);

	}
	else if (state == CONFIRMING)
	{
		if (buying)
		{
			state = BUYING;
		}
		else
		{
			state = SELLING;
		}
		textManager.init(3);
	}
	ActivateText();
}

FullInventoryMenu::FullInventoryMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, int newItem, SpriteRenderer* Renderer) :
	Menu(Cursor, Text, camera, shapeVAO), newItem(newItem)
{
	numberOfOptions = cursor->selectedUnit->inventory.size() + 1;
	currentStats = cursor->selectedUnit->CalculateBattleStats();
	selectedStats = cursor->selectedUnit->CalculateBattleStats(cursor->selectedUnit->inventory[currentOption]->ID);
	renderer = Renderer;
	itemIconUVs = MenuManager::menuManager.itemIconUVs;
}

void FullInventoryMenu::Draw()
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	//Inventory box
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(24, 72, 0.0f));
	model = glm::scale(model, glm::vec3(122, 146, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 1.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	//Item info
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(152, 72, 0.0f));

	model = glm::scale(model, glm::vec3(98, 130, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 1.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	//"Too many items" box
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(128, 0, 0.0f));

	model = glm::scale(model, glm::vec3(122, 66, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 1.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	//Selection
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(24, 82 + (16 * currentOption), 0.0f));

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
	ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());

	for (int i = 0; i < numberOfOptions - 1; i++)
	{
		color = glm::vec3(1);
		int yPosition = 225 + i * 42;
		auto item = inventory[i];
		text->RenderText(item->name, 175, yPosition, 1, color);
		text->RenderTextRight(intToString(item->remainingUses), 375, 230 + i * 42, 1, 14, color);

		ResourceManager::GetShader("Nsprite").Use();
		auto texture = ResourceManager::GetTexture("icons");

		renderer->setUVs(itemIconUVs[item->ID]);
		renderer->DrawSprite(texture, glm::vec2(40, 82 + 16 * i), 0.0f, cursor->dimensions);
	}
	auto item = ItemManager::itemManager.items[newItem];
	text->RenderText(item.name, 175, 225 + (numberOfOptions - 1) * 42, 1, grey);
	text->RenderTextRight(intToString(item.remainingUses), 375, 230 + (numberOfOptions - 1) * 42, 1, 14, grey);

	ResourceManager::GetShader("Nsprite").Use();
	auto texture = ResourceManager::GetTexture("icons");

	renderer->setUVs(itemIconUVs[item.ID]);
	renderer->DrawSprite(texture, glm::vec2(40, 82 + 16 * (numberOfOptions - 1)), 0.0f, cursor->dimensions);
	Item focusedItem = item;
	if (currentOption < numberOfOptions - 1)
	{
		focusedItem = *inventory[currentOption];
	}
	if (focusedItem.isWeapon)
	{
		//temp
		selectedStats = cursor->selectedUnit->CalculateBattleStats(focusedItem.ID);

		int offSet = 42;
		int yPosition = 225;
		auto weaponData = ItemManager::itemManager.weaponData[focusedItem.ID];

		int xStatName = 525;
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
		else if (selectedStats.attackDamage < currentStats.attackDamage)
		{
			text->RenderTextRight("v v v", xStatValue + 60, yPosition, 1, 14);
		}
		yPosition += offSet;
		text->RenderText("Hit", xStatName, yPosition, 1);
		text->RenderTextRight(intToString(selectedStats.hitAccuracy), xStatValue, yPosition, 1, 14);
		if (selectedStats.hitAccuracy > currentStats.hitAccuracy)
		{
			text->RenderTextRight("^^^", xStatValue + 60, yPosition, 1, 14);
		}
		else if (selectedStats.hitAccuracy < currentStats.hitAccuracy)
		{
			text->RenderTextRight("v v v", xStatValue + 60, yPosition, 1, 14);
		}
		yPosition += offSet;
		text->RenderText("Crit", xStatName, yPosition, 1);
		text->RenderTextRight(intToString(selectedStats.hitCrit), xStatValue, yPosition, 1, 14);
		if (selectedStats.hitCrit > currentStats.hitCrit)
		{
			text->RenderTextRight("^^^", xStatValue + 60, yPosition, 1, 14);
		}
		else if (selectedStats.hitCrit < currentStats.hitCrit)
		{
			text->RenderTextRight("v v v", xStatValue + 60, yPosition, 1, 14);
		}
		yPosition += offSet;
		text->RenderText("Avo", xStatName, yPosition, 1);
		text->RenderTextRight(intToString(selectedStats.hitAvoid), xStatValue, yPosition, 1, 14);
		if (selectedStats.hitAvoid > currentStats.hitAvoid)
		{
			text->RenderTextRight("^^^", xStatValue + 60, yPosition, 1, 14);
		}
		else if (selectedStats.hitAvoid < currentStats.hitAvoid)
		{
			text->RenderTextRight("v v v", xStatValue + 60, yPosition, 1, 14);
		}
	}
	else
	{
		text->RenderText(focusedItem.description, 500, 225, 1);
	}
}

void FullInventoryMenu::SelectOption()
{
	if (currentOption < numberOfOptions)
	{
		//Would be sending the item to supply, but that's not part of this project
		cursor->selectedUnit->dropItem(currentOption);
		cursor->selectedUnit->addItem(newItem);
	}
	else
	{
		//Likewise, would be sending the new item to supply if we had one
	}
	ClearMenu();
}

void FullInventoryMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (inputManager.isKeyPressed(SDLK_UP))
	{
		currentOption--;
		if (currentOption < 0)
		{
			currentOption = numberOfOptions - 1;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_DOWN))
	{
		currentOption++;
		if (currentOption >= numberOfOptions)
		{
			currentOption = 0;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
}
