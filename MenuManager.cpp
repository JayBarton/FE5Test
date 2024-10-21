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

#include "UnitResources.h"

#include <SDL.h>
#include <sstream>
#include <algorithm> 

Menu::Menu(Cursor* Cursor, TextRenderer* Text, Camera* Camera, int shapeVAO, SpriteRenderer* Renderer)
	: cursor(Cursor), text(Text), camera(Camera), shapeVAO(shapeVAO), Renderer(Renderer)
{
	ResourceManager::PlaySound("select2");
}

void Menu::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (inputManager.KeyDownDelay(SDLK_UP))
	{
		if (inputManager.isKeyPressed(SDLK_UP))
		{
			PreviousOption();
		}
		else
		{
			PreviousOptionStop();
		}
	}
	else if (inputManager.KeyDownDelay(SDLK_DOWN))
	{
		if (inputManager.isKeyPressed(SDLK_DOWN))
		{
			NextOption();
		}
		else
		{
			NextOptionStop();
		}
	}
	if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
	else if (inputManager.isKeyPressed(SDLK_z))
	{
		CancelOption();
		currentOption = 0;
	}
}

void Menu::DelayedExit()
{
	ClearMenu();
}

void Menu::NextOption()
{
	currentOption++;
	if (currentOption >= numberOfOptions)
	{
		currentOption = 0;
	}
	ResourceManager::PlaySound("optionSelect1");
}

void Menu::NextOptionStop()
{
	currentOption++;
	if (currentOption >= numberOfOptions)
	{
		currentOption = numberOfOptions - 1;
	}
	else
	{
		ResourceManager::PlaySound("optionSelect1");
	}
}

void Menu::PreviousOption()
{
	currentOption--;
	if (currentOption < 0)
	{
		currentOption = numberOfOptions - 1;
	}
	ResourceManager::PlaySound("optionSelect1");
}

void Menu::PreviousOptionStop()
{
	currentOption--;
	if (currentOption < 0)
	{
		currentOption = 0;
	}
	else
	{
		ResourceManager::PlaySound("optionSelect1");
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
	ResourceManager::PlaySound("cancel");

	ClearMenu();

}

void Menu::CancelOption(int num)
{
	for (int i = 0; i < num; i++)
	{
		MenuManager::menuManager.PreviousMenu();
	}
	ResourceManager::PlaySound("cancel"); 
}

void Menu::DrawBox(glm::ivec2 position, int width, int height)
{
	Renderer->shader = ResourceManager::GetShader("slice");

	ResourceManager::GetShader("slice").Use();
	ResourceManager::GetShader("slice").SetMatrix4("projection", camera->getOrthoMatrix());

	auto texture = ResourceManager::GetTexture("UIStuff");

	glm::vec4 uvs = MenuManager::menuManager.boxesUVs[0];
	glm::vec2 size = glm::vec2(width, height);
	float borderSize = 10.0f;
	ResourceManager::GetShader("slice").SetVector2f("u_dimensions", borderSize / size.x, borderSize / size.y);
	ResourceManager::GetShader("slice").SetVector2f("u_border", borderSize / 32.0f, borderSize / 32.0f);
	ResourceManager::GetShader("slice").SetVector4f("bounds", uvs.x, uvs.y, uvs.z, uvs.w);

	Renderer->setUVs();
	int patternIndex = Settings::settings.backgroundPattern;

	glm::vec4 uv = MenuManager::menuManager.patternUVs[patternIndex];
	GLfloat verticies[] =
	{
		uv.x, uv.w,
		uv.y, uv.z,
		uv.x, uv.z,

		uv.x, uv.w,
		uv.y, uv.w,
		uv.y, uv.z
	};
	ResourceManager::GetShader("slice").SetVector2fv("backgroundUVs", 12, verticies);
	auto inColor = Settings::settings.backgroundColors[patternIndex];
	glm::vec3 topColor(inColor[0], inColor[1], inColor[2]);
	glm::vec3 bottomColor(inColor[3], inColor[4], inColor[5]);
	ResourceManager::GetShader("slice").SetVector2f("imageScale", size / glm::vec2(64, 32));
	ResourceManager::GetShader("slice").SetVector2f("sheetScale", glm::vec2(64, 32) / glm::vec2(128, 32));
	ResourceManager::GetShader("slice").SetVector3f("topColor", topColor / 255.0f);
	ResourceManager::GetShader("slice").SetVector3f("bottomColor", bottomColor / 255.0f);
	ResourceManager::GetShader("slice").SetInteger("index", patternIndex);

	auto texture2 = ResourceManager::GetTexture("testpattern");
	glActiveTexture(GL_TEXTURE1);
	texture2.Bind();
	Renderer->DrawSprite(texture, glm::vec2(position.x, position.y), 0.0f, size);

	Renderer->shader = ResourceManager::GetShader("Nsprite");
}

void Menu::DrawPattern(glm::vec2 size, glm::vec2 pos, bool gray)
{
	//Duplicating this in a couple of places unfortunately
	int patternID = Settings::settings.backgroundPattern;
	auto inColor = Settings::settings.backgroundColors[patternID];
	glm::vec3 topColor = glm::vec3(inColor[0], inColor[1], inColor[2]);
	glm::vec3 bottomColor = glm::vec3(inColor[3], inColor[4], inColor[5]);

	Renderer->shader = ResourceManager::GetShader("patterns");
	ResourceManager::GetShader("patterns").Use();
	ResourceManager::GetShader("patterns").SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("patterns").SetInteger("gray", gray);
	if (!gray)
	{
		ResourceManager::GetShader("patterns").SetVector3f("topColor", topColor / 255.0f);
		ResourceManager::GetShader("patterns").SetVector3f("bottomColor", bottomColor / 255.0f);
	}
	ResourceManager::GetShader("patterns").SetInteger("index", patternID);

	ResourceManager::GetShader("patterns").SetVector2f("scale", size / glm::vec2(64, 32));
	//ResourceManager::GetShader("patterns").SetVector2f("sheetScale", glm::vec2(64, 32) / glm::vec2(128, 32));

	auto patternTexture = ResourceManager::GetTexture("testpattern");

	Renderer->setUVs(MenuManager::menuManager.patternUVs[patternID]);
	Renderer->DrawSprite(patternTexture, pos, 0.0f, size);

	Renderer->shader = ResourceManager::GetShader("Nsprite");
}

void Menu::HandleFadeIn(float deltaTime)
{
	fadeInAlpha += deltaTime * 4;
	if (fadeInAlpha >= 1.0f)
	{
		fadeInAlpha = 1.0f;
		fullFadeIn = false;
		fullFadeOut = true;
		fullScreen = !exitMenu;
	}
}

void Menu::HandleFadeOut(float deltaTime)
{
	fadeInAlpha -= deltaTime * 4;
	if (fadeInAlpha <= 0.0f)
	{
		fadeInAlpha = 0.0f;
		fullFadeOut = false;
		if (exitMenu)
		{
			DelayedExit();
		}
	}
}

void Menu::DrawFadeIn()
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", fadeInAlpha);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(0, 0, 0.0f));

	model = glm::scale(model, glm::vec3(256, 224, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.0f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

void Menu::ClearMenu()
{
	MenuManager::menuManager.ClearMenu();
}

UnitOptionsMenu::UnitOptionsMenu(Cursor* Cursor, TextRenderer* Text, Camera* Camera, int shapeVAO, SpriteRenderer* Renderer)
	: Menu(Cursor, Text, Camera, shapeVAO, Renderer)
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
							std::vector<Unit*> unitList;
							unitsToAttack.push_back(unitList);
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
		Menu* newMenu = new SelectWeaponMenu(cursor, text, camera, shapeVAO, Renderer, validWeapons, unitsToAttack);
		MenuManager::menuManager.menus.push_back(newMenu);
		break;
	}
	case CAPTURE:
	{
		std::vector<Item*> validWeapons;
		std::vector<std::vector<Unit*>> unitsToAttack;
		auto playerWeapons = playerUnit->GetOrderedWeapons();
		for (int i = 0; i < playerWeapons.size(); i++)
		{
			bool weaponInRange = false;
			auto weapon = playerUnit->GetWeaponData(playerWeapons[i]);
			if (playerUnit->canUse(weapon))
			{
				if (1 <= weapon.maxRange && 1 >= weapon.minRange)
				{
					validWeapons.push_back(playerWeapons[i]);
				}
			}
		}
		unitsToAttack.resize(validWeapons.size());
		for (int i = 0; i < unitsToAttack.size(); i++)
		{
			unitsToAttack[i] = unitsInCaptureRange;
		}
		Menu* newMenu = new SelectWeaponMenu(cursor, text, camera, shapeVAO, Renderer, validWeapons, unitsToAttack, true);
		MenuManager::menuManager.menus.push_back(newMenu);
		break;
	}
	case DROP:
	{
		Menu* newMenu = new DropMenu(cursor, text, camera, shapeVAO, Renderer, dropPositions);
		MenuManager::menuManager.menus.push_back(newMenu);
	}
		break;
	case RELEASE:
	{
		auto playerUnit = cursor->selectedUnit;
		auto releasedEnemy = playerUnit->carriedUnit;
		heldEnemy = false;

		Menu* newMenu = new UnitMovement(cursor, text, camera, shapeVAO, Renderer, releasedEnemy, nullptr, 3, playerUnit->sprite.getPosition() + glm::vec2(0, 16));
		MenuManager::menuManager.menus.push_back(newMenu);
		break;
	}
	case RESCUE:
	{
		Menu* newMenu = new SelectRescueUnit(cursor, text, camera, shapeVAO, Renderer, rescueUnits);
		MenuManager::menuManager.menus.push_back(newMenu);
		break;
	}
	case TRANSFER:
	{
		Menu* newMenu = new SelectTransferUnit(cursor, text, camera, shapeVAO, Renderer, transferUnits);
		MenuManager::menuManager.menus.push_back(newMenu);
		break;
	}
	case TRADE:
	{
		Menu* newMenu = new SelectTradeUnit(cursor, text, camera, shapeVAO, Renderer, tradeUnits);
		MenuManager::menuManager.menus.push_back(newMenu);
		break;
	}
	case ITEMS:
		MenuManager::menuManager.AddMenu(1);
		break;
	case DISMOUNT:
	{
		playerUnit->MountAction(false);
		MenuManager::menuManager.mustWait = true;
		MenuManager::menuManager.mountActionTaken = true;
		GetOptions();
		ResourceManager::PlaySound("select2");
		break;
	}
	case MOUNT:
	{
		playerUnit->MountAction(true);
		playerUnit->mount->remainingMoves = 0;
		MenuManager::menuManager.mustWait = true;
		MenuManager::menuManager.mountActionTaken = true;
		GetOptions();
		ResourceManager::PlaySound("select2");
		break;
	}
	case TALK:
	{
		Menu* newMenu = new SelectTalkMenu(cursor, text, camera, shapeVAO, Renderer, talkUnits);
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
		ResourceManager::PlaySound("select2");

		ClearMenu();

		break;
	}
	case VENDOR:
	{
		auto position = playerUnit->sprite.getPosition();
		auto vendor = TileManager::tileManager.getVendor(position.x, position.y);
		Menu* newMenu = new VendorMenu(cursor, text, camera, shapeVAO, Renderer, playerUnit, vendor);
		MenuManager::menuManager.menus.push_back(newMenu);
		break;
	}
	case SEIZE:
		//Message
		MenuManager::menuManager.endingSubject.notify();
		ClearMenu();
		break;
	case ANIMATION:
	{
		Menu* newMenu = new AnimationOptionsMenu(cursor, text, camera, shapeVAO, Renderer);
		MenuManager::menuManager.menus.push_back(newMenu);
		break;
	}
	//Wait
	default:
		cursor->Wait();
		ResourceManager::PlaySound("select2");
		ClearMenu();
		break;
	}
}

void UnitOptionsMenu::CancelOption(int num)
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

void UnitOptionsMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	Menu::CheckInput(inputManager, deltaTime);
	MenuManager::menuManager.AnimateIndicator(deltaTime);
}

void UnitOptionsMenu::Draw()
{
	DrawMenu(true);
}

void UnitOptionsMenu::DrawMenu(bool animate)
{
	int xText = 625;
	int xIndicator = 184;
	int yOffset = 160;
	glm::vec2 fixedPosition = camera->worldToScreen(cursor->position);
	if (fixedPosition.x >= camera->screenWidth * 0.5f)
	{
		xText = 75;
		xIndicator = 8;
	}

	DrawBox(glm::ivec2(xIndicator, 48), 66, 18 + numberOfOptions * 16);

	ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	MenuManager::menuManager.DrawIndicator(glm::vec2(xIndicator, 57 + (16 * currentOption)), animate);

	//Just a little test with new line
	std::string commands = "";
	if (canSeize)
	{
		text->RenderText("Seize", xText, yOffset, 1);
		yOffset += 42;
	}
	if (canTalk)
	{
		text->RenderText("Talk", xText, yOffset, 1);
		yOffset += 42;
	}
	if (canAttack)
	{
		commands += "Attack\n";
		text->RenderText("Attack", xText, yOffset, 1);
		yOffset += 42;
	}
	if (canVisit)
	{
		text->RenderText("Visit", xText, yOffset, 1);
		yOffset += 42;
	}
	else if (canBuy)
	{
		text->RenderText("Vendor", xText, yOffset, 1);
		yOffset += 42;
	}
	if (heldFriendly)
	{
		text->RenderText("Drop", xText, yOffset, 1);
		yOffset += 42;
	}
	else if (heldEnemy)
	{
		text->RenderText("Release", xText, yOffset, 1);
		yOffset += 42;
	}
	else
	{
		if (canCapture)
		{
			text->RenderText("Capture", xText, yOffset, 1);
			yOffset += 42;
		}
		if (canRescue)
		{
			text->RenderText("Rescue", xText, yOffset, 1);
			yOffset += 42;
		}
	}
	if (canTransfer)
	{
		text->RenderText("Transfer", xText, yOffset, 1);
		yOffset += 42;
	}
	if (hasItems)
	{
		commands += "Items\n";
		text->RenderText("Items", xText, yOffset, 1);
		yOffset += 42;
	}
	if (canTrade)
	{
		text->RenderText("Trade", xText, yOffset, 1);
		yOffset += 42;
	}
	if (canDismount)
	{
		commands += "Dismount\n";
		text->RenderText("Dismount", xText, yOffset, 1);
		yOffset += 42;
	}
	else if (canMount)
	{
		commands += "Mount\n";
		text->RenderText("Mount", xText, yOffset, 1);
		yOffset += 42;
	}
	if (Settings::settings.mapAnimations == 2)
	{
		text->RenderText("Animation", xText, yOffset, 1);
		yOffset += 42;
	}
	commands += "Wait";
	text->RenderText("Wait", xText, yOffset, 1);
	//Text->RenderText(commands, xText, 100, 1);
}

void UnitOptionsMenu::GetOptions()
{
	currentOption = 0;
	hasItems = false;
	canAttack = false;
	canCapture = false;
	canDismount = false;
	canMount = false;
	heldFriendly = false;
	heldEnemy = false;
	canTransfer = false;
	canVisit = false;
	canBuy = false;
	canSeize = false;
	canRescue = false;
	optionsVector.clear();
	optionsVector.reserve(5);
	auto playerUnit = cursor->selectedUnit;
	auto playerPosition = playerUnit->sprite.getPosition();

	unitsInRange = playerUnit->inRangeUnits(1);
	unitsInCaptureRange.clear();
	//Only Leif can seize
	if (playerUnit->ID == 0 && TileManager::tileManager.getTile(playerPosition.x, playerPosition.y)->seizePoint)
	{
		canSeize = true;
		optionsVector.push_back(SEIZE);
	}
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
			dropPositions = cursor->getDropPositions(playerUnit->carriedUnit);
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
	if (playerUnit->inventory.size() > 0)
	{
		optionsVector.push_back(ITEMS);
		hasItems = true;
	}
	cursor->GetAdjacentUnits(tradeUnits, talkUnits);
	rescueUnits.clear();
	transferUnits.clear();
	if (tradeUnits.size() > 0)
	{
		canTrade = true;
		optionsVector.push_back(TRADE);
		int start = 1;
		if (hasItems)
		{
			start = 2;
		}
		transferUnits.reserve(tradeUnits.size());
		if (!playerUnit->carriedUnit)
		{
			rescueUnits.reserve(tradeUnits.size());
			for (int i = 0; i < tradeUnits.size(); i++)
			{
				auto currentUnit = tradeUnits[i];
				if (!currentUnit->carryingUnit)
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
				optionsVector.insert(optionsVector.begin() + optionsVector.size() - start, RESCUE);
			}
			if (transferUnits.size() > 0)
			{
				canTransfer = true;
				optionsVector.insert(optionsVector.begin() + optionsVector.size() - start, TRANSFER);
			}
		}
		else
		{
			for (int i = 0; i < tradeUnits.size(); i++)
			{
				auto currentUnit = tradeUnits[i];
				if (!currentUnit->carryingUnit)
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
				optionsVector.insert(optionsVector.begin() + optionsVector.size() - start, TRANSFER);
			}
		}
	}
	if (talkUnits.size() > 0)
	{
		canTalk = true;
		optionsVector.insert(optionsVector.begin(), TALK);
	}
	//if can dismount
	if (!MenuManager::menuManager.mountActionTaken)
	{
		if (playerUnit->mount)
		{
			if (playerUnit->mount->mounted && !playerUnit->carriedUnit)
			{
				canDismount = true;
				optionsVector.push_back(DISMOUNT);
			}
			else if (!playerUnit->mount->mounted)
			{
				canMount = true;
				optionsVector.push_back(MOUNT);
			}
		}
	}

	if (Settings::settings.mapAnimations == 2)
	{
		optionsVector.push_back(ANIMATION);
	}

	optionsVector.push_back(WAIT);
	numberOfOptions = optionsVector.size();
}

CantoOptionsMenu::CantoOptionsMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer) 
	: Menu(Cursor, Text, camera, shapeVAO, Renderer)
{
	numberOfOptions = 1;
}

void CantoOptionsMenu::Draw()
{
	int xText = 625;
	int xIndicator = 184;
	int yOffset = 160;
	glm::vec2 fixedPosition = camera->worldToScreen(cursor->position);
	if (fixedPosition.x >= camera->screenWidth * 0.5f)
	{
		xText = 75;
		xIndicator = 8;
	}
	DrawBox(glm::ivec2(xIndicator, 48), 66, 18 + numberOfOptions * 16);

	ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	MenuManager::menuManager.DrawIndicator(glm::vec2(xIndicator, 57 + (16 * currentOption)));

	text->RenderText("Wait", xText, yOffset, 1);
}

void CantoOptionsMenu::SelectOption()
{
	cursor->Wait();
	ResourceManager::PlaySound("select2");
	ClearMenu();
}

void CantoOptionsMenu::CancelOption(int num)
{
	cursor->UndoRemainingMove();
	Menu::CancelOption();
}

void CantoOptionsMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	Menu::CheckInput(inputManager, deltaTime);
	MenuManager::menuManager.AnimateIndicator(deltaTime);
}

ItemOptionsMenu::ItemOptionsMenu(Cursor* Cursor, TextRenderer* Text, Camera* Camera, int shapeVAO, SpriteRenderer* Renderer) : 
	Menu(Cursor, Text, Camera, shapeVAO, Renderer)
{
	GetOptions();
	itemIconUVs = MenuManager::menuManager.itemIconUVs;
	proficiencyIconUVs = MenuManager::menuManager.proficiencyIconUVs;
}

void ItemOptionsMenu::Draw()
{
	Unit* unit = cursor->selectedUnit;
	auto inventory = unit->inventory;
	DrawItemWindow(inventory, unit, true);
	if (inventory[currentOption]->isWeapon)
	{
		DrawWeaponComparison(inventory);
	}
	else
	{
		DrawBox(glm::vec2(152, 72), 98, 130);
		text->RenderText(inventory[currentOption]->description, 525, 225, 1);
	}
}

void ItemOptionsMenu::DrawItemWindow(std::vector<Item*>& inventory, Unit* unit, bool animate)
{
	DrawBox(glm::vec2(24, 72), 122, 18 + numberOfOptions * 16);
	ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	MenuManager::menuManager.DrawIndicator(glm::vec2(24, 82 + (16 * currentOption)), animate);

	//Duplicated this down in ItemUseMenu's Draw.
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
		text->RenderTextRight(intToString(item->remainingUses), 375, yPosition, 1, 28, color);

		ResourceManager::GetShader("Nsprite").Use();
		ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());
		auto texture = ResourceManager::GetTexture("icons");

		Renderer->setUVs(itemIconUVs[item->ID]);
		Renderer->DrawSprite(texture, glm::vec2(40, 82 + 16 * i), 0.0f, glm::vec2(16));
	}

	DrawPortrait(unit);
}

void ItemOptionsMenu::DrawWeaponComparison(std::vector<Item*>& inventory)
{
	DrawBox(glm::vec2(152, 72), 98, 130);

	int offSet = 42;
	int yPosition = 225;
	auto weaponData = ItemManager::itemManager.weaponData[inventory[currentOption]->ID];

	int xStatName = 525;
	int xStatValue = 575;
	text->RenderText("Type", xStatName, yPosition, 1);
	yPosition += offSet;
	text->RenderText("Atk", xStatName, yPosition, 1);
	text->RenderTextRight(intToString(selectedStats.attackDamage), xStatValue, yPosition, 1, 28);
	if (selectedStats.attackDamage > currentStats.attackDamage)
	{
		text->RenderTextRight("^^^", xStatValue + 60, yPosition, 1, 28);
	}
	yPosition += offSet;
	text->RenderText("Hit", xStatName, yPosition, 1);
	text->RenderTextRight(intToString(selectedStats.hitAccuracy), xStatValue, yPosition, 1, 28);
	if (selectedStats.hitAccuracy > currentStats.hitAccuracy)
	{
		text->RenderTextRight("^^^", xStatValue + 60, yPosition, 1, 28);
	}
	yPosition += offSet;
	text->RenderText("Crit", xStatName, yPosition, 1);
	text->RenderTextRight(intToString(selectedStats.hitCrit), xStatValue, yPosition, 1, 28);
	if (selectedStats.hitCrit > currentStats.hitCrit)
	{
		text->RenderTextRight("^^^", xStatValue + 60, yPosition, 1, 28);
	}
	yPosition += offSet;
	text->RenderText("Avo", xStatName, yPosition, 1);
	text->RenderTextRight(intToString(selectedStats.hitAvoid), xStatValue, yPosition, 1, 28);
	if (selectedStats.hitAvoid > currentStats.hitAvoid)
	{
		text->RenderTextRight("^^^", xStatValue + 60, yPosition, 1, 28);
	}

	ResourceManager::GetShader("Nsprite").Use();
	ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());
	auto texture = ResourceManager::GetTexture("icons");

	Renderer->setUVs(proficiencyIconUVs[weaponData.type]);
	Renderer->DrawSprite(texture, glm::vec2(193, 83), 0.0f, glm::vec2(16));
}

void ItemOptionsMenu::DrawPortrait(Unit* unit)
{
	Texture2D portraitTexture = ResourceManager::GetTexture("Portraits");
	ResourceManager::GetShader("Nsprite").Use();
	ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());
	Renderer->setUVs(UnitResources::portraitUVs[unit->portraitID][0]);
	Renderer->DrawSprite(portraitTexture, glm::vec2(40, 8), 0, glm::vec2(48, 64), glm::vec4(1), true);
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
	MenuManager::menuManager.AnimateIndicator(deltaTime);
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

ItemUseMenu::ItemUseMenu(Cursor* Cursor, TextRenderer* Text, Camera* Camera, int shapeVAO, SpriteRenderer* Renderer, Item* selectedItem, int index)
	: Menu(Cursor, Text, Camera, shapeVAO, Renderer)
{
	inventoryIndex = index;
	item = selectedItem;
	GetOptions();
	previous = static_cast<ItemOptionsMenu*>(MenuManager::menuManager.GetCurrent());
}

void ItemUseMenu::Draw()
{
	DrawMenu(true);
}

void ItemUseMenu::DrawMenu(bool animate)
{

	DrawBox(glm::vec2(152, 72), 50, 18 + numberOfOptions * 16);

	ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	MenuManager::menuManager.DrawIndicator(glm::vec2(152, 81 + (16 * currentOption)), animate);

	Unit* unit = cursor->selectedUnit;
	auto inventory = unit->inventory;
	previous->DrawItemWindow(inventory, unit);
	int yOffset = 225;
	if (canUse)
	{
		text->RenderText("Use", 525, yOffset, 1);
		yOffset += 42;
	}
	if (canEquip)
	{
		text->RenderText("Equip", 525, yOffset, 1);
		yOffset += 42;
	}
	glm::vec3 grey = glm::vec3(0.64);
	glm::vec3 color = glm::vec3(1);
	if (!item->canDrop)
	{
		color = grey;
	}
	text->RenderText("Drop", 525, yOffset, 1, color);
}

void ItemUseMenu::SelectOption()
{
	ResourceManager::PlaySound("select2");
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
			Menu* newMenu = new DropConfirmMenu(cursor, text, camera, shapeVAO, Renderer, inventoryIndex);
			MenuManager::menuManager.menus.push_back(newMenu);
		}
		else
		{
			//ResourceManager::PlaySound("no good"); //Play the sound when you cannot drop the item. Yep, it plays both this and the select
		}
		break;
	default:
		break;
	}
}

void ItemUseMenu::CancelOption(int num)
{
	Menu::CancelOption();
	MenuManager::menuManager.menus.back()->GetOptions();
}

void ItemUseMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	Menu::CheckInput(inputManager, deltaTime);
	MenuManager::menuManager.AnimateIndicator(deltaTime);
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

DropConfirmMenu::DropConfirmMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer, int index)
	: Menu(Cursor, Text, camera, shapeVAO, Renderer)
{
	inventoryIndex = index;
	GetOptions();
	currentOption = 1;
	previous = static_cast<ItemUseMenu*>(MenuManager::menuManager.GetCurrent());
}

void DropConfirmMenu::Draw()
{

	DrawBox(glm::vec2(160, 112), 50, 18 + numberOfOptions * 16);

	ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	MenuManager::menuManager.DrawIndicator(glm::vec2(160, 121 + (16 * currentOption)));

	text->RenderText("Yes", 550, 332, 1);
	text->RenderText("No", 550, 374, 1);

	previous->DrawMenu();
}

void DropConfirmMenu::SelectOption()
{
	if (currentOption == 0)
	{
		cursor->selectedUnit->dropItem(inventoryIndex);
	}
	//Going back to the main selection menu is how FE5 does it, not sure if I want to keep that.
	CancelOption();
	MenuManager::menuManager.menus.back()->GetOptions();
}

void DropConfirmMenu::CancelOption(int num)
{
	Menu::CancelOption(3);
}

void DropConfirmMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	Menu::CheckInput(inputManager, deltaTime);
	MenuManager::menuManager.AnimateIndicator(deltaTime);
}

void DropConfirmMenu::GetOptions()
{
	optionsVector.resize(2);
	numberOfOptions = optionsVector.size();
}

AnimationOptionsMenu::AnimationOptionsMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer)
	: Menu(Cursor, Text, camera, shapeVAO, Renderer)
{
	previous = static_cast<UnitOptionsMenu*>(MenuManager::menuManager.GetCurrent());
	GetOptions();
	yPosition = 202 + previous->optionsVector.size() * 42;
	yIndicator = 73 + previous->optionsVector.size() * 16;
}

void AnimationOptionsMenu::Draw()
{
	previous->DrawMenu();

	int xText = 625;
	int xIndicator = 184;
	int yOffset = yPosition;
	glm::vec2 fixedPosition = camera->worldToScreen(cursor->position);
	if (fixedPosition.x >= camera->screenWidth * 0.5f)
	{
		xText = 75;
		xIndicator = 8;
	}

	DrawBox(glm::vec2(xIndicator, yIndicator - 9), 50, 50);

	ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	MenuManager::menuManager.DrawIndicator(glm::vec2(xIndicator, yIndicator + (16 * currentOption)));

	text->RenderText("Normal", xText, yOffset, 1);
	text->RenderText("Map", xText, yOffset + 42, 1);
}

void AnimationOptionsMenu::SelectOption()
{
	if (currentOption == 0)
	{
		cursor->selectedUnit->battleAnimations = true;
	}
	else
	{
		cursor->selectedUnit->battleAnimations = false;
	}
	CancelOption();
}

void AnimationOptionsMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	Menu::CheckInput(inputManager, deltaTime);
	MenuManager::menuManager.AnimateIndicator(deltaTime);
}

void AnimationOptionsMenu::GetOptions()
{
	optionsVector.resize(2);
	currentOption = 1;
	if (cursor->selectedUnit->battleAnimations)
	{
		currentOption = 0;
	}
	numberOfOptions = optionsVector.size();
}

SelectWeaponMenu::SelectWeaponMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer, std::vector<Item*>& validWeapons, std::vector<std::vector<Unit*>>& units, bool capturing)
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

	DrawBox(glm::vec2(24, 72), 122, 18 + numberOfOptions * 16);

	ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	MenuManager::menuManager.DrawIndicator(glm::ivec2(24, 81 + 16 * currentOption));

	Unit* unit = cursor->selectedUnit;
	for (int i = 0; i < inventory.size(); i++)
	{
		int yPosition = 225 + i * 42;
		text->RenderText(inventory[i]->name, 175, yPosition, 1);
		text->RenderTextRight(intToString(inventory[i]->remainingUses), 375, yPosition, 1, 28);
		auto texture = ResourceManager::GetTexture("icons");

		Renderer->setUVs(itemIconUVs[inventory[i]->ID]);
		Renderer->DrawSprite(texture, glm::vec2(40, 82 + 16 * i), 0.0f, glm::vec2(16));
	}

	DrawWeaponComparison(weapons);

	DrawPortrait(unit);
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
	Menu* newMenu = new SelectEnemyMenu(cursor, text, camera, shapeVAO, Renderer, enemyUnits, selectedWeapon, capturing);
	MenuManager::menuManager.menus.push_back(newMenu);
}

void SelectWeaponMenu::GetOptions()
{
	numberOfOptions = weapons.size();
}

void SelectWeaponMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	Menu::CheckInput(inputManager, deltaTime);
	MenuManager::menuManager.AnimateIndicator(deltaTime);
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

SelectEnemyMenu::SelectEnemyMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer, std::vector<Unit*>& units, int selectedWeapon, bool capturing) :
	Menu(Cursor, Text, camera, shapeVAO, Renderer), selectedWeapon(selectedWeapon), capturing(capturing)
{
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
	int enemyStatsTextX = 550;
	int statsDisplay = 168;

	glm::vec2 fixedPosition = camera->worldToScreen(targetPosition);
	if (fixedPosition.x >= camera->screenWidth * 0.5f)
	{
		enemyStatsTextX = 50;
		statsDisplay = 8;
	}

	DrawPattern(glm::vec2(26, 122), glm::vec2(statsDisplay + 27, 67));

	Renderer->setUVs(cursor->uvs[2]);
	Texture2D displayTexture = ResourceManager::GetTexture("UIItems");
	
	Unit* unit = cursor->selectedUnit;
	ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera->getCameraMatrix());
	Renderer->DrawSprite(displayTexture, targetPosition - glm::vec2(3), 0.0f, cursor->dimensions);

	displayTexture = ResourceManager::GetTexture("EnemySelectBackground");
	Renderer->setUVs();

	ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	Renderer->DrawSprite(displayTexture, glm::vec2(statsDisplay, 8), 0.0f, glm::vec2(82, 210));
	int statsY = 53;
	text->RenderText(enemy->name, enemyStatsTextX, statsY, 1);
	statsY += 42;
	text->RenderText(enemy->name, enemyStatsTextX, statsY, 1);
	if (auto enemyWeapon = enemy->GetEquippedItem())
	{
		statsY += 42;
		text->RenderText(enemyWeapon->name, enemyStatsTextX, statsY, 1); //need error checking here
	}

	statsY = 208;
	text->RenderTextRight(intToString(enemy->level), enemyStatsTextX, statsY, 1, 28);
	text->RenderTextRight(intToString(unit->level), enemyStatsTextX + 150, statsY, 1, 28);

	statsY += 42;
	text->RenderTextRight(intToString(enemy->currentHP), enemyStatsTextX, statsY, 1, 28);
	text->RenderTextRight(intToString(unit->currentHP), enemyStatsTextX + 150, statsY, 1, 28);

	statsY += 42;
	text->RenderTextRight(enemyStats.atk, enemyStatsTextX, statsY, 1, 28);
	text->RenderTextRight(playerStats.atk, enemyStatsTextX + 150, statsY, 1, 28);

	statsY += 42;
	text->RenderTextRight(enemyStats.def, enemyStatsTextX, statsY, 1, 28);
	text->RenderTextRight(playerStats.def, enemyStatsTextX + 150, statsY, 1, 28);

	statsY += 42;
	text->RenderTextRight(enemyStats.hit, enemyStatsTextX, statsY, 1, 28);
	text->RenderTextRight(playerStats.hit, enemyStatsTextX + 150, statsY, 1, 28);

	statsY += 42;
	text->RenderTextRight(enemyStats.crit, enemyStatsTextX, statsY, 1, 28);
	text->RenderTextRight(playerStats.crit, enemyStatsTextX + 150, statsY, 1, 28);

	statsY += 42;
	text->RenderTextRight(enemyStats.attackSpeed, enemyStatsTextX, statsY, 1, 28);
	text->RenderTextRight(playerStats.attackSpeed, enemyStatsTextX + 150, statsY, 1, 28);

	text->RenderText(unit->name, enemyStatsTextX + 125, 525, 1);
}

void SelectEnemyMenu::SelectOption()
{
	ResourceManager::PlaySound("select2");
	std::cout << unitsToAttack[currentOption]->name << std::endl;
	cursor->selectedUnit->equipWeapon(selectedWeapon);
	MenuManager::menuManager.battleManager->SetUp(cursor->selectedUnit, unitsToAttack[currentOption], unitNormalStats, enemyNormalStats, attackDistance, enemyCanCounter, *camera, false, capturing);
	cursor->MoveUnitToTile();
	ClearMenu();
}

void SelectEnemyMenu::GetOptions()
{
	numberOfOptions = unitsToAttack.size();
}

void SelectEnemyMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (inputManager.KeyDownDelay(SDLK_UP))
	{
		PreviousOption();
	}
	else if (inputManager.KeyDownDelay(SDLK_DOWN))
	{
		NextOption();
	}
	else if (inputManager.KeyDownDelay(SDLK_LEFT))
	{
		PreviousOption();
	}
	else if (inputManager.KeyDownDelay(SDLK_RIGHT))
	{
		NextOption();
	}
	if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
	else if (inputManager.isKeyPressed(SDLK_z))
	{
		CancelOption();
		currentOption = 0;
	}

	if (inputManager.isKeyPressed(SDLK_UP) || inputManager.isKeyPressed(SDLK_DOWN) || inputManager.isKeyPressed(SDLK_RIGHT) || inputManager.isKeyPressed(SDLK_LEFT))
	{
		CanEnemyCounter();
	}
}

void SelectEnemyMenu::CancelOption(int num)
{
	//Obvious but stupid solution to resetting this malus after cancelling a capture action
	if (capturing)
	{
		cursor->selectedUnit->carryingMalus = 1;
	}
	Menu::CancelOption(2);
}

void SelectEnemyMenu::CanEnemyCounter(bool capturing /*= false */)
{
	auto enemy = unitsToAttack[currentOption];
	auto unit = cursor->selectedUnit;
	attackDistance = abs(enemy->sprite.getPosition().x - unit->sprite.getPosition().x) + abs(enemy->sprite.getPosition().y - unit->sprite.getPosition().y);
	attackDistance /= TileManager::TILE_SIZE;
	auto enemyWeapon = enemy->GetEquippedWeapon();
	auto unitWeapon = unit->GetWeaponData(unit->inventory[selectedWeapon]);
	enemyCanCounter = false;
	if (enemyWeapon.maxRange >= attackDistance && enemyWeapon.minRange <= attackDistance)
	{
		enemyCanCounter = true;
	}

	enemyNormalStats = enemy->CalculateBattleStats();
	unitNormalStats = unit->CalculateBattleStats(unit->inventory[selectedWeapon]->ID);

	enemy->CalculateMagicDefense(enemyWeapon, enemyNormalStats, attackDistance);
	unit->CalculateMagicDefense(unitWeapon, unitNormalStats, attackDistance);

	MenuManager::menuManager.battleManager->CalculateFinalStats(unitNormalStats, enemyNormalStats, unit, enemy, unitWeapon, enemyWeapon);

	playerStats = DisplayedBattleStats{ intToString(unit->level), intToString(unit->currentHP), intToString(unitNormalStats.attackDamage), intToString(unitNormalStats.defense), intToString(unitNormalStats.hitAccuracy), intToString(unitNormalStats.hitCrit), intToString(unitNormalStats.attackSpeed) };

	enemyStats = DisplayedBattleStats{ intToString(enemy->level), intToString(enemy->currentHP), intToString(enemyNormalStats.attackDamage), intToString(enemyNormalStats.defense), intToString(enemyNormalStats.hitAccuracy), intToString(enemyNormalStats.hitCrit), intToString(enemyNormalStats.attackSpeed) };
	if (!enemyCanCounter)
	{
		enemyStats.hit = "--";
		enemyStats.atk = "--";
		enemyStats.crit = "--";
	}
}

SelectTradeUnit::SelectTradeUnit(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer, std::vector<Unit*>& units)
	: Menu(Cursor, Text, camera, shapeVAO, Renderer)
{
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
	glm::vec2 targetPosition;
	if (tradeUnit->carryingUnit)
	{
		targetPosition = tradeUnit->carryingUnit->sprite.getPosition();
	}
	else
	{
		targetPosition = tradeUnit->sprite.getPosition();
	}
	int xText = 500;
	int xIndicator = 152;
	glm::vec2 fixedPosition = camera->worldToScreen(targetPosition);
	if (fixedPosition.x >= camera->screenWidth * 0.5f)
	{
		xText = 50;
		xIndicator = 8;
	}

	DrawBox(glm::ivec2(xIndicator, 8), 98, 10 + (inventorySize + 2) * 16);

	Renderer->setUVs(cursor->uvs[2]);
	Texture2D displayTexture = ResourceManager::GetTexture("UIItems");
	Renderer->DrawSprite(displayTexture, targetPosition - glm::vec2(3), 0.0f, cursor->dimensions);

	int textHeight = 53;
	text->RenderText(tradeUnit->name, xText, textHeight, 1);
	textHeight += 64;
	for (int i = 0; i < inventorySize; i++)
	{
		text->RenderText(tradeUnit->inventory[i]->name, xText, textHeight, 1);
		text->RenderTextRight(intToString(tradeUnit->inventory[i]->remainingUses), xText + 200, textHeight, 1, 28);
		textHeight += 42;
	}
}

void SelectTradeUnit::SelectOption()
{
	Menu* newMenu = new TradeMenu(cursor, text, camera, shapeVAO, Renderer, tradeUnits[currentOption]);
	MenuManager::menuManager.menus.push_back(newMenu);
}

void SelectTradeUnit::GetOptions()
{
	numberOfOptions = tradeUnits.size();
}

void SelectTradeUnit::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (inputManager.KeyDownDelay(SDLK_UP))
	{
		PreviousOption();
	}
	else if (inputManager.KeyDownDelay(SDLK_DOWN))
	{
		NextOption();
	}
	else if (inputManager.KeyDownDelay(SDLK_LEFT))
	{
		PreviousOption();
	}
	else if (inputManager.KeyDownDelay(SDLK_RIGHT))
	{
		NextOption();
	}
	if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
	else if (inputManager.isKeyPressed(SDLK_z))
	{
		CancelOption();
		currentOption = 0;
	}
}

SelectTalkMenu::SelectTalkMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer, std::vector<Unit*>& units)
	: Menu(Cursor, Text, camera, shapeVAO, Renderer)
{
	talkUnits = units;
	GetOptions();
}

void SelectTalkMenu::Draw()
{
	auto talkUnit = talkUnits[currentOption];

	auto targetPosition = talkUnit->sprite.getPosition();
	int xText = 550;
	int xIndicator = 168;
	int boxHeight = 98;
	glm::vec2 fixedPosition = camera->worldToScreen(targetPosition);
	if (fixedPosition.x >= camera->screenWidth * 0.5f)
	{
		xText = 50;
		xIndicator = 8;
	}
	DrawBox(glm::vec2(xIndicator, 8), 82, boxHeight);

	Renderer->setUVs(cursor->uvs[2]);
	Texture2D displayTexture = ResourceManager::GetTexture("UIItems");
	Unit* unit = cursor->selectedUnit;
	Renderer->DrawSprite(displayTexture, targetPosition - glm::vec2(3), 0.0f, cursor->dimensions);

	int textHeight = 53;
	text->RenderText(talkUnit->name, xText, textHeight, 1);
	textHeight += 42;
	text->RenderText(talkUnit->unitClass, xText, textHeight, 1);
	textHeight += 42;
	if (auto weapon = talkUnit->GetEquippedItem())
	{
		text->RenderText(weapon->name, xText, textHeight, 1);
	}
	textHeight += 42;
	text->RenderTextRight(intToString(talkUnit->level), xText + 150, textHeight, 1, 28);
	textHeight += 42;
	text->RenderText("HP", xText, textHeight, 1);
	text->RenderText(intToString(talkUnit->maxHP) + "/" + intToString(talkUnit->currentHP), xText + 75, textHeight, 1);
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
	ResourceManager::PlaySound("select2");

	ClearMenu();
}

void SelectTalkMenu::GetOptions()
{
	numberOfOptions = talkUnits.size();
}

void SelectTalkMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (inputManager.KeyDownDelay(SDLK_UP))
	{
		PreviousOption();
	}
	else if (inputManager.KeyDownDelay(SDLK_DOWN))
	{
		NextOption();
	}
	else if (inputManager.KeyDownDelay(SDLK_LEFT))
	{
		PreviousOption();
	}
	else if (inputManager.KeyDownDelay(SDLK_RIGHT))
	{
		NextOption();
	}
	if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
	else if (inputManager.isKeyPressed(SDLK_z))
	{
		CancelOption();
		currentOption = 0;
	}
}

TradeMenu::TradeMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer, Unit* unit)
	: Menu(Cursor, Text, camera, shapeVAO, Renderer)
{
	fullFadeIn = true;
	tradeUnit = unit;
	itemIconUVs = MenuManager::menuManager.itemIconUVs;
	GetOptions();
}

void TradeMenu::Draw()
{
	if ((!fullFadeIn && !exitMenu) || (exitMenu && !fullFadeOut))
	{
		DrawPattern(glm::vec2(256, 80), glm::vec2(0));
		DrawPattern(glm::vec2(256, 144), glm::vec2(0, 80));

		ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());
		Texture2D texture = ResourceManager::GetTexture("TradeMenuBG");
		Renderer->setUVs();
		Renderer->DrawSprite(texture, glm::vec2(0, 0), 0, glm::vec2(256, 224));

		int x = 0;
		if (firstInventory)
		{
			x = 7;
		}
		else
		{
			x = 135;
		}

		ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());
		MenuManager::menuManager.DrawIndicator(glm::vec2(x, 97 + (16 * currentOption)));

		if (moving)
		{
			if (moveFromFirst)
			{
				x = 7;
			}
			else
			{
				x = 135;
			}

			MenuManager::menuManager.DrawIndicator(glm::vec2(x, 97 + (16 * itemToMove)), false);
		}

		auto firstUnit = cursor->selectedUnit;
		text->RenderText(firstUnit->name, 25, 32, 1);
		text->RenderText(firstUnit->unitClass, 25, 75, 1);
		text->RenderText(tradeUnit->name, 425, 32, 1);
		text->RenderText(tradeUnit->unitClass, 425, 75, 1);
		ResourceManager::GetShader("Nsprite").Use();

		for (int i = 0; i < firstUnit->inventory.size(); i++)
		{
			text->RenderText(firstUnit->inventory[i]->name, 125, 267 + i * 42, 1);
			text->RenderTextRight(intToString(firstUnit->inventory[i]->remainingUses), 325, 267 + i * 42, 1, 28);
			ResourceManager::GetShader("Nsprite").Use();
			auto texture = ResourceManager::GetTexture("icons");

			Renderer->setUVs(itemIconUVs[firstUnit->inventory[i]->ID]);
			Renderer->DrawSprite(texture, glm::vec2(24, 98 + 16 * i), 0.0f, glm::vec2(16));
		}

		for (int i = 0; i < tradeUnit->inventory.size(); i++)
		{
			text->RenderText(tradeUnit->inventory[i]->name, 525, 267 + i * 42, 1);
			text->RenderTextRight(intToString(tradeUnit->inventory[i]->remainingUses), 725, 267 + i * 42, 1, 28);

			ResourceManager::GetShader("Nsprite").Use();
			auto texture = ResourceManager::GetTexture("icons");

			Renderer->setUVs(itemIconUVs[tradeUnit->inventory[i]->ID]);
			Renderer->DrawSprite(texture, glm::vec2(152, 98 + 16 * i), 0.0f, glm::vec2(16));
		}

		Texture2D portraitTexture = ResourceManager::GetTexture("Portraits");
		ResourceManager::GetShader("Nsprite").Use();
		ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());
		Renderer->setUVs(UnitResources::portraitUVs[firstUnit->portraitID][0]);
		Renderer->DrawSprite(portraitTexture, glm::vec2(72, 8), 0, glm::vec2(48, 64), glm::vec4(1), true);

		Renderer->setUVs(UnitResources::portraitUVs[tradeUnit->portraitID][0]);
		Renderer->DrawSprite(portraitTexture, glm::vec2(200, 8), 0, glm::vec2(48, 64));
	}
	if (fullFadeIn || fullFadeOut)
	{
		DrawFadeIn();
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
		currentOption = 0;
	}
	ResourceManager::PlaySound("select2");
}

void TradeMenu::GetOptions()
{
	if (firstInventory)
	{
		numberOfOptions = cursor->selectedUnit->inventory.size();
		if (moving && !moveFromFirst)
		{
			if (numberOfOptions < 7)
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
			if (numberOfOptions < 7)
			{
				numberOfOptions++;
			}
		}
	}
}

void TradeMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (fullFadeIn)
	{
		HandleFadeIn(deltaTime);
	}
	else if (exitMenu && fullFadeOut)
	{
		HandleFadeOut(deltaTime);
	}
	else
	{
		if (fullFadeOut)
		{
			HandleFadeOut(deltaTime);
		}
		Menu::CheckInput(inputManager, deltaTime);
		if (inputManager.isKeyPressed(SDLK_LEFT) && !firstInventory)
		{
			firstInventory = true;
			auto firstInv = cursor->selectedUnit->inventory;
			if (currentOption >= firstInv.size())
			{
				currentOption = firstInv.size() - 1;
			}
			GetOptions();
			ResourceManager::PlaySound("optionSelect2");
		}
		if (inputManager.isKeyPressed(SDLK_RIGHT) && firstInventory)
		{
			auto firstInv = cursor->selectedUnit->inventory;
			firstInventory = false;
			if (currentOption >= tradeUnit->inventory.size())
			{
				currentOption = tradeUnit->inventory.size() - 1;
			}
			GetOptions();
			ResourceManager::PlaySound("optionSelect2");
		}
		MenuManager::menuManager.AnimateIndicator(deltaTime);
	}
}

void TradeMenu::CancelOption(int num)
{
	ResourceManager::PlaySound("cancel");
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
		exitMenu = true;
		fullFadeIn = true;
	}
}

void TradeMenu::DelayedExit()
{
	MenuManager::menuManager.PreviousMenu();
	MenuManager::menuManager.PreviousMenu();
	MenuManager::menuManager.menus.back()->currentOption = 0;
	MenuManager::menuManager.menus.back()->GetOptions();
}

UnitStatsViewMenu::UnitStatsViewMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer, Unit* unit) :
	Menu(Cursor, Text, camera, shapeVAO, Renderer)
{
	this->unit = unit;
	battleStats = unit->CalculateBattleStats();
	fullScreen = true;
	if (unit->team == 0)
	{
		unitList = &MenuManager::menuManager.playerManager->units;
	}
	else if (unit->team == 1)
	{
		unitList = &MenuManager::menuManager.enemyManager->units;
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
	skillIconUVs = MenuManager::menuManager.skillIconUVs;
	carryUVs = MenuManager::menuManager.carryingIconsUVs;

	skillInfo.resize(6);
	skillInfo[0] = { "Prayer", "Dodge when low\non health" };
	skillInfo[1] = { "Continue", "Double attacks\nsomethimes" };
	skillInfo[2] = { "Wrath", "Critical on\ncounter" };
	skillInfo[3] = { "Vantage", "Always attack\nfirst" };
	skillInfo[4] = { "Accost", "Long battles\nor somethin" };
	skillInfo[5] = { "Charisma", "Bonuses for\nbuddies" };
}

void UnitStatsViewMenu::Draw()
{

	DrawPattern(glm::vec2(256, 145), glm::vec2(0, 79));

	//page 1
	if (firstPage || transition)
	{
		DrawPage1();
	}
	if (!firstPage || transition)
	{
		DrawPage2();
	}

	DrawUpperSection();
}

void UnitStatsViewMenu::DrawPage1()
{
	float adjustedOffset = ((224 - yOffset) / 224.0f) * 600;

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
		text->RenderTextRight(intToString(item->remainingUses) + "/", 625, yPosition, 1, 28, color);
		text->RenderTextRight(intToString(item->maxUses), 700, yPosition, 1, 28, color);
		if (i == unit->equippedWeapon)
		{
			text->RenderText("E", 750, yPosition, 1);
		}
		ResourceManager::GetShader("Nsprite").Use();
		ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());
		auto texture = ResourceManager::GetTexture("icons");

		Renderer->setUVs(itemIconUVs[item->ID]);
		Renderer->DrawSprite(texture, glm::vec2(120, (95 + 16 * i) - (224 - yOffset)), 0.0f, glm::vec2(16));
	}

	int str = unit->getStrength();
	int mag = unit->getMagic();
	int skl = unit->getSkill();
	int spd = unit->getSpeed();
	int lck = unit->getLuck();
	int def = unit->getDefense();
	int bld = unit->getBuild();

	Renderer->shader = ResourceManager::GetShader("slice");

	ResourceManager::GetShader("slice").Use();
	ResourceManager::GetShader("slice").SetMatrix4("projection", camera->getOrthoMatrix());

	auto texture = ResourceManager::GetTexture("UIItems");

	glm::vec4 uvs = MenuManager::menuManager.statBarUV;
	glm::vec2 size;
	float borderSize = 1.0f;
	Renderer->setUVs();
	int x = 26;
	int y = 103;
	if (str > 0)
	{
		size = glm::vec2(4 * str - 1, 7);
		size.x = std::min(int(size.x), 78);
		ResourceManager::GetShader("slice").SetVector2f("u_dimensions", borderSize / size.x, borderSize / size.y);
		ResourceManager::GetShader("slice").SetVector2f("u_border", borderSize / 7.0f, borderSize / 7.0f);
		ResourceManager::GetShader("slice").SetVector4f("bounds", uvs.x, uvs.y, uvs.z, uvs.w);
		Renderer->DrawSprite(texture, glm::vec2(x, y - (224 - yOffset)), 0.0f, size);
	}
	y += 16;

	if (mag > 0)
	{
		size = glm::vec2(4 * mag - 1, 7);
		size.x = std::min(int(size.x), 78);
		ResourceManager::GetShader("slice").SetVector2f("u_dimensions", borderSize / size.x, borderSize / size.y);
		ResourceManager::GetShader("slice").SetVector2f("u_border", borderSize / 7.0f, borderSize / 7.0f);
		ResourceManager::GetShader("slice").SetVector4f("bounds", uvs.x, uvs.y, uvs.z, uvs.w);
		Renderer->DrawSprite(texture, glm::vec2(x, y - (224 - yOffset)), 0.0f, size);
	}
	y += 16;

	if (skl > 0)
	{
		size = glm::vec2(4 * skl - 1, 7);
		size.x = std::min(int(size.x), 78);
		ResourceManager::GetShader("slice").SetVector2f("u_dimensions", borderSize / size.x, borderSize / size.y);
		ResourceManager::GetShader("slice").SetVector2f("u_border", borderSize / 7.0f, borderSize / 7.0f);
		ResourceManager::GetShader("slice").SetVector4f("bounds", uvs.x, uvs.y, uvs.z, uvs.w);
		Renderer->DrawSprite(texture, glm::vec2(x, y - (224 - yOffset)), 0.0f, size);
	}
	y += 16;

	if (spd > 0)
	{
		size = glm::vec2(4 * spd - 1, 7);
		size.x = std::min(int(size.x), 78);
		ResourceManager::GetShader("slice").SetVector2f("u_dimensions", borderSize / size.x, borderSize / size.y);
		ResourceManager::GetShader("slice").SetVector2f("u_border", borderSize / 7.0f, borderSize / 7.0f);
		ResourceManager::GetShader("slice").SetVector4f("bounds", uvs.x, uvs.y, uvs.z, uvs.w);
		Renderer->DrawSprite(texture, glm::vec2(x, y - (224 - yOffset)), 0.0f, size);
	}
	y += 16;

	if (lck > 0)
	{
		size = glm::vec2(4 * lck - 1, 7);
		size.x = std::min(int(size.x), 78);
		ResourceManager::GetShader("slice").SetVector2f("u_dimensions", borderSize / size.x, borderSize / size.y);
		ResourceManager::GetShader("slice").SetVector2f("u_border", borderSize / 7.0f, borderSize / 7.0f);
		ResourceManager::GetShader("slice").SetVector4f("bounds", uvs.x, uvs.y, uvs.z, uvs.w);
		Renderer->DrawSprite(texture, glm::vec2(x, y - (224 - yOffset)), 0.0f, size);
	}
	y += 16;

	if (def > 0)
	{
		size = glm::vec2(4 * def - 1, 7);
		size.x = std::min(int(size.x), 78);
		ResourceManager::GetShader("slice").SetVector2f("u_dimensions", borderSize / size.x, borderSize / size.y);
		ResourceManager::GetShader("slice").SetVector2f("u_border", borderSize / 7.0f, borderSize / 7.0f);
		ResourceManager::GetShader("slice").SetVector4f("bounds", uvs.x, uvs.y, uvs.z, uvs.w);
		Renderer->DrawSprite(texture, glm::vec2(x, y - (224 - yOffset)), 0.0f, size);
	}
	y += 16;

	if (bld > 0)
	{
		size = glm::vec2(4 * bld - 1, 7);
		size.x = std::min(int(size.x), 78);
		ResourceManager::GetShader("slice").SetVector2f("u_dimensions", borderSize / size.x, borderSize / size.y);
		ResourceManager::GetShader("slice").SetVector2f("u_border", borderSize / 7.0f, borderSize / 7.0f);
		ResourceManager::GetShader("slice").SetVector4f("bounds", uvs.x, uvs.y, uvs.z, uvs.w);
		Renderer->DrawSprite(texture, glm::vec2(x, y - (224 - yOffset)), 0.0f, size);
	}

	Renderer->shader = ResourceManager::GetShader("Nsprite");

	ResourceManager::GetShader("Nsprite").Use();
	ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());
	texture = ResourceManager::GetTexture("page1lower");
	Renderer->setUVs();
	Renderer->DrawSprite(texture, glm::vec2(0, 79 - (224 - yOffset)), 0, glm::vec2(256, 145));


	if (unit->carryingMalus > 1)
	{
		texture = ResourceManager::GetTexture("UIItems");

		Renderer->setUVs(MenuManager::menuManager.malusArrowUV);
		Renderer->DrawSprite(texture, glm::vec2(56, 98 - (224 - yOffset)), 0, glm::vec2(8, 10));
		Renderer->DrawSprite(texture, glm::vec2(56, 114 - (224 - yOffset)), 0, glm::vec2(8, 10));
		Renderer->DrawSprite(texture, glm::vec2(56, 130 - (224 - yOffset)), 0, glm::vec2(8, 10));
		Renderer->DrawSprite(texture, glm::vec2(56, 146 - (224 - yOffset)), 0, glm::vec2(8, 10));
		Renderer->DrawSprite(texture, glm::vec2(56, 178 - (224 - yOffset)), 0, glm::vec2(8, 10));
	}

	x = 200;
	//Going to need an indication of what stats are affected by modifiers
	text->RenderTextRight(intToString(str), x, 270 - adjustedOffset, 1, 28, glm::vec3(0.78f, 0.92f, 1.0f));
	text->RenderTextRight(intToString(mag), x, 313 - adjustedOffset, 1, 28, glm::vec3(0.78f, 0.92f, 1.0f));
	text->RenderTextRight(intToString(skl), x, 356 - adjustedOffset, 1, 28, glm::vec3(0.78f, 0.92f, 1.0f));
	text->RenderTextRight(intToString(spd), x, 399 - adjustedOffset, 1, 28, glm::vec3(0.78f, 0.92f, 1.0f));
	text->RenderTextRight(intToString(lck), x, 441 - adjustedOffset, 1, 28, glm::vec3(0.78f, 0.92f, 1.0f));
	text->RenderTextRight(intToString(def), x, 484 - adjustedOffset, 1, 28, glm::vec3(0.78f, 0.92f, 1.0f));
	text->RenderTextRight(intToString(bld), x, 527 - adjustedOffset, 1, 28, glm::vec3(0.78f, 0.92f, 1.0f));
	if (!examining)
	{
		if (!transition)
		{
			MenuManager::menuManager.DrawArrow(glm::ivec2(124, 217));
		}
	}
	else
	{
		MenuManager::menuManager.DrawIndicator(glm::ivec2(103, 97 + 16 * currentOption));

		DrawPattern(glm::vec2(96, 144), glm::vec2(8, 79), true);

		Renderer->shader = ResourceManager::GetShader("slice");
		ResourceManager::GetShader("slice").Use();
		ResourceManager::GetShader("slice").SetMatrix4("projection", camera->getOrthoMatrix());
		auto uiTexture = ResourceManager::GetTexture("UIStuff");

		glm::vec4 uvs = MenuManager::menuManager.boxesUVs[2];
		glm::vec2 size = glm::vec2(96, 144);
		float borderSize = 6.0f;
		ResourceManager::GetShader("slice").SetVector2f("u_dimensions", borderSize / size.x, borderSize / size.y);
		ResourceManager::GetShader("slice").SetVector2f("u_border", borderSize / 32.0f, borderSize / 32.0f);
		ResourceManager::GetShader("slice").SetVector4f("bounds", uvs.x, uvs.y, uvs.z, uvs.w);
		Renderer->setUVs();
		Renderer->DrawSprite(uiTexture, glm::vec2(8, 79), 0.0f, size);
		Renderer->shader = ResourceManager::GetShader("Nsprite");
		
		int yPosition = 243;
		if (inventory[currentOption]->isWeapon)
		{
			auto weaponData = ItemManager::itemManager.weaponData[inventory[currentOption]->ID];

			ResourceManager::GetShader("Nsprite").Use();
			ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());
			auto iconsTexture = ResourceManager::GetTexture("icons");
			Renderer->setUVs(proficiencyIconUVs[weaponData.type]);
			Renderer->DrawSprite(iconsTexture, glm::vec2(57, 91), 0.0f, glm::vec2(16));
			auto& profMap = MenuManager::menuManager.profcienciesMap;
			int offSet = 42;

			int xStatName = 100;
			int xStatValue = 200;
			text->RenderText("Type", xStatName, yPosition, 1);
			if (weaponData.rank <= 5)
			{
				text->RenderText(profMap[weaponData.rank], 225, 246, 1);
			}
			else
			{
				text->RenderText("*", 225, 246, 1);
			}
			yPosition += offSet;
			text->RenderText("Hit", xStatName, yPosition, 1);
			text->RenderTextRight(intToString(weaponData.hit), xStatValue, yPosition, 1, 28);
			yPosition += offSet;
			text->RenderText("Might", xStatName, yPosition, 1);
			text->RenderTextRight(intToString(weaponData.might), xStatValue, yPosition, 1, 28);
			yPosition += offSet;
			text->RenderText("Crit", xStatName, yPosition, 1);
			text->RenderTextRight(intToString(weaponData.crit), xStatValue, yPosition, 1, 28);
			yPosition += offSet;
			text->RenderText("Range", xStatName, yPosition, 1);
			if (weaponData.maxRange == weaponData.minRange)
			{
				text->RenderTextRight(intToString(weaponData.minRange), xStatValue, yPosition, 1, 28); //need to display max as well if different
			}
			else
			{
				text->RenderTextRight(intToString(weaponData.minRange) + "~" + intToString(weaponData.maxRange), xStatValue, yPosition, 1, 30);

			}
			yPosition += offSet;
			text->RenderText("Weight", xStatName, yPosition, 1);
			text->RenderTextRight(intToString(weaponData.weight), xStatValue, yPosition, 1, 28);
			yPosition += offSet;
		}
		text->RenderText(inventory[currentOption]->description, 50, yPosition, 1);
	}
}

void UnitStatsViewMenu::DrawPage2()
{
	ResourceManager::GetShader("Nsprite").Use();
	ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());
	auto pageTexture = ResourceManager::GetTexture("page2lower");
	Renderer->setUVs();
	Renderer->DrawSprite(pageTexture, glm::vec2(0, 79 + yOffset), 0, glm::vec2(256, 145));

	auto iconsTexture = ResourceManager::GetTexture("icons");
	for (int i = 0; i < unit->skills.size(); i++)
	{
		Renderer->setUVs(skillIconUVs[unit->skills[i]]);
		Renderer->DrawSprite(iconsTexture, glm::vec2(120 + 16 * i, 200 + yOffset), 0.0f, glm::vec2(16));
	}
	if (examining && !transition)
	{
		auto skillHighlightTexture = ResourceManager::GetTexture("UIItems");
		Renderer->setUVs(MenuManager::menuManager.skillHighlightUVs[skillAnimateFrame]);
		Renderer->DrawSprite(skillHighlightTexture, glm::vec2(120 + 16 * currentOption, 200), 0, glm::vec2(16));

		ResourceManager::GetShader("Nsprite").Use();
		MenuManager::menuManager.DrawIndicator(glm::ivec2(121, 188 + 16 * currentOption), true, 1.5708f);
	}
	else if (!transition)
	{
		MenuManager::menuManager.DrawArrow(glm::ivec2(124, 80), false);
	}

	auto& profMap = MenuManager::menuManager.profcienciesMap;

	int adjustedOffset = (yOffset / 224.0f) * 600;
	if (unit->mount)
	{
		if (unit->mount->mounted)
		{
			if (unit->weaponProficiencies[WeaponData::TYPE_SWORD] > 0)
			{
				text->RenderText("-" + profMap[unit->weaponProficiencies[WeaponData::TYPE_SWORD]], 500, 291 + adjustedOffset, 1, glm::vec3(0.64f));
			}
			text->RenderText(profMap[unit->mount->weaponProficiencies[WeaponData::TYPE_LANCE]], 500, 333 + adjustedOffset, 1);
			text->RenderText(profMap[unit->mount->weaponProficiencies[WeaponData::TYPE_AXE]], 500, 375 + adjustedOffset, 1);
		}
		else
		{
			text->RenderText(profMap[unit->weaponProficiencies[WeaponData::TYPE_SWORD]], 500, 291 + adjustedOffset, 1);

			text->RenderText("-" + profMap[unit->mount->weaponProficiencies[WeaponData::TYPE_LANCE]], 500, 333 + adjustedOffset, 1, glm::vec3(0.64f));
			text->RenderText(profMap[unit->mount->weaponProficiencies[WeaponData::TYPE_AXE]], 500, 375 + adjustedOffset, 1);
		}
	}
	else
	{
		text->RenderText(profMap[unit->weaponProficiencies[WeaponData::TYPE_SWORD]], 500, 291 + adjustedOffset, 1);
		text->RenderText(profMap[unit->weaponProficiencies[WeaponData::TYPE_LANCE]], 500, 333 + adjustedOffset, 1);
		text->RenderText(profMap[unit->weaponProficiencies[WeaponData::TYPE_AXE]], 500, 375 + adjustedOffset, 1);
	}
	text->RenderText(profMap[unit->weaponProficiencies[WeaponData::TYPE_BOW]], 500, 417 + adjustedOffset, 1);
	text->RenderText(profMap[unit->weaponProficiencies[WeaponData::TYPE_STAFF]], 500, 459 + adjustedOffset, 1);

	text->RenderText(profMap[unit->weaponProficiencies[WeaponData::TYPE_FIRE]], 700, 291 + adjustedOffset, 1);
	text->RenderText(profMap[unit->weaponProficiencies[WeaponData::TYPE_THUNDER]], 700, 333 + adjustedOffset, 1);
	text->RenderText(profMap[unit->weaponProficiencies[WeaponData::TYPE_WIND]], 700, 375 + adjustedOffset, 1);
	text->RenderText(profMap[unit->weaponProficiencies[WeaponData::TYPE_LIGHT]], 700, 417 + adjustedOffset, 1);
	text->RenderText(profMap[unit->weaponProficiencies[WeaponData::TYPE_DARK]], 700, 459 + adjustedOffset, 1);

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
		Texture2D carryTexture = ResourceManager::GetTexture("UIItems");

		ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());
		Renderer->setUVs(carryUVs[unit->carriedUnit->team]);
		Renderer->DrawSprite(carryTexture, glm::vec2(39, 159 + yOffset), 0, glm::vec2(8));

		if (unit->buildMalus > 1)
		{
			Renderer->setUVs(MenuManager::menuManager.malusArrowUV);
			Renderer->DrawSprite(carryTexture, glm::vec2(32, 171 + yOffset), 0, glm::vec2(8, 10));
		}

		text->RenderText(unit->carriedUnit->name, 150, 420 + adjustedOffset, 1);
	}
	else if (unit->carryingUnit)
	{
		text->RenderText(unit->carryingUnit->name, 150, 420 + adjustedOffset, 1);
	}
	else
	{
		text->RenderText("----", 153, 420 + adjustedOffset, 1);
	}

	text->RenderText(intToString(unit->getMove()), 200, 466 + adjustedOffset, 1);
	//Fatigue
	if (unit->fatigue >= 0)
	{
		text->RenderText(intToString(unit->fatigue), 200, 508 + adjustedOffset, 1);
	}
	else
	{
		text->RenderText("--", 171, 516 + adjustedOffset, 1);
	}
	//Status
	text->RenderText("----", 153, 549 + adjustedOffset, 1);

	if (examining)
	{
		DrawPattern(glm::vec2(96, 104), glm::vec2(16, 112), true);

		Renderer->shader = ResourceManager::GetShader("slice");
		ResourceManager::GetShader("slice").Use();
		ResourceManager::GetShader("slice").SetMatrix4("projection", camera->getOrthoMatrix());
		auto uiTexture = ResourceManager::GetTexture("UIStuff");

		glm::vec4 uvs = MenuManager::menuManager.boxesUVs[2];
		glm::vec2 size = glm::vec2(96, 104);
		float borderSize = 6.0f;
		ResourceManager::GetShader("slice").SetVector2f("u_dimensions", borderSize / size.x, borderSize / size.y);
		ResourceManager::GetShader("slice").SetVector2f("u_border", borderSize / 32.0f, borderSize / 32.0f);
		ResourceManager::GetShader("slice").SetVector4f("bounds", uvs.x, uvs.y, uvs.z, uvs.w);
		Renderer->setUVs();
		Renderer->DrawSprite(uiTexture, glm::vec2(16, 112), 0.0f, size);
		Renderer->shader = ResourceManager::GetShader("Nsprite");

		Renderer->setUVs(skillIconUVs[unit->skills[currentOption]]);
		Renderer->DrawSprite(iconsTexture, glm::vec2(24, 122), 0.0f, glm::vec2(16));

		text->RenderText(skillInfo[unit->skills[currentOption]].name, 125, 332, 1);

		text->RenderText(skillInfo[unit->skills[currentOption]].description, 75, 393, 1);

		text->RenderText("Personal Skill", 75, 525, 1, glm::vec3(0.8f, 1.0f, 0.8f));
	}
}

void UnitStatsViewMenu::DrawUpperSection()
{
	Renderer->shader = ResourceManager::GetShader("slice");
	ResourceManager::GetShader("slice").Use();
	ResourceManager::GetShader("slice").SetMatrix4("projection", camera->getOrthoMatrix());
	auto texture = ResourceManager::GetTexture("UIStuff");

	glm::vec4 uvs = MenuManager::menuManager.boxesUVs[1];
	glm::vec2 size = glm::vec2(256, 448);
	float borderSize = 6.0f;
	ResourceManager::GetShader("slice").SetVector2f("u_dimensions", borderSize / size.x, borderSize / size.y);
	ResourceManager::GetShader("slice").SetVector2f("u_border", borderSize / 32.0f, borderSize / 32.0f);
	ResourceManager::GetShader("slice").SetVector4f("bounds", uvs.x, uvs.y, uvs.z, uvs.w);
	Renderer->setUVs();
	Renderer->DrawSprite(texture, glm::vec2(0, 0 - (224 - yOffset)), 0.0f, size);
	Renderer->shader = ResourceManager::GetShader("Nsprite");

	DrawPattern(glm::vec2(256, 79), glm::vec2(0));

	ResourceManager::GetShader("Nsprite").Use();
	ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());
	texture = ResourceManager::GetTexture("unitViewUpper");
	Renderer->setUVs();
	Renderer->DrawSprite(texture, glm::vec2(152, 31), 0, glm::vec2(104, 40));

	Renderer->shader = ResourceManager::GetShader("slice");
	texture = ResourceManager::GetTexture("UIStuff");
	size = glm::vec2(256, 80);
	ResourceManager::GetShader("slice").Use().SetVector2f("u_dimensions", borderSize / size.x, borderSize / size.y);
	ResourceManager::GetShader("slice").SetVector2f("u_border", borderSize / 32.0f, borderSize / 32.0f);
	ResourceManager::GetShader("slice").SetVector4f("bounds", uvs.x, uvs.y, uvs.z, uvs.w);
	Renderer->DrawSprite(texture, glm::vec2(0, -1), 0.0f, size);

	Renderer->shader = ResourceManager::GetShader("Nsprite");

	Texture2D portraitTexture = ResourceManager::GetTexture("Portraits");
	ResourceManager::GetShader("Nsprite").Use();
	ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());
	Renderer->setUVs(UnitResources::portraitUVs[unit->portraitID][0]);
	Renderer->DrawSprite(portraitTexture, glm::vec2(96, 8), 0, glm::vec2(48, 64), glm::vec4(1), true);

	if (unit->carryingUnit)
	{
		Texture2D carryTexture = ResourceManager::GetTexture("UIItems");

		Renderer->setUVs(carryUVs[unit->team]);
		Renderer->DrawSprite(carryTexture, glm::vec2(23, 14), 0, glm::vec2(8));
	}
	else
	{
		//Would like to not use batch here it's just easier without writing a new function/shader
		ResourceManager::GetShader("sprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());
		SBatch Batch;
		Batch.init();
		Batch.begin();
		unit->Draw(&Batch, glm::vec2(16, 10), true);
		Batch.end();
		Batch.renderBatch();
	}

	text->RenderText(unit->name, 100, 29, 1);
	text->RenderText(unit->unitClass, 50, 72, 1);
	text->RenderText("Lv", 50, 120, 1);
	text->RenderText("HP", 50, 163, 1);

	text->RenderTextRight(intToString(unit->level), 125, 120, 1, 28);
	text->RenderTextRight(intToString(unit->currentHP), 125, 163, 1, 28);

	text->RenderText("E", 175, 120, 1, glm::vec3(0.69f, 0.62f, 0.49f));
	text->RenderText("/", 175, 163, 1, glm::vec3(0.69f, 0.62f, 0.49f));

	text->RenderTextRight(intToString(unit->experience), 200, 120, 1, 28);
	text->RenderTextRight(intToString(unit->maxHP), 200, 163, 1, 28);

	int leftX = 575;
	if (unit->equippedWeapon >= 0)
	{
		text->RenderTextRight(intToString(battleStats.attackDamage), leftX, 77, 1, 28);
		text->RenderTextRight(intToString(battleStats.hitAccuracy), leftX, 120, 1, 28);
		auto weapon = unit->GetEquippedWeapon();
		if (weapon.maxRange == weapon.minRange)
		{
			text->RenderTextRight(intToString(weapon.maxRange), leftX, 163, 1, 28);
		}
		else
		{
			text->RenderTextRight(intToString(weapon.minRange) + " ~ " + intToString(weapon.maxRange), leftX, 163, 1, 30);
		}
		ResourceManager::GetShader("Nsprite").Use();
		auto texture = ResourceManager::GetTexture("icons");

		Renderer->setUVs(proficiencyIconUVs[unit->GetEquippedWeapon().type]);
		Renderer->DrawSprite(texture, glm::vec2(233, 59), 0.0f, glm::vec2(16));
	}
	else
	{
		text->RenderText("--", leftX, 77, 1);
		text->RenderText("--", leftX, 120, 1);
		text->RenderText("--", leftX, 163, 1);

	}
	int rightX = 725;
	text->RenderTextRight(intToString(battleStats.hitCrit), rightX, 77, 1, 28);
	text->RenderTextRight(intToString(battleStats.hitAvoid), rightX, 120, 1, 28);
}

void UnitStatsViewMenu::SelectOption()
{
}

void UnitStatsViewMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	MenuManager::menuManager.AnimateArrow(deltaTime);
	MenuManager::menuManager.AnimateIndicator(deltaTime);
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
					ResourceManager::PlaySound("pagechange");
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
					ResourceManager::PlaySound("pagechange");
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
				ResourceManager::PlaySound("optionSelect2");
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
				ResourceManager::PlaySound("optionSelect2");
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
						skillAniamteTimer = 0;
						skillAnimateFrame = 0;
						examining = true;
						currentOption = 0;
						numberOfOptions = unit->skills.size();
					}
				}
				if (examining)
				{
					ResourceManager::PlaySound("select1");
				}
			}
			else if (inputManager.isKeyPressed(SDLK_z))
			{
				CancelOption();
			}
		}
		else
		{
			if (firstPage)
			{
				if (inputManager.isKeyPressed(SDLK_UP))
				{
					PreviousOption();
				}
				else if (inputManager.isKeyPressed(SDLK_DOWN))
				{
					NextOption();
				}
			}
			else
			{
				skillAniamteTimer += deltaTime;
				//4 frame animation
				if (skillAniamteTimer >= 0.0666f)
				{
					skillAniamteTimer = 0;
					skillAnimateFrame++;
					if (skillAnimateFrame >= 4)
					{
						skillAnimateFrame = 0;
					}
				}
				if (inputManager.isKeyPressed(SDLK_LEFT))
				{
					PreviousOption();
				}
				else if (inputManager.isKeyPressed(SDLK_RIGHT))
				{
					NextOption();
				}
			}
			if (inputManager.isKeyPressed(SDLK_z))
			{
				ResourceManager::PlaySound("cancel");
				examining = false;
			}
		}
	}
}

void UnitStatsViewMenu::CancelOption(int num)
{
	cursor->SetFocus((*unitList)[unitIndex]);
	camera->SetMove(cursor->position);
	Menu::CancelOption();
}

ExtraMenu::ExtraMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer)
	: Menu(Cursor, Text, camera, shapeVAO, Renderer)
{
	numberOfOptions = 5;
}

void ExtraMenu::Draw()
{
	int xText = 625;
	int xIndicator = 184;
	int yOffset = 117;
	glm::vec2 fixedPosition = camera->worldToScreen(cursor->position);
	if (fixedPosition.x >= camera->screenWidth * 0.5f)
	{
		xText = 75;
		xIndicator = 8;
	}

	DrawBox(glm::ivec2(xIndicator, 32), 58, 18 + numberOfOptions * 16);

	ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	MenuManager::menuManager.DrawIndicator(glm::vec2(xIndicator, 41 + (16 * currentOption)));
	
	text->RenderText("Unit", xText, 117, 1);
	text->RenderText("Status", xText, 160, 1);
	text->RenderText("Options", xText, 203, 1);
	text->RenderText("Suspend", xText, 246, 1);
	text->RenderText("End", xText, 289, 1);
}

void ExtraMenu::SelectOption()
{
	switch (currentOption)
	{
	case UNIT:
	{
		Menu* newMenu = new UnitListMenu(cursor, text, camera, shapeVAO, Renderer);
		MenuManager::menuManager.menus.push_back(newMenu);
		break;
	}
	case STATUS:
	{
		Menu* newMenu = new StatusMenu(cursor, text, camera, shapeVAO, Renderer);
		MenuManager::menuManager.menus.push_back(newMenu);
		break;
	}
	case OPTIONS:
	{
		Menu* newMenu = new OptionsMenu(cursor, text, camera, shapeVAO, Renderer);
		MenuManager::menuManager.menus.push_back(newMenu);
		break;
	}
	case SUSPEND:
	{
		Menu* newMenu = new SuspendMenu(cursor, text, camera, shapeVAO, Renderer);
		MenuManager::menuManager.menus.push_back(newMenu);
		break;
	}
	case END:
		MenuManager::menuManager.subject.notify(0);
		ResourceManager::PlaySound("select2");
		ClearMenu();
		break;
	}
}

void ExtraMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	Menu::CheckInput(inputManager, deltaTime);
	MenuManager::menuManager.AnimateIndicator(deltaTime);
}

SelectRescueUnit::SelectRescueUnit(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer, std::vector<Unit*>& units)
	: Menu(Cursor, Text, camera, shapeVAO, Renderer)
{
	rescueUnits = units;
	GetOptions();
}

void SelectRescueUnit::Draw()
{
	auto rescueUnit = rescueUnits[currentOption];
	int boxHeight = 98;

	auto targetPosition = rescueUnit->sprite.getPosition();
	int xText = 550;
	int xIndicator = 168;
	glm::vec2 fixedPosition = camera->worldToScreen(targetPosition);
	if (fixedPosition.x >= camera->screenWidth * 0.5f)
	{
		xText = 50;
		xIndicator = 8;
	}

	DrawBox(glm::vec2(xIndicator, 8), 82, boxHeight);

	Renderer->setUVs(cursor->uvs[2]);
	Texture2D displayTexture = ResourceManager::GetTexture("UIItems");
	Unit* unit = cursor->selectedUnit;
	Renderer->DrawSprite(displayTexture, targetPosition - glm::vec2(3), 0.0f, cursor->dimensions);

	int textHeight = 53;
	text->RenderText(rescueUnit->name, xText, textHeight, 1);
	textHeight += 42;
	text->RenderText(rescueUnit->unitClass, xText, textHeight, 1);
	textHeight += 42;
	if (auto weapon = rescueUnit->GetEquippedItem())
	{
		text->RenderText(weapon->name, xText, textHeight, 1);
	}
	textHeight += 42;
	text->RenderTextRight(intToString(rescueUnit->level), xText + 150, textHeight, 1, 28);
	textHeight += 42;
	text->RenderText("HP", xText, textHeight, 1);
	text->RenderText(intToString(rescueUnit->maxHP) + "/" + intToString(rescueUnit->currentHP), xText + 75, textHeight, 1);
}

void SelectRescueUnit::SelectOption()
{
	auto rescuedUnit = rescueUnits[currentOption];
	auto playerUnit = cursor->selectedUnit;
	TileManager::tileManager.removeUnit(rescuedUnit->sprite.getPosition().x, rescuedUnit->sprite.getPosition().y);
	Menu* newMenu = new UnitMovement(cursor, text, camera, shapeVAO, Renderer, rescuedUnit, playerUnit, 0);
	MenuManager::menuManager.menus.push_back(newMenu);
	//playerUnit->carryUnit(rescuedUnit);
	//rescuedUnit->hasMoved = false;
}

void SelectRescueUnit::GetOptions()
{
	numberOfOptions = rescueUnits.size();
}

void SelectRescueUnit::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (inputManager.KeyDownDelay(SDLK_UP))
	{
		PreviousOption();
	}
	else if (inputManager.KeyDownDelay(SDLK_DOWN))
	{
		NextOption();
	}
	else if (inputManager.KeyDownDelay(SDLK_LEFT))
	{
		PreviousOption();
	}
	else if (inputManager.KeyDownDelay(SDLK_RIGHT))
	{
		NextOption();
	}
	if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
	else if (inputManager.isKeyPressed(SDLK_z))
	{
		CancelOption();
		currentOption = 0;
	}
}

DropMenu::DropMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer, std::vector<glm::ivec2>& positions) :
	Menu(Cursor, Text, camera, shapeVAO, Renderer), positions(positions)
{
	GetOptions();
}

void DropMenu::Draw()
{
	auto targetPosition = positions[currentOption];
	Renderer->setUVs(cursor->uvs[2]);
	Texture2D displayTexture = ResourceManager::GetTexture("UIItems");
	Unit* unit = cursor->selectedUnit;
	Renderer->DrawSprite(displayTexture, targetPosition - glm::ivec2(3), 0.0f, cursor->dimensions);
}

void DropMenu::SelectOption()
{
	auto playerUnit = cursor->selectedUnit;
	auto heldUnit = playerUnit->carriedUnit;

	Menu* newMenu = new UnitMovement(cursor, text, camera, shapeVAO, Renderer, heldUnit, nullptr, 2, positions[currentOption]);
	MenuManager::menuManager.menus.push_back(newMenu);
}

void DropMenu::GetOptions()
{
	numberOfOptions = positions.size();
}

void DropMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (inputManager.KeyDownDelay(SDLK_UP))
	{
		PreviousOption();
	}
	else if (inputManager.KeyDownDelay(SDLK_DOWN))
	{
		NextOption();
	}
	else if (inputManager.KeyDownDelay(SDLK_LEFT))
	{
		PreviousOption();
	}
	else if (inputManager.KeyDownDelay(SDLK_RIGHT))
	{
		NextOption();
	}
	if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
	else if (inputManager.isKeyPressed(SDLK_z))
	{
		CancelOption();
		currentOption = 0;
	}
}

SelectTransferUnit::SelectTransferUnit(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer, std::vector<Unit*>& units)
	: Menu(Cursor, Text, camera, shapeVAO, Renderer)
{
	transferUnits = units;
	GetOptions();
}

void SelectTransferUnit::Draw()
{
	auto transferUnit = transferUnits[currentOption];
	int boxHeight = 98;

	auto targetPosition = transferUnit->sprite.getPosition();

	int xText = 550;
	int xIndicator = 168;
	glm::vec2 fixedPosition = camera->worldToScreen(targetPosition);
	if (fixedPosition.x >= camera->screenWidth * 0.5f)
	{
		xText = 50;
		xIndicator = 8;
	}

	DrawBox(glm::vec2(xIndicator, 8), 82, boxHeight);

	Renderer->setUVs(cursor->uvs[2]);
	Texture2D displayTexture = ResourceManager::GetTexture("UIItems");
	Unit* unit = cursor->selectedUnit;
	Renderer->DrawSprite(displayTexture, targetPosition - glm::vec2(3), 0.0f, cursor->dimensions);

	int textHeight = 53;
	text->RenderText(transferUnit->name, xText, textHeight, 1);
	textHeight += 42;
	text->RenderText(transferUnit->unitClass, xText, textHeight, 1);
	textHeight += 42;
	if (auto weapon = transferUnit->GetEquippedItem())
	{
		text->RenderText(weapon->name, xText, textHeight, 1);
	}
	textHeight += 42;
	text->RenderTextRight(intToString(transferUnit->level), xText + 150, textHeight, 1, 28);
	textHeight += 42;
	text->RenderText("HP", xText, textHeight, 1);
	text->RenderText(intToString(transferUnit->maxHP) + "/" + intToString(transferUnit->currentHP), xText + 75, textHeight, 1);
}

void SelectTransferUnit::SelectOption()
{
	auto transferUnit = transferUnits[currentOption];
	auto playerUnit = cursor->selectedUnit;
	Unit* receivingUnit;
	Unit* transferedUnit;
	if (playerUnit->carriedUnit != nullptr)
	{
		transferedUnit = playerUnit->carriedUnit;
		receivingUnit = transferUnit;
		playerUnit->releaseUnit();
	}
	else
	{
		transferedUnit = transferUnit->carriedUnit;
		receivingUnit = playerUnit;
		transferUnit->releaseUnit();
	}
	Menu* newMenu = new UnitMovement(cursor, text, camera, shapeVAO, Renderer, transferedUnit, receivingUnit, 1);
	MenuManager::menuManager.menus.push_back(newMenu);
}

void SelectTransferUnit::GetOptions()
{
	numberOfOptions = transferUnits.size();
}

void SelectTransferUnit::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (inputManager.KeyDownDelay(SDLK_UP))
	{
		PreviousOption();
	}
	else if (inputManager.KeyDownDelay(SDLK_DOWN))
	{
		NextOption();
	}
	else if (inputManager.KeyDownDelay(SDLK_LEFT))
	{
		PreviousOption();
	}
	else if (inputManager.KeyDownDelay(SDLK_RIGHT))
	{
		NextOption();
	}
	if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
	else if (inputManager.isKeyPressed(SDLK_z))
	{
		CancelOption();
		currentOption = 0;
	}
}

UnitListMenu::UnitListMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer) : 
	Menu(Cursor, Text, camera, shapeVAO, Renderer)
{
	numberOfOptions = MenuManager::menuManager.playerManager->units.size();
	fullScreen = true;
	unitData.resize(numberOfOptions);
	for (int i = 0; i < numberOfOptions; i++)
	{
		auto unit = MenuManager::menuManager.playerManager->units[i];
		unitData[i] = std::make_pair(unit, unit->CalculateBattleStats());
	}
	if (MenuManager::menuManager.unitViewSortType > 0)
	{
		sortType = MenuManager::menuManager.unitViewSortType;
		SortView();
	}

	currentPage = MenuManager::menuManager.unitViewPage;
	sortIndicator = MenuManager::menuManager.unitViewIndicator;
	currentSort = MenuManager::menuManager.unitViewCurrentSort;

	pageSortOptions.resize(6);
	pageSortOptions[GENERAL] = 6;
	pageSortOptions[EQUIPMENT] = 5;
	pageSortOptions[COMBAT_STATS] = 8;
	pageSortOptions[PERSONAL] = 5;
	pageSortOptions[WEAPON_RANKS] = 11;
	pageSortOptions[SKILLS] = 2;

	sortNames.resize(37);
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
	sortNames[35] = "Name";
	sortNames[36] = "Skills";

	sortIndicatorLocations.resize(37);

	//+25 for the height of the upside down arrows
	sortIndicatorLocations[0] = glm::ivec2(36, 28);
	sortIndicatorLocations[1] = glm::ivec2(105, 28);
	sortIndicatorLocations[2] = glm::ivec2(156, 28);
	sortIndicatorLocations[3] = glm::ivec2(180, 28);
	sortIndicatorLocations[4] = glm::ivec2(205, 28);
	sortIndicatorLocations[5] = glm::ivec2(233, 28);
	sortIndicatorLocations[6] = glm::ivec2(36, 28);
	sortIndicatorLocations[7] = glm::ivec2(108, 28);
	sortIndicatorLocations[8] = glm::ivec2(164, 28);
	sortIndicatorLocations[9] = glm::ivec2(193, 28);
	sortIndicatorLocations[10] = glm::ivec2(224, 28);
	sortIndicatorLocations[11] = glm::ivec2(36, 28);
	sortIndicatorLocations[12] = glm::ivec2(88, 28);
	sortIndicatorLocations[13] = glm::ivec2(108, 28);
	sortIndicatorLocations[14] = glm::ivec2(136, 28);
	sortIndicatorLocations[15] = glm::ivec2(156, 28);
	sortIndicatorLocations[16] = glm::ivec2(180, 28);
	sortIndicatorLocations[17] = glm::ivec2(204, 28);
	sortIndicatorLocations[18] = glm::ivec2(228, 28);
	sortIndicatorLocations[19] = glm::ivec2(36, 28);
	sortIndicatorLocations[20] = glm::ivec2(92, 28);
	sortIndicatorLocations[21] = glm::ivec2(116, 28);
	sortIndicatorLocations[22] = glm::ivec2(156, 28);
	sortIndicatorLocations[23] = glm::ivec2(208, 28);
	sortIndicatorLocations[24] = glm::ivec2(36, 28);
	sortIndicatorLocations[25] = glm::ivec2(89, 28);
	sortIndicatorLocations[26] = glm::ivec2(105, 28);
	sortIndicatorLocations[27] = glm::ivec2(121, 28);
	sortIndicatorLocations[28] = glm::ivec2(137, 28);
	sortIndicatorLocations[29] = glm::ivec2(153, 28);
	sortIndicatorLocations[30] = glm::ivec2(169, 28);
	sortIndicatorLocations[31] = glm::ivec2(185, 28);
	sortIndicatorLocations[32] = glm::ivec2(201, 28);
	sortIndicatorLocations[33] = glm::ivec2(217, 28);
	sortIndicatorLocations[34] = glm::ivec2(233, 28);
	sortIndicatorLocations[35] = glm::ivec2(36, 28);
	sortIndicatorLocations[36] = glm::ivec2(152, 28);

	profOrder[0] = WeaponData::TYPE_SWORD;
	profOrder[1] = WeaponData::TYPE_LANCE;
	profOrder[2] = WeaponData::TYPE_AXE;
	profOrder[3] = WeaponData::TYPE_BOW;
	profOrder[4] = WeaponData::TYPE_STAFF;
	profOrder[5] = WeaponData::TYPE_FIRE;
	profOrder[6] = WeaponData::TYPE_THUNDER;
	profOrder[7] = WeaponData::TYPE_WIND;
	profOrder[8] = WeaponData::TYPE_LIGHT;
	profOrder[9] = WeaponData::TYPE_DARK;

	proficiencyIconUVs = MenuManager::menuManager.proficiencyIconUVs;
	skillIconUVs = MenuManager::menuManager.skillIconUVs;
}

void UnitListMenu::Draw()
{
	DrawPattern(glm::vec2(256, 20), glm::vec2(0, 31));
	DrawPattern(glm::vec2(256, 171), glm::vec2(0, 53));

	ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());

	Texture2D texture = ResourceManager::GetTexture("UnitViewBG");

	Renderer->setUVs();
	Renderer->DrawSprite(texture, glm::vec2(0, 0), 0, glm::vec2(256, 224));

	glm::mat4 model = glm::mat4();
	if (!sortMode)
	{
		MenuManager::menuManager.DrawIndicator(glm::vec2(-1, 57 + (16 * currentOption)));
	}
	else
	{
		MenuManager::menuManager.DrawIndicator(glm::vec2(127, 8));

		MenuManager::menuManager.DrawArrow(sortIndicatorLocations[currentSort]);
		MenuManager::menuManager.DrawArrow(sortIndicatorLocations[currentSort] + glm::ivec2(0, 19), false);

	}
	MenuManager::menuManager.DrawArrowIndicator(glm::ivec2(168, 14));

	ResourceManager::GetShader("sprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	SBatch Batch;
	Batch.init();
	Batch.begin();
	glm::vec3 greenText = glm::vec3(0.8f, 1.0f, 0.8f);
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

			int textY = 160 + 42 * i;
			text->RenderText(unit->name, 106, textY, 1.0f);

			text->RenderText(unit->unitClass, 202, textY, 1.0f);
			text->RenderTextRight(intToString(unit->level), 480, textY, 1.0f, 28);
			text->RenderTextRight(intToString(unit->experience), 560, textY, 1.0f, 28, greenText);
			text->RenderText(intToString(unit->currentHP) + "/", 690, textY, 1.0f);
			text->RenderText(intToString(unit->maxHP), 720, textY, 1.0f, greenText);
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

			int textY = 160 + 42 * i;
			text->RenderText(unit->name, 106, textY, 1.0f);
			//Should probably just save the battle stats to an array rather than recalculating them here over and over...
			auto battleStats = unitData[i].second;
			text->RenderText(unit->GetEquippedItem()->name, 300, textY, 1.0f); //need to account for no equipped
			text->RenderTextRight(intToString(battleStats.attackDamage), 500, textY, 1, 28);
			text->RenderTextRight(intToString(battleStats.hitAccuracy), 575, textY, 1, 28, greenText);
			text->RenderTextRight(intToString(battleStats.hitAvoid), 700, textY, 1.0f, 28);
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

			int textY = 160 + 42 * i;
			text->RenderText(unit->name, 106, textY, 1.0f);
			text->RenderTextRight(intToString(unit->getStrength()), 275, textY, 1, 28);
			text->RenderTextRight(intToString(unit->getMagic()), 350, textY, 1, 28, greenText);
			text->RenderTextRight(intToString(unit->getSkill()), 425, textY, 1.0f, 28);
			text->RenderTextRight(intToString(unit->getSpeed()), 500, textY, 1.0f, 28, greenText);
			text->RenderTextRight(intToString(unit->getLuck()), 575, textY, 1.0f, 28);
			text->RenderTextRight(intToString(unit->getDefense()), 650, textY, 1.0f, 28, greenText);
			text->RenderTextRight(intToString(unit->getBuild()), 725, textY, 1.0f, 28);
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

			int textY = 160 + 42 * i;
			text->RenderText(unit->name, 106, textY, 1.0f);
			text->RenderTextRight(intToString(unit->getMove()), 300, textY, 1, 28);
			if (unit->fatigue >= 0)
			{
				text->RenderTextRight(intToString(unit->fatigue), 350, textY, 1, 28, greenText);
			}
			else
			{
				text->RenderTextRight("--", 350, textY, 1, 28, greenText);
			}
			text->RenderText("----", 450, textY, 1);
			if (unit->carriedUnit)
			{
				text->RenderText(unit->carriedUnit->name, 600, textY, 1, greenText);
			}
			else
			{
				text->RenderText("-----", 600, textY, 1, greenText);
			}
		}
		break;
	case WEAPON_RANKS:
	{
		pageName = "Weapon Ranks";

		ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());
		texture = ResourceManager::GetTexture("icons");

		for (int i = 0; i < 10; i++)
		{
			Renderer->setUVs(proficiencyIconUVs[profOrder[i]]);
			Renderer->DrawSprite(texture, glm::vec2(85 + 16 * i, 34), 0.0f, glm::vec2(16));
		}

		for (int i = 0; i < numberOfOptions; i++)
		{
			auto unit = unitData[i].first;
			unit->Draw(&Batch, glm::vec2(16, 56 + 16 * i), true);

			int textY = 160 + 42 * i;
			text->RenderText(unit->name, 106, textY, 1.0f);

			auto& profMap = MenuManager::menuManager.profcienciesMap;
			auto toDisplay = unit->weaponProficiencies;

			if (unit->isMounted())
			{
				toDisplay = unit->mount->weaponProficiencies;
			}

			int textX = 275;
			int xOffset = 50;
			text->RenderText(profMap[toDisplay[WeaponData::TYPE_SWORD]], textX, textY, 1);
			textX += xOffset;
			text->RenderText(profMap[toDisplay[WeaponData::TYPE_LANCE]], textX, textY, 1, greenText);
			textX += xOffset;
			text->RenderText(profMap[toDisplay[WeaponData::TYPE_AXE]], textX, textY, 1);
			textX += xOffset;

			text->RenderText(profMap[toDisplay[WeaponData::TYPE_BOW]], textX, textY, 1, greenText);
			textX += xOffset;
			text->RenderText(profMap[toDisplay[WeaponData::TYPE_STAFF]], textX, textY, 1);
			textX += xOffset;

			text->RenderText(profMap[toDisplay[WeaponData::TYPE_FIRE]], textX, textY, 1, greenText);
			textX += xOffset;
			text->RenderText(profMap[toDisplay[WeaponData::TYPE_THUNDER]], textX, textY, 1);
			textX += xOffset;
			text->RenderText(profMap[toDisplay[WeaponData::TYPE_WIND]], textX, textY, 1, greenText);
			textX += xOffset;
			text->RenderText(profMap[toDisplay[WeaponData::TYPE_LIGHT]], textX, textY, 1);
			textX += xOffset;
			text->RenderText(profMap[toDisplay[WeaponData::TYPE_DARK]], textX, textY, 1, greenText);
		}

		break;
	}
	case SKILLS:
	{
		pageName = "Skills";
		text->RenderText("Skills", 450, 96, 1);
		ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());
		texture = ResourceManager::GetTexture("icons");
		for (int i = 0; i < numberOfOptions; i++)
		{
			auto unit = unitData[i].first;
			unit->Draw(&Batch, glm::vec2(16, 56 + 16 * i), true);

			int textY = 160 + 42 * i;
			text->RenderText(unit->name, 106, textY, 1.0f);
			for (int c = 0; c < unit->skills.size(); c++)
			{
				Renderer->setUVs(skillIconUVs[unit->skills[c]]);
				Renderer->DrawSprite(texture, glm::vec2(80 + 16 * c, 56 + 16 * i), 0.0f, glm::vec2(16));
			}
		}
		break;
	}
	}
	Batch.end();
	Batch.renderBatch();
	text->RenderTextCenter(pageName, 106, 29, 1, 40, glm::vec3(0.9f, 0.9f, 1.0f));
	text->RenderText("Name", 106, 96, 1);
	if (sortType >= 25 && sortType <= 34)
	{
		ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());
		texture = ResourceManager::GetTexture("icons");

		Renderer->setUVs(proficiencyIconUVs[profOrder[sortType - 25]]);
		Renderer->DrawSprite(texture, glm::vec2(148, 9), 0.0f, glm::vec2(16));
	}
	else
	{
		text->RenderText(sortNames[sortType], 450, 32, 1); //sort type
	}
	text->RenderText(intToString(currentPage + 1) , 675, 37, 1);
	text->RenderText("/", 686, 37, 1);
	text->RenderText(intToString(numberOfPages), 698, 37, 1);
}

void UnitListMenu::SelectOption()
{
	ResourceManager::PlaySound("select2");
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
			sortType = 0;
			for (int i = 0; i < unitData.size(); i++)
			{
				auto unit = MenuManager::menuManager.playerManager->units[i];
				unitData[i] = std::make_pair(unit, unit->CalculateBattleStats());
			}
		}
		else
		{
			sortType = currentSort;
			SortView();
		}
	}
}

void UnitListMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	MenuManager::menuManager.AnimateIndicator(deltaTime);
	MenuManager::menuManager.AnimateArrow(deltaTime);
	MenuManager::menuManager.AnimateArrowIndicator(deltaTime);
	if (sortMode)
	{
		if (inputManager.isKeyPressed(SDLK_DOWN))
		{
			sortMode = false;
			ResourceManager::PlaySound("cancel");
		}
		else if (inputManager.KeyDownDelay(SDLK_RIGHT))
		{
			if (currentPage < numberOfPages)
			{
				if (currentSort < 36)
				{
					sortIndicator++;
					if (sortIndicator > pageSortOptions[currentPage] - 1)
					{
						sortIndicator = 0;
						currentPage++;
					}
					currentSort++;
					ResourceManager::PlaySound("optionSelect2");
				}
			}
		}
		else if (inputManager.KeyDownDelay(SDLK_LEFT))
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
						currentSort--;
						ResourceManager::PlaySound("optionSelect2");
					}
					else
					{
						sortIndicator = 0;
					}
				}
				else
				{
					currentSort--;
					ResourceManager::PlaySound("optionSelect2");
				}
			}
		}
	}
	else
	{
		if (inputManager.KeyDownDelay(SDLK_UP, 0.05f, 0.175f))
		{
			if (currentOption == 0)
			{
				if (inputManager.isKeyPressed(SDLK_UP))
				{
					sortMode = true;

				}
			}
			else
			{
				currentOption--;
			}
			ResourceManager::PlaySound("optionSelect1");
		}
		else if (inputManager.KeyDownDelay(SDLK_DOWN, 0.05f, 0.175f))
		{
			currentOption++;
			if (currentOption > numberOfOptions - 1)
			{
				currentOption = numberOfOptions - 1;
			}
			else
			{
				ResourceManager::PlaySound("optionSelect1");
			}
		}
		else if (inputManager.KeyDownDelay(SDLK_RIGHT, 0.1f, 0.175f))
		{
			if (currentPage < numberOfPages - 1)
			{
				currentSort -= sortIndicator;
				currentSort += pageSortOptions[currentPage];
				currentPage++;
				if (currentPage >= numberOfPages - 1)
				{
					currentPage = numberOfPages - 1;
				}
				sortIndicator = 0;
				ResourceManager::PlaySound("optionSelect2");
			}
		}
		else if (inputManager.KeyDownDelay(SDLK_LEFT, 0.1f, 0.175f))
		{
			currentPage--;
			if (currentPage < 0)
			{
				currentPage = 0;
			}
			else
			{
				currentSort -= sortIndicator;
				currentSort -= pageSortOptions[currentPage];
				sortIndicator = 0;
				ResourceManager::PlaySound("optionSelect2");

			}
		}
		else if (inputManager.isKeyPressed(SDLK_SPACE))
		{
			MenuManager::menuManager.AddUnitStatMenu(unitData[currentOption].first);
		}
	}
	if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
	else if (inputManager.isKeyPressed(SDLK_z))
	{
		ResourceManager::PlaySound("cancel");
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
	MenuManager::menuManager.unitViewPage = currentPage;
	MenuManager::menuManager.unitViewIndicator = sortIndicator;
	MenuManager::menuManager.unitViewCurrentSort = currentSort;
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
	case 21:
		std::sort(unitData.begin(), unitData.end(), [](const auto& a, const auto& b) {
			if (a.first->fatigue < 0 || b.first->fatigue < 0) {
				return a.first->fatigue < b.first->fatigue;
			}
			return a.first->fatigue > b.first->fatigue;
			});
		break;
	case 25:
		std::sort(unitData.begin(), unitData.end(), [](const auto& a, const auto& b) {
			auto aDisplay = a.first->weaponProficiencies;
			auto bDisplay = b.first->weaponProficiencies;
			if (a.first->isMounted())
			{
				aDisplay = a.first->mount->weaponProficiencies;
			}
			if (b.first->isMounted())
			{
				bDisplay = b.first->mount->weaponProficiencies;
			}
			return aDisplay[WeaponData::TYPE_SWORD] > bDisplay[WeaponData::TYPE_SWORD];
			});
		break;
	case 26:
		std::sort(unitData.begin(), unitData.end(), [](const auto& a, const auto& b) {
			auto aDisplay = a.first->weaponProficiencies;
			auto bDisplay = b.first->weaponProficiencies;
			if (a.first->isMounted())
			{
				aDisplay = a.first->mount->weaponProficiencies;
			}
			if (b.first->isMounted())
			{
				bDisplay = b.first->mount->weaponProficiencies;
			}
			return aDisplay[WeaponData::TYPE_LANCE] > bDisplay[WeaponData::TYPE_LANCE];
			});
		break;
	case 27:
		std::sort(unitData.begin(), unitData.end(), [](const auto& a, const auto& b) {
			return a.first->weaponProficiencies[WeaponData::TYPE_AXE] > b.first->weaponProficiencies[WeaponData::TYPE_AXE];
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
	case 36:
		std::sort(unitData.begin(), unitData.end(), [](const auto& a, const auto& b) {
			return a.first->skills.size() > b.first->skills.size() || ((a.first->skills.size() > 0 && b.first->skills.size() > 0) && a.first->skills[0] < b.first->skills[0]);
			});
		break;
	}
}

StatusMenu::StatusMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer)
	: Menu(Cursor, Text, camera, shapeVAO, Renderer)
{
	numberOfOptions = 2;
	fullFadeIn = true;
}

void StatusMenu::Draw()
{
	if ((!fullFadeIn && !exitMenu) || (exitMenu && !fullFadeOut))
	{
		DrawPattern(glm::vec2(256, 72), glm::vec2(0, 32));
		DrawPattern(glm::vec2(256, 88), glm::vec2(0, 136));

		ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());

		Texture2D texture = ResourceManager::GetTexture("StatusMenuBG");
		Renderer->setUVs();
		Renderer->DrawSprite(texture, glm::vec2(0, 0), 0, glm::vec2(256, 224));

		MenuManager::menuManager.DrawIndicator(glm::vec2(7, 137 + 16 * currentOption));

		//So I think what I would want here ultimately would be for each unit to have an "affiliation" property,
		//and using that I will construct vectors of references to these. When a unit dies it would have to be removed from both
		//its team vector and its affiliation vector.
		//Not going to bother with that right now, but it is an interesting thing to consider for the future
		//Though only if I would want a separate affiliation from team, and who knows
		if (currentOption == 0)
		{
			Texture2D portraitTexture = ResourceManager::GetTexture("Portraits");
			Renderer->setUVs(UnitResources::portraitUVs[MenuManager::menuManager.playerManager->units[0]->portraitID][0]);
			Renderer->DrawSprite(portraitTexture, glm::vec2(200, 144), 0, glm::vec2(48, 64), glm::vec4(1));

			ResourceManager::GetShader("sprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());
			SBatch Batch;
			Batch.init();
			Batch.begin();
			MenuManager::menuManager.playerManager->units[0]->Draw(&Batch, glm::vec2(128, 144), true);
			Batch.end();
			Batch.renderBatch();

			text->RenderText(MenuManager::menuManager.playerManager->units[0]->name, 450, 396, 1);
			text->RenderText(MenuManager::menuManager.playerManager->units[0]->unitClass, 400, 439, 1);
			text->RenderText("HP", 400, 530, 1, glm::vec3(0.69f, 0.62f, 0.49f));
			text->RenderText(intToString(MenuManager::menuManager.playerManager->units[0]->currentHP), 475, 530, 1);
			text->RenderText("/", 500, 530, 1);
			text->RenderText(intToString(MenuManager::menuManager.playerManager->units[0]->maxHP), 515, 530, 1);
			text->RenderTextRight(intToString(MenuManager::menuManager.playerManager->units[0]->level), 575, 487, 1, 28);
		}
		else
		{
			text->RenderText("No leader for this army", 425, 460, 1);
		}

		text->RenderText(MenuManager::menuManager.playerManager->units[0]->name, 100, 375, 1);
		text->RenderText("-----", 100, 431, 1);

		text->RenderTextCenter("Chapter 1: The Warrior of Fiana", 0, 26, 1, 744); //Chapter Tile Goes here
		text->RenderText("Sieze the manor's gate", 175, 91, 1); //Objective goes here

		text->RenderTextRight(intToString(MenuManager::menuManager.playerManager->funds), 574, 230, 1, 70);
		text->RenderTextRight(intToString(MenuManager::menuManager.playerManager->currentRound), 200, 230, 1, 42);
		text->RenderTextRight(intToString(MenuManager::menuManager.playerManager->totalCaptures), 250, 187, 1, 42);
		text->RenderTextRight(intToString(MenuManager::menuManager.playerManager->totalWins), 375, 187, 1, 42);

	}
	if (fullFadeIn || fullFadeOut)
	{
		DrawFadeIn();
	}
}

void StatusMenu::SelectOption()
{
}

void StatusMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (fullFadeIn)
	{
		HandleFadeIn(deltaTime);
	}
	else if (fullFadeOut)
	{
		HandleFadeOut(deltaTime);
	}
	else
	{
		MenuManager::menuManager.AnimateIndicator(deltaTime);
		if (inputManager.isKeyPressed(SDLK_UP))
		{
			PreviousOption();
		}
		else if (inputManager.isKeyPressed(SDLK_DOWN))
		{
			NextOption();
		}
		else if (inputManager.isKeyPressed(SDLK_z))
		{
			ResourceManager::PlaySound("cancel");
			exitMenu = true;
			fullFadeIn = true;
		}
	}
}

OptionsMenu::OptionsMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer)
	: Menu(Cursor, Text, camera, shapeVAO, Renderer)
{
	numberOfOptions = 16;
	fullFadeIn = true;
	optionDescriptions[0] = "Attack animation settings";
	optionDescriptions[1] = "Terrain Display";
	optionDescriptions[2] = "Unit window display";
	optionDescriptions[3] = "Auto cursor";
	optionDescriptions[4] = "How fast to display messages";
	optionDescriptions[5] = "Unit movement speed";
	optionDescriptions[6] = "Sound settings(not working)";
	optionDescriptions[7] = "Music settings";
	optionDescriptions[8] = "Volume settings";
	optionDescriptions[9] = "Window tile settings";
	optionDescriptions[10] = "Upper red layer";
	optionDescriptions[11] = "Upper green layer";
	optionDescriptions[12] = "Upper blue layer";
	optionDescriptions[13] = "Lower red layer";
	optionDescriptions[14] = "Lower green layer";
	optionDescriptions[15] = "Lower blue layer";
	optionDescriptions[16] = "Return to default color";
}

void OptionsMenu::Draw()
{
	if ((!fullFadeIn && !exitMenu) || (exitMenu && !fullFadeOut))
	{
		//Appears to be a bit of black around/under the main background on the edges, keep in mind
		glm::vec2 size(256, 161);
		glm::vec2 pos;

		int patternID = Settings::settings.backgroundPattern;
		auto inColor = Settings::settings.backgroundColors[patternID];
		glm::vec3 topColor = glm::vec3(inColor[0], inColor[1], inColor[2]);
		glm::vec3 bottomColor = glm::vec3(inColor[3], inColor[4], inColor[5]);

		DrawPattern(glm::vec2(256, 161), glm::vec2(0, 31));

		Texture2D optionIcons = ResourceManager::GetTexture("UIItems");
		ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());
		auto optionIconUVs = MenuManager::menuManager.optionIconUVs;
		glm::vec2 iconSize = glm::vec2(16);
		int adjustedOffset = round((yOffset / 600.0f) * 224.0f);
		for (int i = 0; i < 11; i++)
		{
			Renderer->setUVs(optionIconUVs[i]);
			Renderer->DrawSprite(optionIcons, glm::vec2(24, 39 + 24 * i - adjustedOffset), 0, iconSize);
		}
		//I have no idea why FE5 has this black line being drawn here. It is drawn over the option sprites but under the text.
		//I don't think it looks as good as drawing over both but it is replicated here.
		glm::mat4 model = glm::mat4();
		model = glm::translate(model, glm::vec3(5, 191, 0.0f));
		model = glm::scale(model, glm::vec3(256, 1, 0.0f));
		ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
		ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
		ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.0f, 0.0f));
		ResourceManager::GetShader("shape").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		if (yOffset < 256)
		{
			MenuManager::menuManager.DrawArrow(glm::ivec2(120, 186));
		}
		if (yOffset > 0)
		{
			MenuManager::menuManager.DrawArrow(glm::ivec2(120, 32), false);
		}

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

		text->RenderText("Unit Window", optionNameX, 245 - (yOffset), 1);
		RenderText("On", selectionXStart, 245 - (yOffset), 1, Settings::settings.unitWindow);
		RenderText("Off", selectionXStart + text->GetTextWidth("On", 1) + 50, 245 - (yOffset), 1, !Settings::settings.unitWindow);

		text->RenderText("Autocursor", optionNameX, 309 - (yOffset), 1);
		RenderText("On", selectionXStart, 309 - (yOffset), 1, Settings::settings.autoCursor);
		RenderText("Off", selectionXStart + text->GetTextWidth("On", 1) + 50, 309 - (yOffset), 1, !Settings::settings.autoCursor);

		text->RenderText("Text Speed", optionNameX, 373 - (yOffset), 1);
		RenderText("Slow", selectionXStart, 373 - (yOffset), 1, Settings::settings.textSpeed == 0);
		xOffset = 0;
		xOffset += selectionXStart + text->GetTextWidth("Slow", 1) + 50;
		RenderText("Normal", xOffset, 373 - (yOffset), 1, Settings::settings.textSpeed == 1);
		xOffset += text->GetTextWidth("Normal", 1) + 50;
		RenderText("Fast", xOffset, 373 - (yOffset), 1, Settings::settings.textSpeed == 2);

		text->RenderText("Unit Speed", optionNameX, 437 - (yOffset), 1);
		RenderText("Normal", selectionXStart, 437 - (yOffset), 1, Settings::settings.unitSpeed < 5);
		RenderText("Fast", selectionXStart + text->GetTextWidth("Normal", 1) + 50, 437 - (yOffset), 1, Settings::settings.unitSpeed >= 5);

		text->RenderText("Audio", optionNameX, 501 - (yOffset), 1);
		RenderText("Stereo", selectionXStart, 501 - (yOffset), 1, Settings::settings.sterero);
		RenderText("Mono", selectionXStart + text->GetTextWidth("Stereo", 1) + 50, 501 - (yOffset), 1, !Settings::settings.sterero);

		text->RenderText("Music", optionNameX, 565 - (yOffset), 1);
		RenderText("On", selectionXStart, 565 - (yOffset), 1, Settings::settings.music);
		RenderText("Off", selectionXStart + text->GetTextWidth("On", 1) + 50, 565 - (yOffset), 1, !Settings::settings.music);

		text->RenderText("Volume", optionNameX, 629 - (yOffset), 1);
		RenderText("4", selectionXStart, 629 - (yOffset), 1, Settings::settings.volume == 4);
		xOffset = 0;
		xOffset += selectionXStart + text->GetTextWidth("4", 1) + 50;
		RenderText("3", xOffset, 629 - (yOffset), 1, Settings::settings.volume == 3);
		xOffset += text->GetTextWidth("3", 1) + 50;
		RenderText("2", xOffset, 629 - (yOffset), 1, Settings::settings.volume == 2);
		xOffset += text->GetTextWidth("2", 1) + 50;
		RenderText("Off", xOffset, 629 - (yOffset), 1, Settings::settings.volume == 1);

		text->RenderText("Window Tile", optionNameX, 693 - (yOffset), 1);
		RenderText("1", selectionXStart, 693 - (yOffset), 1, Settings::settings.backgroundPattern == 0);
		xOffset = 0;
		xOffset += selectionXStart + text->GetTextWidth("1", 1) + 50;
		RenderText("2", xOffset, 693 - (yOffset), 1, Settings::settings.backgroundPattern == 1);

		text->RenderText("Window Color", optionNameX, 757 - (yOffset), 1); //757

		text->RenderText("Upper", optionNameX + 50, 777 - (yOffset), 1); //42 diff
		text->RenderText("Lower", optionNameX + 50, 841 - (yOffset), 1); //62 diff was 799

		RenderText("Default", selectionXStart, 930 - (yOffset), 1, Settings::settings.editedColor[patternID]);

		ResourceManager::GetShader("Nsprite").Use();

		Renderer->setUVs(MenuManager::menuManager.colorBarsUV);
		Renderer->DrawSprite(optionIcons, glm::vec2(128, 287 - adjustedOffset), 0, glm::vec2(107, 56));

		ResourceManager::GetShader("gradient").Use().SetMatrix4("projection", camera->getOrthoMatrix());
		ResourceManager::GetShader("gradient").SetFloat("alpha", 1.0f);
		ResourceManager::GetShader("gradient").SetFloat("barEnd", 232.0f);
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(139, 290 - adjustedOffset, 0.0f));
		model = glm::scale(model, glm::vec3(std::max(1, 4 * int(topColor.x / 8)), 2, 0.0f));
		ResourceManager::GetShader("gradient").SetVector3f("shapeColor", glm::vec3(1.0f, 0.0f, 0.0f));
		ResourceManager::GetShader("gradient").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		model = glm::mat4();
		model = glm::translate(model, glm::vec3(139, 298 - adjustedOffset, 0.0f));
		model = glm::scale(model, glm::vec3(std::max(1, 4 * int(topColor.y / 8)), 2, 0.0f));
		ResourceManager::GetShader("gradient").SetVector3f("shapeColor", glm::vec3(0.0f, 1.0f, 0.0f));
		ResourceManager::GetShader("gradient").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		model = glm::mat4();
		model = glm::translate(model, glm::vec3(139, 306 - adjustedOffset, 0.0f));
		model = glm::scale(model, glm::vec3(std::max(1, 4 * int(topColor.z / 8)), 2, 0.0f));
		ResourceManager::GetShader("gradient").SetVector3f("shapeColor", glm::vec3(0.0f, 0.0f, 1.0f));
		ResourceManager::GetShader("gradient").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		model = glm::mat4();
		model = glm::translate(model, glm::vec3(139, 322 - adjustedOffset, 0.0f));
		model = glm::scale(model, glm::vec3(std::max(1, 4 * int(bottomColor.x / 8)), 2, 0.0f));
		ResourceManager::GetShader("gradient").SetVector3f("shapeColor", glm::vec3(1.0f, 0.0f, 0.0f));
		ResourceManager::GetShader("gradient").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		model = glm::mat4();
		model = glm::translate(model, glm::vec3(139, 330 - adjustedOffset, 0.0f));
		model = glm::scale(model, glm::vec3(std::max(1, 4 * int(bottomColor.y / 8)), 2, 0.0f));
		ResourceManager::GetShader("gradient").SetVector3f("shapeColor", glm::vec3(0.0f, 1.0f, 0.0f));
		ResourceManager::GetShader("gradient").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		model = glm::mat4();
		model = glm::translate(model, glm::vec3(139, 338 - adjustedOffset, 0.0f));
		model = glm::scale(model, glm::vec3(std::max(1, 4 * int(bottomColor.z / 8)), 2, 0.0f));
		ResourceManager::GetShader("gradient").SetVector3f("shapeColor", glm::vec3(0.0f, 0.0f, 1.0f));
		ResourceManager::GetShader("gradient").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		ResourceManager::GetShader("Nsprite").Use();
		Renderer->setUVs(MenuManager::menuManager.colorIndicatorUV);

		Renderer->DrawSprite(optionIcons, glm::vec2(136 + 4 * int(topColor.x / 8), 287 - adjustedOffset), 0, glm::vec2(3, 8));
		Renderer->DrawSprite(optionIcons, glm::vec2(136 + 4 * int(topColor.y / 8), 295 - adjustedOffset), 0, glm::vec2(3, 8));
		Renderer->DrawSprite(optionIcons, glm::vec2(136 + 4 * int(topColor.z / 8), 303 - adjustedOffset), 0, glm::vec2(3, 8));

		Renderer->DrawSprite(optionIcons, glm::vec2(136 + 4 * int(bottomColor.x / 8), 319 - adjustedOffset), 0, glm::vec2(3, 8));
		Renderer->DrawSprite(optionIcons, glm::vec2(136 + 4 * int(bottomColor.y / 8), 327 - adjustedOffset), 0, glm::vec2(3, 8));
		Renderer->DrawSprite(optionIcons, glm::vec2(136 + 4 * int(bottomColor.z / 8), 335 - adjustedOffset), 0, glm::vec2(3, 8));

		DrawIndicators();

		DrawPattern(glm::vec2(256, 32), glm::vec2(0, 192));

		text->RenderTextCenter(optionDescriptions[currentOption], 0, 541, 1, 800);

		Texture2D test = ResourceManager::GetTexture("OptionsScreenBackground");
		ResourceManager::GetShader("Nsprite").Use();
		Renderer->setUVs();
		Renderer->DrawSprite(test, glm::vec2(0, 0), 0, glm::vec2(256, 224));
	}
	if (fullFadeIn || fullFadeOut)
	{
		DrawFadeIn();
	}
}

void OptionsMenu::DrawIndicators()
{
	MenuManager::menuManager.DrawIndicator(glm::vec2(8, indicatorY), false);

	int xLoc = 111;
	if (currentOption == 0)
	{
		if (Settings::settings.mapAnimations == 1)
		{
			xLoc = 151;
		}
		else if (Settings::settings.mapAnimations == 2)
		{
			xLoc = 183;
		}
	}
	else if (currentOption == 1)
	{
		if (Settings::settings.showTerrain == 0)
		{
			xLoc = 135;
		}
	}
	else if (currentOption == 2)
	{
		if (Settings::settings.autoCursor == 0)
		{
			xLoc = 135;
		}
	}
	else if (currentOption == 3)
	{
		if (Settings::settings.autoCursor == 0)
		{
			xLoc = 135;
		}
	}
	else if (currentOption == 4)
	{
		if (Settings::settings.textSpeed == 1)
		{
			xLoc = 143;
		}
		else if (Settings::settings.textSpeed == 2)
		{
			xLoc = 183;
		}
	}
	else if (currentOption == 5)
	{
		if (Settings::settings.unitSpeed > 3)
		{
			xLoc = 151;
		}
	}
	else if (currentOption == 6)
	{
		if (Settings::settings.sterero == 0)
		{
			xLoc = 151;
		}
	}
	else if (currentOption == 7)
	{
		if (Settings::settings.music == 0)
		{
			xLoc = 135;
		}
	}
	else if (currentOption == 8)
	{
		if (Settings::settings.volume == 3)
		{
			xLoc = 132;
		}
		else if (Settings::settings.volume == 2)
		{
			xLoc = 153;
		}
		else if (Settings::settings.volume == 1)
		{
			xLoc = 173;
		}
	}
	else if (currentOption == 9)
	{
		if (Settings::settings.backgroundPattern == 1)
		{
			xLoc = 132;
		}
	}

	MenuManager::menuManager.DrawIndicator(glm::vec2(xLoc, indicatorY2));
}

void OptionsMenu::SelectOption()
{

}

void OptionsMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	MenuManager::menuManager.AnimateArrow(deltaTime);
	MenuManager::menuManager.AnimateIndicator(deltaTime);
	if (fullFadeIn)
	{
		HandleFadeIn(deltaTime);
	}
	else if (exitMenu && fullFadeOut)
	{
		HandleFadeOut(deltaTime);
	}
	else
	{
		if (fullFadeOut)
		{
			HandleFadeOut(deltaTime);
		}
		//Still need to handle the bottom case and disable input during that animation
		int& patternID = Settings::settings.backgroundPattern;
		auto& inColor = Settings::settings.backgroundColors[patternID];
		auto defaultColor = Settings::settings.defaultColors[patternID];
		if (!moveToBottom)
		{
			if (inputManager.KeyDownDelay(SDLK_UP, 0.05f, 0.25f))
			{
				currentOption--;
				if (currentOption < 0)
				{
					currentOption = 0;
				}
				else
				{
					ResourceManager::PlaySound("optionSelect1");
					if (currentOption == 0)
					{
						indicatorY = 39;
						goal = 0;
						up = true;
						hitBottom = false;
						indicatorY2 = indicatorY;
					}
					else
					{
						if (currentOption < 10)
						{
							indicatorY -= indicatorIncrement;
							//The boundary is a bit different once the bottom has been hit
							int bound = 63;
							if (hitBottom)
							{
								bound = 55;
							}
							if (indicatorY < bound)
							{
								indicatorY = bound;
								up = true;
								goal -= 64;
							}
							indicatorY2 = indicatorY;
						}
						else
						{
							if (currentOption == 12)
							{
								indicatorY2 -= 16;
							}
							else if (currentOption == 15)
							{
								indicatorY2 = 153;
							}
							else
							{
								indicatorY2 -= 8;
							}
						}
					}
				}

			}
			else if (inputManager.KeyDownDelay(SDLK_DOWN, 0.05f, 0.25f))
			{
				currentOption++;
				if (currentOption > numberOfOptions)
				{
					currentOption = numberOfOptions;
				}
				else
				{
					ResourceManager::PlaySound("optionSelect1");
					if (currentOption == 10)
					{
						indicatorY = 279;
						indicatorY2 = 281;
						goal = 470;
						down = true;
						hitBottom = true;
						moveToBottom = true;
					}
					else
					{
						if (currentOption < 10)
						{
							indicatorY += indicatorIncrement;

							int bound = 159;
							if (hitBottom)
							{
								bound = 151;
							}
							if (indicatorY > bound)
							{
								indicatorY = bound;
								down = true;
								goal += 64;
							}
							indicatorY2 = indicatorY;
						}
						else
						{
							if (currentOption == 13)
							{
								indicatorY2 += 16;
							}
							else if (currentOption == 16)
							{
								indicatorY2 = 169;
							}
							else
							{
								indicatorY2 += 8;
							}
						}
					}
				}
				if (currentOption < 10)
				{
					indicatorY2 = indicatorY;
				}
			}
			else if (inputManager.KeyDownDelay(SDLK_RIGHT, 0.05f, 0.15f))
			{
				switch (currentOption)
				{
				case 0:
					Settings::settings.mapAnimations++;
					if (Settings::settings.mapAnimations > 2)
					{
						Settings::settings.mapAnimations = 2;
					}
					else
					{
						ResourceManager::PlaySound("optionSelect2");
					}
					break;
				case 1:
					if (Settings::settings.showTerrain)
					{
						Settings::settings.showTerrain = false;
						ResourceManager::PlaySound("optionSelect2");
					}
					break;
				case 2:
					if (Settings::settings.unitWindow)
					{
						Settings::settings.unitWindow = false;
						ResourceManager::PlaySound("optionSelect2");
					}
					break;
				case 3:
					if (Settings::settings.autoCursor)
					{
						Settings::settings.autoCursor = false;
						ResourceManager::PlaySound("optionSelect2");
					}
					break;
				case 4:
					Settings::settings.textSpeed++;
					if (Settings::settings.textSpeed > 2)
					{
						Settings::settings.textSpeed = 2;
					}
					else
					{
						ResourceManager::PlaySound("optionSelect2");
					}
					break;
				case 5:
					if (Settings::settings.unitSpeed < 5)
					{
						Settings::settings.unitSpeed = 5;
						ResourceManager::PlaySound("optionSelect2");
					}
					break;
				case 6:
					if (Settings::settings.sterero)
					{
						Settings::settings.sterero = false;
						ResourceManager::PlaySound("optionSelect2");
					}
					break;
				case 7:
					if (Settings::settings.music)
					{
						Settings::settings.music = false;
						ResourceManager::PlaySound("optionSelect2");
					}
					break;
				case 8:
					if (Settings::settings.volume > 1)
					{
						Settings::settings.volume--;
						ResourceManager::PlaySound("optionSelect2");
					}
					break;
				case 9:
					if (patternID < 1)
					{
						patternID++;
						ResourceManager::PlaySound("optionSelect2");
					}
					break;
				case 10:
					if (inColor[0] < 192)
					{
						inColor[0] += 8;
						inColor[0] = glm::clamp(inColor[0], 0, 192);
						CheckColorChange(inColor, defaultColor, patternID);
					}
					break;
				case 11:
					if (inColor[1] < 192)
					{
						inColor[1] += 8;
						inColor[1] = glm::clamp(inColor[1], 0, 192);
						CheckColorChange(inColor, defaultColor, patternID);
					}
					break;
				case 12:
					if (inColor[2] < 192)
					{
						inColor[2] += 8;
						inColor[2] = glm::clamp(inColor[2], 0, 192);
						CheckColorChange(inColor, defaultColor, patternID);
					}
					break;
				case 13:
					if (inColor[3] < 192)
					{
						inColor[3] += 8;
						inColor[3] = glm::clamp(inColor[3], 0, 192);
						CheckColorChange(inColor, defaultColor, patternID);
					}
					break;
				case 14:
					if (inColor[4] < 192)
					{
						inColor[4] += 8;
						inColor[4] = glm::clamp(inColor[4], 0, 192);
						CheckColorChange(inColor, defaultColor, patternID);
					}
					break;
				case 15:
					if (inColor[5] < 192)
					{
						inColor[5] += 8;
						inColor[5] = glm::clamp(inColor[5], 0, 192);
						CheckColorChange(inColor, defaultColor, patternID);
					}
					break;
				}
			}
			else if (inputManager.KeyDownDelay(SDLK_LEFT, 0.05f, 0.15f))
			{
				switch (currentOption)
				{
				case 0:
					Settings::settings.mapAnimations--;
					if (Settings::settings.mapAnimations < 0)
					{
						Settings::settings.mapAnimations = 0;
					}
					else
					{
						ResourceManager::PlaySound("optionSelect2");
					}
					break;
				case 1:
					if (!Settings::settings.showTerrain)
					{
						Settings::settings.showTerrain = true;
						ResourceManager::PlaySound("optionSelect2");
					}
					break;
				case 2:
					if (!Settings::settings.unitWindow)
					{
						Settings::settings.unitWindow = true;
						ResourceManager::PlaySound("optionSelect2");
					}
					break;
				case 3:
					if (!Settings::settings.autoCursor)
					{
						Settings::settings.autoCursor = true;
						ResourceManager::PlaySound("optionSelect2");
					}
					break;
				case 4:
					Settings::settings.textSpeed--;
					if (Settings::settings.textSpeed < 0)
					{
						Settings::settings.textSpeed = 0;
					}
					else
					{
						ResourceManager::PlaySound("optionSelect2");
					}
					break;
				case 5:
					if (Settings::settings.unitSpeed > 2.5f)
					{
						Settings::settings.unitSpeed = 2.5f;
						ResourceManager::PlaySound("optionSelect2");
					}
					break;
				case 6:
					if (!Settings::settings.sterero)
					{
						Settings::settings.sterero = true;
						ResourceManager::PlaySound("optionSelect2");
					}
					break;
				case 7:
					if (!Settings::settings.music)
					{
						Settings::settings.music = true;
						ResourceManager::PlaySound("optionSelect2");
					}
					break;
				case 8:
					if (Settings::settings.volume < 4)
					{
						Settings::settings.volume++;
						ResourceManager::PlaySound("optionSelect2");
					}
					break;
				case 9:
					if (patternID > 0)
					{
						patternID--;
						ResourceManager::PlaySound("optionSelect2");
					}
					break;
				case 10:
					if (inColor[0] > 0)
					{
						inColor[0] -= 8;
						inColor[0] = glm::clamp(inColor[0], 0, 192);
						CheckColorChange(inColor, defaultColor, patternID);
					}
					break;
				case 11:
					if (inColor[1] > 0)
					{
						inColor[1] -= 8;
						inColor[1] = glm::clamp(inColor[1], 0, 192);
						CheckColorChange(inColor, defaultColor, patternID);
					}
					break;
				case 12:
					if (inColor[2] > 0)
					{
						inColor[2] -= 8;
						inColor[2] = glm::clamp(inColor[2], 0, 192);
						CheckColorChange(inColor, defaultColor, patternID);
					}
					break;
				case 13:
					if (inColor[3] > 0)
					{
						inColor[3] -= 8;
						inColor[3] = glm::clamp(inColor[3], 0, 192);
						CheckColorChange(inColor, defaultColor, patternID);
					}
					break;
				case 14:
					if (inColor[4] > 0)
					{
						inColor[4] -= 8;
						inColor[4] = glm::clamp(inColor[4], 0, 192);
						CheckColorChange(inColor, defaultColor, patternID);
					}
					break;
				case 15:
					if (inColor[5] > 0)
					{
						inColor[5] -= 8;
						inColor[5] = glm::clamp(inColor[5], 0, 192);
						CheckColorChange(inColor, defaultColor, patternID);
					}
					break;
				}
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
				if (hitBottom)
				{
					moveToBottom = false;
				}
			}
			if (currentOption == 10)
			{
				indicatorY = 279 - round((yOffset / 600.0f) * 224.0f);
				indicatorY2 = indicatorY + 2;
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
			if (currentOption == 16 && Settings::settings.editedColor[patternID])
			{
				inColor = defaultColor;
				Settings::settings.editedColor[patternID] = false;
				ResourceManager::PlaySound("select2");
			}
		}
		else if (inputManager.isKeyPressed(SDLK_z))
		{
			if (Settings::settings.music)
			{
				if (!Mix_PlayingMusic())
				{
					ResourceManager::PlayMusic("PlayerTurn"); //This needs to do something to determine the correct music
				}
			}
			else
			{
				Mix_HookMusicFinished(nullptr);
				Mix_FadeOutMusic(500.0f);
			}
			if (Settings::settings.volume > 1)
			{
				Mix_Volume(-1, 128 - 48 * (4 - Settings::settings.volume));
				ResourceManager::PlaySound("cancel");
			}
			else
			{
				Mix_Volume(-1, 0);
			}
			exitMenu = true;
			fullFadeIn = true;
		}
	}
}

void OptionsMenu::CheckColorChange(std::vector<int>& inColor, std::vector<int>& defaultColor, int& patternID)
{
	if (inColor != defaultColor)
	{
		Settings::settings.editedColor[patternID] = true;
	}
	else
	{
		Settings::settings.editedColor[patternID] = false;
	}
	ResourceManager::PlaySound("optionSelect2");
}

void OptionsMenu::RenderText(std::string toWrite, float x, float y, float scale, bool selected)
{
	glm::vec3 grey = glm::vec3(0.64f);
	glm::vec3 selectedColor = glm::vec3(0.77, 0.92, 1.0f);
	glm::vec3 color = selected ? selectedColor : grey;
	text->RenderText(toWrite, x, y, 1, color);
}

VendorMenu::VendorMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer, Unit* buyer, Vendor* vendor) :
	Menu(Cursor, Text, camera, shapeVAO, Renderer), buyer(buyer), vendor(vendor)
{
	fullFadeIn = true;
	numberOfOptions = vendor->items.size();

	textManager.textLines.clear();

	textManager.textLines.push_back(SpeakerText{ nullptr, 2, "This is a tool shop.\nYou buyin' or sellin'?<2", 14});
	textManager.textLines.push_back(SpeakerText{ nullptr, 2, "What are ya buyin'?<2", 14});
	textManager.textLines.push_back(SpeakerText{ nullptr, 2, "What are ya sellin'?<2", 14});
	textManager.textLines.push_back(SpeakerText{ nullptr, 2, "Is that all?<2", 14});
	textManager.textLines.push_back(SpeakerText{ nullptr, 2, "Thanks, come back any time.<3", 14});
	textManager.textLines.push_back(SpeakerText{ nullptr, 2, "You sure?<2", 14 });
	textManager.textLines.push_back(SpeakerText{ nullptr, 2, "You want anything else?<2", 14 });
	textManager.textLines.push_back(SpeakerText{ nullptr, 2, "That one look good?<2", 14 });
	textManager.textLines.push_back(SpeakerText{ nullptr, 2, "You haven't enough cash for that.<2", 14 });
	textManager.textLines.push_back(SpeakerText{ nullptr, 2, "Looks like you haven't got the space.\nYou can't carry anything else.<2", 14 });
	textManager.textLines.push_back(SpeakerText{ nullptr, 2, "You ain't got nothing to sell.<2", 14 });
	textManager.textLines.push_back(SpeakerText{ nullptr, 2, "I can't buy that one.<2", 14 });
	textManager.textLines.push_back(SpeakerText{ nullptr, 2, "Anything else to sell?<2", 14 });


	textManager.init();
	textManager.active = true;

	state = GREETING;
}

void VendorMenu::Draw()
{
	if ((!fullFadeIn && !exitMenu) || (exitMenu && !fullFadeOut))
	{
		ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());
		auto texture = ResourceManager::GetTexture("VendorBackground");

		Renderer->setUVs();
		Renderer->DrawSprite(texture, glm::vec2(0), 0.0f, glm::vec2(256, 224));

		if (textManager.ShowText())
		{
			textManager.Draw(text, Renderer, camera);
		}

		if (!textManager.active && !delay)
		{
			switch (state)
			{
			case GREETING:
				MenuManager::menuManager.DrawIndicator(glm::ivec2(143 + 49 * !buying, 50));
				text->RenderText("Buy", 500, 133, 1);
				text->RenderText("Sell", 646, 133, 1);
				break;
			case BUYING:
				MenuManager::menuManager.DrawIndicator(glm::ivec2(88, 89 + 16 * currentOption));
				break;
			case SELLING:
				MenuManager::menuManager.DrawIndicator(glm::ivec2(88, 89 + 16 * currentOption));
				break;
			case CONFIRMING:
				MenuManager::menuManager.DrawIndicator(glm::ivec2(143 + (49 * !confirm), 50));
				MenuManager::menuManager.DrawIndicator(glm::ivec2(88, 89 + 16 * currentOption), false);
				text->RenderText("Yes", 500, 133, 1);
				text->RenderText("No", 646, 133, 1);
				break;
			}
		}
		else
		{
			if (state == CONFIRMING)
			{
				MenuManager::menuManager.DrawIndicator(glm::ivec2(88, 89 + 16 * currentOption), false);
			}
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
				text->RenderTextRight(intToString(item.maxUses), 575, yPos + 43 * i + 1, 1, 28);
				text->RenderTextRight(intToString(item.value), 675, yPos + 43 * i + 1, 1, 56); //price

				ResourceManager::GetShader("Nsprite").Use();
				ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());
				auto texture = ResourceManager::GetTexture("icons");

				Renderer->setUVs(MenuManager::menuManager.itemIconUVs[item.ID]);
				Renderer->DrawSprite(texture, glm::vec2(104, 90 + 16 * i), 0.0f, glm::vec2(16));
			}
		}
		else
		{
			for (int i = 0; i < buyer->inventory.size(); i++)
			{
				auto item = buyer->inventory[i];
				text->RenderText(item->name, xPos, yPos + 43 * i + 1, 1);
				text->RenderTextRight(intToString(item->remainingUses), 575, yPos + 43 * i + 1, 1, 28);
				if (item->value > 0)
				{
					int halfValue = item->value * 0.5f;
					int useDecrease = ((halfValue / item->maxUses) / 10) * 10;
					int value = halfValue - useDecrease * (item->maxUses - item->remainingUses);
					text->RenderTextRight(intToString(value), 675, yPos + 43 * i + 1, 1, 56);
				}
				else
				{
					text->RenderTextRight("----", 675, yPos + 43 * i + 1, 1, 56);
				}

				ResourceManager::GetShader("Nsprite").Use();
				ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());
				auto texture = ResourceManager::GetTexture("icons");

				Renderer->setUVs(MenuManager::menuManager.itemIconUVs[item->ID]);
				Renderer->DrawSprite(texture, glm::vec2(104, 90 + 16 * i), 0.0f, glm::vec2(16));
			}
		}
		text->RenderTextRight(intToString(MenuManager::menuManager.playerManager->funds), 75, 551, 1, 56);
		text->RenderText("G", 175, 551, 1);

		if (!shopDelay && (state == BUYING || state == SELLING || state == CONFIRMING))
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
				text->RenderTextRight(intToString(weaponData.type), xStatValue, yPosition, 1, 28);
				yPosition += offSet;
				text->RenderText("Hit", xStatName, yPosition, 1);
				text->RenderTextRight(intToString(weaponData.hit), xStatValue, yPosition, 1, 28);
				yPosition += offSet;
				text->RenderText("Might", xStatName, yPosition, 1);
				text->RenderTextRight(intToString(weaponData.might), xStatValue, yPosition, 1, 28);
				yPosition += offSet;
				text->RenderText("Crit", xStatName, yPosition, 1);
				text->RenderTextRight(intToString(weaponData.crit), xStatValue, yPosition, 1, 28);
				yPosition += offSet;
				text->RenderText("Range", xStatName, yPosition, 1);
				text->RenderTextRight(intToString(weaponData.minRange), xStatValue, yPosition, 1, 28);
				yPosition += offSet;
				text->RenderText("Weight", xStatName, yPosition, 1);
				text->RenderTextRight(intToString(weaponData.weight), xStatValue, yPosition, 1, 28);
				yPosition += offSet;
			}
			else
			{
				text->RenderText(currentItem.description, 50, yPosition, 1);
			}
		}
	}
	if (fullFadeIn || fullFadeOut)
	{
		DrawFadeIn();
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
				//cancel sound
			}
		}
		ResourceManager::PlaySound("select2");
		shopDelay = true;
		ActivateText();
		break;
	case BUYING:
	{
		auto items = ItemManager::itemManager.items;
		//cancel sound plays if you cannot buy. There is another sound that plays when you do buy but I don't think I can isolate that one
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
		ResourceManager::PlaySound("select2");
		break;
	}
	case SELLING:
		if (buyer->inventory[currentOption]->value == 0)
		{
			textManager.init(11);
		}
		else
		{
			textManager.init(5);
			state = CONFIRMING;
			confirm = true;
		}
		ResourceManager::PlaySound("select2");
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
				auto buyerItem = buyer->inventory[currentOption];
				int halfValue = buyerItem->value * 0.5f;
				int useDecrease = ((halfValue / buyerItem->maxUses) / 10) * 10;
				int value = halfValue - useDecrease * (buyerItem->maxUses - buyerItem->remainingUses);
				MenuManager::menuManager.playerManager->funds += value;
				buyer->dropItem(currentOption);
				numberOfOptions--;
				if (numberOfOptions == 0)
				{
					state = GREETING;
					textManager.init(3);
					ActivateText();
				}
				else
				{
					textManager.init(12);
					if (currentOption == numberOfOptions)
					{
						currentOption--;
					}
					state = SELLING;
				}
			}
			ResourceManager::PlaySound("select2");

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
		break;
	}
	if (state != GREETING)
	{
		ActivateText();
	}
}

void VendorMenu::ActivateText()
{
	textManager.textObjects[2].displayedText = "";
	textManager.active = true;
	delay = true;
}

void VendorMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	MenuManager::menuManager.AnimateIndicator(deltaTime);
	if (fullFadeIn)
	{
		HandleFadeIn(deltaTime);

	}
	else if (fullFadeOut)
	{
		HandleFadeOut(deltaTime);

		if (exitMenu)
		{
			volumeModifier += deltaTime * 2;
		}
		else
		{
			volumeModifier -= deltaTime * 2;
			if (!fullFadeOut)
			{
				volumeModifier = 0.5f;
			}
		}
		Mix_VolumeMusic(128 * volumeModifier);
	}
	else
	{
		if (textManager.active)
		{
			textManager.Update(deltaTime, inputManager);
		}
		else if (delay)
		{
			delayTimer += deltaTime;
			if (delayTimer >= delayTime)
			{
				delayTimer = 0.0f;
				delay = false;
				shopDelay = false;
			}
		}
		else if (state == LEAVING)
		{
			exitMenu = true;
			fullFadeIn = true;
		}
		else
		{
			if (state == CONFIRMING)
			{
				if (inputManager.isKeyPressed(SDLK_RIGHT))
				{
					confirm = false;
					ResourceManager::PlaySound("optionSelect2");
				}
				else if (inputManager.isKeyPressed(SDLK_LEFT))
				{
					confirm = true;
					ResourceManager::PlaySound("optionSelect2");
				}
			}
			else if (state == BUYING || state == SELLING)
			{
				if (inputManager.KeyDownDelay(SDLK_UP))
				{
					if (inputManager.isKeyPressed(SDLK_UP))
					{
						PreviousOption();
					}
					else
					{
						PreviousOptionStop();
					}
				}
				else if (inputManager.KeyDownDelay(SDLK_DOWN))
				{
					if (inputManager.isKeyPressed(SDLK_DOWN))
					{
						NextOption();
					}
					else
					{
						NextOptionStop();
					}
				}
			}
			else if (state == GREETING)
			{
				if (inputManager.isKeyPressed(SDLK_RIGHT))
				{
					buying = false;
					numberOfOptions = buyer->inventory.size();
					ResourceManager::PlaySound("optionSelect2");
				}
				else if (inputManager.isKeyPressed(SDLK_LEFT))
				{
					buying = true;
					numberOfOptions = vendor->items.size();
					ResourceManager::PlaySound("optionSelect2");
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
}

void VendorMenu::CancelOption(int num)
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
		currentOption = 0;
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
	ResourceManager::PlaySound("cancel");
	ActivateText();
}

void VendorMenu::DelayedExit()
{
	if (MenuManager::menuManager.mustWait)
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
	else
	{
		MenuManager::menuManager.PreviousMenu();
		ResourceManager::PlaySound("select2");
	}
	Mix_VolumeMusic(128 * volumeModifier);
}

FullInventoryMenu::FullInventoryMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer, int newItem) :
	Menu(Cursor, Text, camera, shapeVAO, Renderer), newItem(newItem)
{
	numberOfOptions = cursor->selectedUnit->inventory.size() + 1;
	currentStats = cursor->selectedUnit->CalculateBattleStats();
	selectedStats = cursor->selectedUnit->CalculateBattleStats(cursor->selectedUnit->inventory[currentOption]->ID);
	itemIconUVs = MenuManager::menuManager.itemIconUVs;
	proficiencyIconUVs = MenuManager::menuManager.proficiencyIconUVs;
}

void FullInventoryMenu::Draw()
{
	DrawBox(glm::vec2(128, 0), 122, 66);
	DrawBox(glm::vec2(24, 72), 122, 146);
	DrawBox(glm::vec2(152, 72), 98, 130);

	//Selection
	ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	MenuManager::menuManager.DrawIndicator(glm::vec2(24, 81 + (16 * currentOption)));

	//Duplicated this down in ItemUseMenu's Draw.
	Unit* unit = cursor->selectedUnit;
	auto inventory = unit->inventory;
	glm::vec3 color = glm::vec3(1);
	glm::vec3 grey = glm::vec3(0.64f);

	for (int i = 0; i < numberOfOptions - 1; i++)
	{
		color = glm::vec3(1);
		int yPosition = 225 + i * 42;
		auto item = inventory[i];
		text->RenderText(item->name, 175, yPosition, 1, color);
		text->RenderTextRight(intToString(item->remainingUses), 375, 230 + i * 42, 1, 28, color);

		ResourceManager::GetShader("Nsprite").Use();
		auto texture = ResourceManager::GetTexture("icons");

		Renderer->setUVs(itemIconUVs[item->ID]);
		Renderer->DrawSprite(texture, glm::vec2(40, 82 + 16 * i), 0.0f, glm::vec2(16));
	}
	auto item = ItemManager::itemManager.items[newItem];
	text->RenderText(item.name, 175, 225 + (numberOfOptions - 1) * 42, 1, grey);
	text->RenderTextRight(intToString(item.remainingUses), 375, 230 + (numberOfOptions - 1) * 42, 1, 28, grey);

	ResourceManager::GetShader("Nsprite").Use();
	auto texture = ResourceManager::GetTexture("icons");

	Renderer->setUVs(itemIconUVs[item.ID]);
	Renderer->DrawSprite(texture, glm::vec2(40, 82 + 16 * (numberOfOptions - 1)), 0.0f, glm::vec2(16));
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
		int xStatValue = 600;
	
		ResourceManager::GetShader("Nsprite").Use();
		ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());
		auto texture = ResourceManager::GetTexture("icons");
		Renderer->setUVs(proficiencyIconUVs[weaponData.type]);
		Renderer->DrawSprite(texture, glm::vec2(193, 83), 0.0f, glm::vec2(16));

		text->RenderText("Type", xStatName, yPosition, 1);
		yPosition += offSet;
		text->RenderText("Atk", xStatName, yPosition, 1);
		text->RenderTextRight(intToString(selectedStats.attackDamage), xStatValue, yPosition, 1, 28);
		if (selectedStats.attackDamage > currentStats.attackDamage)
		{
			text->RenderTextRight("^^^", xStatValue + 60, yPosition, 1, 28);
		}
		else if (selectedStats.attackDamage < currentStats.attackDamage)
		{
			text->RenderTextRight("v v v", xStatValue + 60, yPosition, 1, 28);
		}
		yPosition += offSet;
		text->RenderText("Hit", xStatName, yPosition, 1);
		text->RenderTextRight(intToString(selectedStats.hitAccuracy), xStatValue, yPosition, 1, 28);
		if (selectedStats.hitAccuracy > currentStats.hitAccuracy)
		{
			text->RenderTextRight("^^^", xStatValue + 60, yPosition, 1, 28);
		}
		else if (selectedStats.hitAccuracy < currentStats.hitAccuracy)
		{
			text->RenderTextRight("v v v", xStatValue + 60, yPosition, 1, 28);
		}
		yPosition += offSet;
		text->RenderText("Crit", xStatName, yPosition, 1);
		text->RenderTextRight(intToString(selectedStats.hitCrit), xStatValue, yPosition, 1, 28);
		if (selectedStats.hitCrit > currentStats.hitCrit)
		{
			text->RenderTextRight("^^^", xStatValue + 60, yPosition, 1, 28);
		}
		else if (selectedStats.hitCrit < currentStats.hitCrit)
		{
			text->RenderTextRight("v v v", xStatValue + 60, yPosition, 1, 28);
		}
		yPosition += offSet;
		text->RenderText("Avo", xStatName, yPosition, 1);
		text->RenderTextRight(intToString(selectedStats.hitAvoid), xStatValue, yPosition, 1, 28);
		if (selectedStats.hitAvoid > currentStats.hitAvoid)
		{
			text->RenderTextRight("^^^", xStatValue + 60, yPosition, 1, 28);
		}
		else if (selectedStats.hitAvoid < currentStats.hitAvoid)
		{
			text->RenderTextRight("v v v", xStatValue + 60, yPosition, 1, 28);
		}
	}
	else
	{
		text->RenderText(focusedItem.description, 500, 225, 1);
	}

	text->RenderText("Too many items.\nPick an item to\nsend to Supply", 425, 32, 1);

	Texture2D portraitTexture = ResourceManager::GetTexture("Portraits");
	ResourceManager::GetShader("Nsprite").Use();
	ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());
	Renderer->setUVs(UnitResources::portraitUVs[unit->portraitID][0]);
	Renderer->DrawSprite(portraitTexture, glm::vec2(40, 8), 0, glm::vec2(48, 64), glm::vec4(1), true);
}

void FullInventoryMenu::SelectOption()
{
	if (currentOption < numberOfOptions - 1)
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
	if (inputManager.KeyDownDelay(SDLK_UP))
	{
		if (inputManager.isKeyPressed(SDLK_UP))
		{
			PreviousOption();
		}
		else
		{
			PreviousOptionStop();
		}
	}
	else if (inputManager.KeyDownDelay(SDLK_DOWN))
	{
		if (inputManager.isKeyPressed(SDLK_DOWN))
		{
			NextOption();
		}
		else
		{
			NextOptionStop();
		}
	}
	else if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
	MenuManager::menuManager.AnimateIndicator(deltaTime);
}

UnitMovement::UnitMovement(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer, Unit* movingUnit, Unit* receivingUnit, int operation, glm::ivec2 dropPosition) :
	Menu(Cursor, Text, camera, shapeVAO, Renderer), movingUnit(movingUnit), receivingUnit(receivingUnit), operation(operation), dropPosition(dropPosition)
{
	movingUnit->hasMoved = false;
	if (movingUnit->carryingUnit)
	{
		movingUnit->sprite.SetPosition(movingUnit->carryingUnit->sprite.getPosition());
	}
	auto playerUnit = cursor->selectedUnit;
	if (operation == RESCUE || operation == TRANSFER)
	{
		receivingUnit->carryUnit(movingUnit);
	}
	if (operation != TRANSFER)
	{
		if (receivingUnit == playerUnit)
		{
			receivingUnit->sprite.moveAnimate = false;
		}
	}
	if (operation == DROP || operation == RELEASE)
	{
		movingUnit->carryingUnit = nullptr;
		movingUnit->hasMoved = false;
		
		playerUnit->releaseUnit();
	//	movingUnit->sprite.SetPosition(playerUnit->sprite.getPosition());
		std::vector<glm::ivec2> path = { dropPosition, playerUnit->sprite.getPosition() };
		movingUnit->startMovement(path, 0, false);
	}
	if (!playerUnit->isMounted() || playerUnit->mount->remainingMoves == 0)
	{
		doneHere = true;
	}
}

void UnitMovement::Draw()
{
}

void UnitMovement::SelectOption()
{
}

void UnitMovement::CheckInput(InputManager& inputManager, float deltaTime)
{
	moveTimer += deltaTime;
	if (operation == RELEASE)
	{
		if (enemyFading)
		{
			if (moveTimer >= 0.02f)
			{
				movingUnit->hide = !movingUnit->hide;
				moveTimer = 0.0f;
			}
			removeTimer += deltaTime;
			if (removeTimer >= 0.5f)
			{
				auto playerUnit = cursor->selectedUnit;
				//Need to call the death message
				movingUnit->isDead = true;
				MenuManager::menuManager.unitDiedSubject.notify(movingUnit);
				if (playerUnit->isMounted() && playerUnit->mount->remainingMoves > 0)
				{
					cursor->GetRemainingMove();
					MenuManager::menuManager.mustWait = true;
				}
				else
				{
					cursor->Wait();
				}
				ClearMenu();
			}
		}
		else
		{
			if (moveTimer >= moveDelay)
			{
				movingUnit->UpdateMovement(deltaTime, inputManager);
				if (!movingUnit->movementComponent.moving)
				{
					enemyFading = true;
				}
			}
		}
	}
	else if (moveTimer >= moveDelay)
	{
		auto playerUnit = cursor->selectedUnit;
		movingUnit->UpdateMovement(deltaTime, inputManager);
		if (!movingUnit->movementComponent.moving)
		{
			movingUnit->sprite.moveAnimate = false;
			movingUnit->hasMoved = true;
			switch (operation)
			{
			case DROP:
				if (!doneHere)
				{
					cursor->GetRemainingMove();
					MenuManager::menuManager.mustWait = true;
				}
				else
				{
					cursor->Wait();
				}
				movingUnit->placeUnit(dropPosition.x, dropPosition.y);
				ClearMenu();
				break;
			case RESCUE:
				movingUnit->hide = true;
				if (!doneHere)
				{
					cursor->GetRemainingMove();
					MenuManager::menuManager.mustWait = true;
				}
				else
				{
					cursor->Wait();
				}
				ClearMenu();
				break;
			case TRANSFER:
				movingUnit->hide = true;
				auto playerUnit = cursor->selectedUnit;
				playerUnit->sprite.setFocus();
				MenuManager::menuManager.PreviousMenu();
				MenuManager::menuManager.PreviousMenu();
				MenuManager::menuManager.menus.back()->GetOptions();
				MenuManager::menuManager.mustWait = true;
				ResourceManager::PlaySound("select1");
				break;
			}
		}
	}
}

SuspendMenu::SuspendMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer) 
	: Menu(Cursor, Text, camera, shapeVAO, Renderer)
{
	numberOfOptions = 2;
	currentOption = 1;
	opening = true;
}

void SuspendMenu::Draw()
{
	if (!opening && !closing)
	{
		if (suspended)
		{
			DrawBox(glm::ivec2(72, 104), 106, 82);

			text->RenderText("So may resume\nthis chapter\nfrom the main\nmenu", 275, 310, 1);
		}
		else
		{
			DrawBox(glm::ivec2(72, 104), 109, 50);

			ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());
			MenuManager::menuManager.DrawIndicator(glm::vec2(87 + 32 * currentOption, 129));

			text->RenderText("Suspend the game?", 275, 310, 1);
			text->RenderText("Yes", 325, 353, 1);
			text->RenderText("No", 425, 353, 1);
		}

		Texture2D portraitTexture = ResourceManager::GetTexture("Portraits");
		ResourceManager::GetShader("Nsprite").Use();
		ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());
		Renderer->setUVs(UnitResources::portraitUVs[20][0]);
		Renderer->DrawSprite(portraitTexture, glm::vec2(104, 40), 0, glm::vec2(48, 64), glm::vec4(1), true);
	}
	if (returningToMenu)
	{
		ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
		ResourceManager::GetShader("shape").SetFloat("alpha", fadeOut);
		glm::mat4 model = glm::mat4();
		model = glm::translate(model, glm::vec3(0, 0, 0.0f));

		model = glm::scale(model, glm::vec3(256, 224, 0.0f));

		ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.0f, 0.0f));

		ResourceManager::GetShader("shape").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
	}
}

void SuspendMenu::SelectOption()
{
	if (currentOption == 0)
	{
		MenuManager::menuManager.suspendSubject.notify(0);
		suspended = true;
	}
	else
	{
		closing = true;
	}
	ResourceManager::PlaySound("select2");
}

void SuspendMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (returningToMenu)
	{
		menuReturnTimer += deltaTime;
		if (menuReturnTimer >= 1.0f)
		{
			menuReturnTimer = 1.0f;
			MenuManager::menuManager.suspendSubject.notify(1);
		}
		fadeOut = glm::mix(0.0f, 1.0f, menuReturnTimer);
	}
	if (opening)
	{
		delay += deltaTime;
		fadeValue += deltaTime * 0.525;
		if (delay >= 1.0f)
		{
			delay = 0.0f;
			opening = false;
			fadeValue = fadeValueMax;
		}
		ResourceManager::GetShader("instance").SetFloat("backgroundFade", fadeValue, true);
	}
	else if (closing)
	{
		delay += deltaTime;
		fadeValue -= deltaTime * 5.25f;
		if (delay >= 0.1f)
		{
			ClearMenu();
		}
		ResourceManager::GetShader("instance").SetFloat("backgroundFade", fadeValue, true);
	}
	else
	{
		if (suspended)
		{
			if (inputManager.isKeyPressed(SDLK_SPACE))
			{
				returningToMenu = true;
				Mix_HookMusicFinished(nullptr);
				Mix_FadeOutMusic(1000.0f);
			}
		}
		else
		{
			MenuManager::menuManager.AnimateIndicator(deltaTime);
			if (inputManager.isKeyPressed(SDLK_LEFT))
			{
				PreviousOption();
			}
			if (inputManager.isKeyPressed(SDLK_RIGHT))
			{
				NextOption();
			}
			else if (inputManager.isKeyPressed(SDLK_RETURN))
			{
				SelectOption();
			}
			else if (inputManager.isKeyPressed(SDLK_z))
			{
				closing = true;
				ResourceManager::PlaySound("cancel");
			}
		}
	}
}

TitleMenu::TitleMenu(Cursor* Cursor, TextRenderer* Text, Camera* camera, int shapeVAO, SpriteRenderer* Renderer, Subject<int>* subject, bool foundSuspend)
{
	//Don't actually want to call the base constructor with this one, because that would play a sound,
	// so have to initialize everything like this
	this->cursor = Cursor;
	this->text = Text;
	this->camera = camera;
	this->shapeVAO = shapeVAO;
	this->Renderer = Renderer;
	this->foundSuspend = foundSuspend;
	this->subject = subject;

	GetOptions();
}

void TitleMenu::Draw()
{
	ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	Renderer->setUVs();
	Texture2D displayTexture = ResourceManager::GetTexture("TitleButton");
	if (foundSuspend)
	{
		Renderer->DrawSprite(displayTexture, glm::vec2(72, 64), 0.0f, glm::vec2(112, 32));
		Renderer->DrawSprite(displayTexture, glm::vec2(72, 96), 0.0f, glm::vec2(112, 32));
		MenuManager::menuManager.DrawIndicator(glm::vec2(55, 73 + 32 * currentOption));
		text->RenderText("Resume Game", 325, 195, 1.0f);
		text->RenderText("New Game", 325, 283, 1.0f);
	}
	else
	{
		Renderer->DrawSprite(displayTexture, glm::vec2(72, 96), 0.0f, glm::vec2(112, 32));
		MenuManager::menuManager.DrawIndicator(glm::vec2(55, 105));

		text->RenderText("New Game", 325, 283, 1.0f);
	}
	if (fadingOut)
	{
		ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
		ResourceManager::GetShader("shape").SetFloat("alpha", fadeAlpha);
		glm::mat4 model = glm::mat4();
		model = glm::translate(model, glm::vec3(0, 0, 0.0f));

		model = glm::scale(model, glm::vec3(256, 224, 0.0f));

		ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.0f, 0.0f));

		ResourceManager::GetShader("shape").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
	}
}

void TitleMenu::SelectOption()
{
	fadingOut = true;
	ResourceManager::PlaySound("titleconfirm");
	Mix_HookMusicFinished(nullptr);
	Mix_FadeOutMusic(500.0f);
}

void TitleMenu::CancelOption(int num)
{
}

void TitleMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (fadingOut)
	{
		//30 frames/0.5s
		fadeAlpha += 0.0333f;
		if (fadeAlpha >= 1.0f)
		{
			fadeAlpha = 1.0f;
			if (foundSuspend)
			{
				if (currentOption == 0)
				{
					subject->notify(1);
				}
				else
				{
					subject->notify(0);
				}
			}
			else
			{
				subject->notify(0);
			}
			ClearMenu();
		}
	}
	else
	{
		MenuManager::menuManager.AnimateIndicator(deltaTime);

		if (inputManager.isKeyPressed(SDLK_UP))
		{
			currentOption--;
			if (currentOption < 0)
			{
				currentOption = 0;
			}
			else
			{
				ResourceManager::PlaySound("optionSelect1");

			}
		}
		else if (inputManager.isKeyPressed(SDLK_DOWN))
		{
			currentOption++;
			if (currentOption >= numberOfOptions)
			{
				currentOption = numberOfOptions - 1;
			}
			else
			{
				ResourceManager::PlaySound("optionSelect1");
			}
		}
		else if (inputManager.isKeyPressed(SDLK_RETURN))
		{
			SelectOption();
		}
	}
}

void TitleMenu::GetOptions()
{
	numberOfOptions = 1;
	if (foundSuspend)
	{
		numberOfOptions = 2;
	}
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

	auto uiTexture = ResourceManager::GetTexture("UIItems");
	proficiencyIconUVs = ResourceManager::GetTexture("icons").GetUVs(0, 0, 16, 16, 10, 1);
	itemIconUVs = ResourceManager::GetTexture("icons").GetUVs(0, 16, 16, 16, 10, 2, 19);
	skillIconUVs = ResourceManager::GetTexture("icons").GetUVs(0, 48, 16, 16, 6, 1, 6);
	optionIconUVs = uiTexture.GetUVs(0, 0, 16, 16, 4, 3, 11);
	arrowAnimUVs = uiTexture.GetUVs(0, 48, 7, 6, 6, 1);
	indicatorUV = uiTexture.GetUVs(48, 32, 16, 16, 1, 1)[0];
	arrowUV = uiTexture.GetUVs(54, 54, 8, 8, 1, 1);
	carryingIconsUVs = uiTexture.GetUVs(32, 54, 8, 8, 2, 1);
	statBarUV = uiTexture.GetUVs(0, 54, 7, 7, 1, 1)[0];
	skillHighlightUVs = uiTexture.GetUVs(32, 96, 16, 16, 4, 1);

	malusArrowUV = uiTexture.GetUVs(16, 54, 8, 10, 1, 1)[0];

	colorBarsUV = uiTexture.GetUVs(196, 196, 107, 56, 1, 1)[0];
	colorIndicatorUV = uiTexture.GetUVs(50, 54, 3, 8, 1, 1)[0];

	auto boxesTexture = ResourceManager::GetTexture("UIStuff");
	boxesUVs = boxesTexture.GetUVs(0, 0, 32, 32, 3, 1);

	auto asdgasd = ResourceManager::GetTexture("testpattern");
	patternUVs = ResourceManager::GetTexture("testpattern").GetUVs(0, 0, 64, 32, 2, 1);

	arrowSprite.setSize(glm::vec2(7, 6));
	arrowSprite.uv = &arrowAnimUVs;
}

void MenuManager::AddMenu(int ID)
{
	//I don't know if this ID system will stick around
	if (ID == 0)
	{
		Menu* newMenu = new UnitOptionsMenu(cursor, text, camera, shapeVAO, renderer);
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
		Menu* newMenu = new ItemUseMenu(cursor, text, camera, shapeVAO, renderer, cursor->selectedUnit->inventory[currentOption], currentOption);
		menus.push_back(newMenu);
	}
	else if (ID == 3)
	{
		Menu* newMenu = new ExtraMenu(cursor, text, camera, shapeVAO, renderer);
		menus.push_back(newMenu);
	}
	else if (ID == 4)
	{
		Menu* newMenu = new CantoOptionsMenu(cursor, text, camera, shapeVAO, renderer);
		menus.push_back(newMenu);
	}
}

void MenuManager::AddUnitStatMenu(Unit* unit)
{
	Menu* newMenu = new UnitStatsViewMenu(cursor, text, camera, shapeVAO, renderer, unit);
	menus.push_back(newMenu);
}

void MenuManager::AddFullInventoryMenu(int itemID)
{
	Menu* newMenu = new FullInventoryMenu(cursor, text, camera, shapeVAO, renderer, itemID);
	MenuManager::menuManager.menus.push_back(newMenu);
}

void MenuManager::AddTitleMenu(Subject<int>* subject, bool foundSuspend)
{
	Menu* newMenu = new TitleMenu(cursor, text, camera, shapeVAO, renderer, subject, foundSuspend);
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
	mountActionTaken = false;
	while (menus.size() > 0)
	{
		PreviousMenu();
	}
}

void MenuManager::AnimateArrow(float deltaTime)
{
	arrowSprite.playAnimation(deltaTime, 6, false);
}

void MenuManager::DrawArrow(glm::ivec2 position, bool down)
{
	renderer->setUVs(arrowSprite.getUV());
	Texture2D texture = ResourceManager::GetTexture("UIItems");
	renderer->DrawSprite(texture, position, 0, arrowSprite.getSize(), glm::vec4(1), false, !down);
}

void MenuManager::AnimateIndicator(float deltaTime)
{
	indicatorDrawX = glm::mix(0, 4, pow(sin(indicatorT), 2));
	indicatorT += 6.5f * deltaTime;
	indicatorT = fmod(indicatorT, glm::pi<float>());
}

void MenuManager::DrawIndicator(glm::ivec2 position, bool animated, float rot)
{
	renderer->setUVs(indicatorUV);
	Texture2D texture = ResourceManager::GetTexture("UIItems");
	if (animated)
	{
		if (rot > 0)
		{
			position.y -= indicatorDrawX; //Not very robust but I only need this in one place
		}
		else
		{
			position.x -= indicatorDrawX;
		}
	}
	renderer->DrawSprite(texture, position, rot, glm::vec2(16));
}

void MenuManager::AnimateArrowIndicator(float deltaTime)
{
	indicatorArrowDrawX = glm::mix(0, 4, pow(sin(indicatorArrowT), 2));
	indicatorArrowT += 5.5f * deltaTime;
	indicatorArrowT = fmod(indicatorArrowT, glm::pi<float>());
}

void MenuManager::DrawArrowIndicator(glm::ivec2 position)
{
	renderer->setUVs(arrowUV[0]);
	Texture2D texture = ResourceManager::GetTexture("UIItems");
	position.y -= indicatorArrowDrawX; 

	renderer->DrawSprite(texture, position, 0, glm::vec2(8));
}

Menu* MenuManager::GetCurrent()
{
	if (menus.size() >= 1)
	{
		return menus.back();
	}
	return nullptr;
}

Menu* MenuManager::GetPrevious()
{
	if (menus.size() >= 2)
	{
		return menus[menus.size() - 2];
	}
	return nullptr;
}