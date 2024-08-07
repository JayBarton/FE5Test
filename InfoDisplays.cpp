#include "InfoDisplays.h"
#include "Unit.h"
#include "ResourceManager.h"
#include "TextRenderer.h"
#include "Globals.h"
#include "Camera.h"
#include "InputManager.h"
#include "SpriteRenderer.h"
#include <glm.hpp>
#include <SDL.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include "Settings.h"
#include "UnitResources.h"

using json = nlohmann::json;

#include "Items.h"
#include "EnemyManager.h"

void InfoDisplays::init(TextObjectManager* textManager)
{
	this->textManager = textManager;
}

void InfoDisplays::AddExperience(Unit* unit, Unit* foe, glm::vec2 levelUpPosition)
{
	if (unit->isDead)
	{
		focusedUnit = nullptr;
		state = NONE;
		endBattle.notify(0);
	}
	//Need to handle if the player unit was captured
	else if (unit->carryingUnit)
	{
		focusedUnit = nullptr;
		state = NONE;
		endBattle.notify(4);
	}
	else
	{
		if (foe->carryingUnit)
		{
			capturing = true;
		}
		focusedUnit = unit;

		gainedExperience = unit->CalculateExperience(foe);
		focusedUnit = unit;
		displayedExperience = focusedUnit->experience;
		finalExperience = gainedExperience + displayedExperience;
		if (finalExperience >= 100)
		{
			finalExperience -= 100;
		}
		state = ADD_EXPERIENCE;
		if (Settings::settings.mapAnimations == 0 || Settings::settings.mapAnimations == 2 && unit->battleAnimations)
		{
			this->levelUpPosition = levelUpPosition - glm::vec2(0, 16);
			battleDisplay = true;
		}
		else
		{
			this->levelUpPosition = focusedUnit->sprite.getPosition();
			battleDisplay = false;
		}
	}
}

void InfoDisplays::OnUnitLevel(Unit* unit)
{
	focusedUnit = unit;
	preLevelStats = new StatGrowths{ focusedUnit->maxHP, focusedUnit->strength, focusedUnit->magic,
		focusedUnit->skill, focusedUnit->speed, focusedUnit->luck,focusedUnit->defense,focusedUnit->build,focusedUnit->move };
	state = LEVEL_UP_NOTE;
	ResourceManager::PlaySound("levelUp");
	if (!battleDisplay)
	{
		Mix_VolumeMusic(128 * 0.5f);
	}
}

void InfoDisplays::StartUse(Unit* unit, int index, Camera* camera)
{
	//When I have other usable items, can check/pass in what type here to determine what to do.
	usedItem = true;
	if (index == 0)
	{
		StartUnitHeal(unit, unit->maxHP, camera);
	}
	else if (index == 1)
	{
		StartUnitStatBoost(unit, camera);
	}
}

void InfoDisplays::StartUnitStatBoost(Unit* unit, Camera* camera)
{
	statDelay = true;
	focusedUnit = unit;
	preLevelStats = new StatGrowths{ focusedUnit->maxHP, focusedUnit->strength, focusedUnit->magic,
		focusedUnit->skill, focusedUnit->speed, focusedUnit->luck,focusedUnit->defense,focusedUnit->build,focusedUnit->move };
	camera->SetCenter(focusedUnit->sprite.getPosition());
	state = STAT_BOOST;
}

void InfoDisplays::EnemyUse(Unit* unit, int index)
{
	state = ENEMY_USE;
	focusedUnit = unit;
	itemToUse = index;
}

void InfoDisplays::EnemyTrade(EnemyManager* enemyManager)
{
	state = ENEMY_TRADE;
	this->enemyManager = enemyManager;
	focusedUnit = enemyManager->units[enemyManager->currentEnemy];
	itemToUse = enemyManager->healIndex;
}

void InfoDisplays::EnemyBuy(EnemyManager* enemyManager)
{
	state = ENEMY_BUY;
	this->enemyManager = enemyManager;
	focusedUnit = enemyManager->units[enemyManager->currentEnemy];
}

void InfoDisplays::GetItem(int itemID)
{
	itemToUse = itemID;
	state = GOT_ITEM;
	ResourceManager::PlaySound("getItem");
	Mix_VolumeMusic(128 * 0.5f);
}

//Call this from StartUse
void InfoDisplays::StartUnitHeal(Unit* unit, int healAmount, Camera* camera)
{
	state = HEALING_ANIMATION;
	healDelay = true;
	focusedUnit = unit;
	displayedHP = focusedUnit->currentHP;
	healedHP = healAmount;
	camera->SetCenter(focusedUnit->sprite.getPosition());
}

void InfoDisplays::ChangeTurn(int currentTurn)
{
	turn = currentTurn;
	state = TURN_CHANGE;
	turnChangeStart = true;
	turnDisplayAlpha = 0.0f;
	turnTextX = -100;
	displayTimer = 0.0f;
	playTurnChange = true;
}

void InfoDisplays::PlayerUnitDied(Unit* unit)
{
	textManager->textLines.clear();
	textManager->textLines.push_back(SpeakerText{ nullptr, 1, unit->deathMessage, unit->portraitID });
	textManager->textObjects[1].fadeIn = true;
/*	testText.position = glm::vec2(62, 455);
	testText.portraitPosition = glm::vec2(176, 96);
	testText.displayedPosition = testText.position;
	testText.charsPerLine = 55;
	testText.nextIndex = 55;
	testText.fadeIn = true;

	textManager->textObjects.clear();
	textManager->textObjects.push_back(testText);*/
	textManager->init();
	textManager->active = true;
	textManager->talkActivated = true;

	state = PLAYER_DIED;
}

void InfoDisplays::PlayerLost(int messageID)
{
	textManager->textLines.clear();

	std::ifstream f("Levels/GameOverDialogues.json");
	json data = json::parse(f);
	json texts = data["text"];
	for (const auto& text : texts)
	{
		int ID = text["ID"];
		if (ID == messageID)
		{
			auto dialogues = text["dialogue"];
			for (const auto& dialogue : dialogues)
			{
				textManager->textLines.push_back(SpeakerText{ nullptr, dialogue["location"], dialogue["speech"], 1 }); //This should vary on the level/loss condition
			}
		}
	}
	textManager->textObjects[1].fadeIn = true;
	//This is repeated from up above in player death, need to fix this shit
/*	testText.position = glm::vec2(62, 455);
	testText.portraitPosition = glm::vec2(176, 96);
	testText.displayedPosition = testText.position;
	testText.charsPerLine = 55;
	testText.nextIndex = 55;
	testText.fadeIn = true;

	textManager->textObjects.clear();
	textManager->textObjects.push_back(testText);*/
	textManager->init();
	textManager->active = true;
	textManager->talkActivated = true;
	state = PLAYER_DIED;
}

void InfoDisplays::UnitEscaped(EnemyManager* enemyManager)
{
	this->enemyManager = enemyManager;
	state = UNIT_ESCAPED;
}

void InfoDisplays::Update(float deltaTime, InputManager& inputManager)
{
	if (state != NONE)
	{
		displayTimer += deltaTime;
	}
	switch (state)
	{
	case ADD_EXPERIENCE:
		UpdateExperienceDisplay(deltaTime);
		break;
	case LEVEL_UP_NOTE:
		if (displayTimer > levelUpNoteTime)
		{
			displayTimer = 0.0f;
			if (battleDisplay)
			{
				Mix_FadeInMusic(ResourceManager::Music["LevelUpTheme"], -1, 500);
				state = BATTLE_FADE_THING;
			}
			else
			{
				state = MAP_LEVEL_UP;
				ResourceManager::PlaySound("pointUp");
				Mix_VolumeMusic(128);
			}
		}
		break;
	case MAP_LEVEL_UP:
	case BATTLE_LEVEL_UP:
		UpdateLevelUpDisplay(deltaTime);
		break;
	case BATTLE_FADE_THING:
		levelUpBlockAlpha += deltaTime;
		if (levelUpBlockAlpha >= 1)
		{
			levelUpBlockAlpha = 1;
			state = LEVEL_UP_BLOCK;
			displayTimer = 0;
		}
		break;
	case LEVEL_UP_BLOCK:
		levelUpBlockAlpha -= deltaTime;
		if (levelUpBlockAlpha <= 0)
		{
			levelUpBlockAlpha = 0;
			state = BATTLE_LEVEL_UP;
			displayTimer = 0;
		}
		break;
	case HEALING_ANIMATION:
		if (healDelay)
		{
			if (displayTimer > healDelayTime)
			{
				healDelay = false;
				displayTimer = 0.0f;
				ResourceManager::PlaySound("heal");
			}
		}
		else if (displayTimer > healAnimationTime)
		{
			displayTimer = 0.0f;
			state = HEALING_BAR;
		}
		break;
	case HEALING_BAR:
		UpdateHealthBarDisplay(deltaTime);
		break;
	case ENEMY_USE:
		if (displayTimer > textDisplayTime)
		{
			displayTimer = 0.0f;
			ItemManager::itemManager.UseItem(focusedUnit, itemToUse);
		}
	case ENEMY_TRADE:
		if (displayTimer > textDisplayTime)
		{
			displayTimer = 0.0f;
			state = ENEMY_USE;
		}
		break;
	case ENEMY_BUY:
		if (displayTimer > textDisplayTime)
		{
			displayTimer = 0.0f;
			enemyManager->FinishMove();
			state = NONE;
		}
		break;
	case GOT_ITEM:
		if (displayTimer > textDisplayTime)
		{
			Mix_VolumeMusic(128);
			displayTimer = 0.0f;
			state = NONE;
		}
		break;
	case TURN_CHANGE:
		TurnChangeUpdate(inputManager, deltaTime);
		break;
	case PLAYER_DIED:
		if (!textManager->active)
		{
			state = NONE;
		}
		break;
	case UNIT_ESCAPED:
		if (displayTimer > textDisplayTime)
		{
			displayTimer = 0.0f;
			enemyManager->UnitLeaveMap();
			state = NONE;
		}
		else
		{
			enemyManager->GetCurrentUnit()->UpdateMovement(deltaTime, inputManager);
		}
		break;
	case STAT_BOOST:
		if (statDelay)
		{
			if (displayTimer >= statDisplayTime)
			{
				displayTimer = 0;
				statDelay = false;
				ResourceManager::PlaySound("pointUp");
			}
		}
		else
		{
			if (inputManager.isKeyPressed(SDLK_RETURN))
			{
				displayTimer = statViewTime;
			}
			if (displayTimer >= statViewTime)
			{
				//Only play units can use stat boosting items...For now.
				endBattle.notify(1);
				state = NONE;
			}
		}
		break;
	}
}

void InfoDisplays::TurnChangeUpdate(InputManager& inputManager, float deltaTime)
{
	if (playTurnChange)
	{
		ResourceManager::PlaySound("turnEnd");
		playTurnChange = false;
	}
	if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		state = NONE;
		displayTimer = 0.0f;
		ResourceManager::GetShader("instance").Use().SetFloat("subtractValue", 0);
		ResourceManager::GetShader("NSprite").Use().SetFloat("subtractValue", 0);
		ResourceManager::GetShader("sprite").Use().SetFloat("subtractValue", 0);
		ResourceManager::StopSound(-1);
	}
	else
	{
		if (turnChangeStart)
		{
			turnDisplayAlpha += 186 * deltaTime;
			if (turnDisplayAlpha >= turnDisplayMaxAlpha)
			{
				turnDisplayAlpha = turnDisplayMaxAlpha;
			}
			turnTextX += deltaTime + 30;
			if (turnTextX >= turnTextXFinal)
			{
				turnTextX = turnTextXFinal;
			}
		}
		else
		{
			turnDisplayAlpha -= 186 * deltaTime;
			if (turnDisplayAlpha <= 0)
			{
				turnDisplayAlpha = 0;
			}
		}
		if (displayTimer > turnDisplayTime)
		{
			displayTimer = 0.0f;
			if (turnChangeStart)
			{
				turnChangeStart = false;
			}
			else
			{
				state = NONE;
			}
		}
	}
	if (state == NONE)
	{
		endTurn.notify();
	}
}

void InfoDisplays::UpdateHealthBarDisplay(float deltaTime)
{
	if (finishedHealing)
	{
		if (displayTimer > healAnimationTime)
		{
			focusedUnit->currentHP = healedHP;
			finishedHealing = false;
			displayTimer = 0;
			state = NONE;
			if (usedItem)
			{
				usedItem = false;
				//I don't know about this gravy...
				if (focusedUnit->team == 0)
				{
					endBattle.notify(1);
				}
				else
				{
					endBattle.notify(2);
				}
			}
			else
			{
				endBattle.notify(3);
			}
		}
	}
	else
	{
		if (displayTimer > healDisplayTime)
		{
			displayedHP++;
			if (displayedHP >= healedHP)
			{
				displayedHP = healedHP;
				finishedHealing = true;
			}
			ResourceManager::PlaySound("healthbar", 1);
			displayTimer = 0;
		}
	}
}

void InfoDisplays::UpdateLevelUpDisplay(float deltaTime)
{
	if (displayTimer > 2.0f)
	{
		displayTimer = 0;
		if (battleDisplay)
		{
			state = BATTLE_LEVEL_DELAY;
		}
		else
		{
			ClearLevelUpDisplay();
		}
		if (capturing)
		{
			endBattle.notify(4);
			capturing = false;
		}
		else
		{
			endBattle.notify(0);
		}
	}
}

void InfoDisplays::ClearLevelUpDisplay()
{
	delete preLevelStats;
	focusedUnit = nullptr;
	state = NONE;
}

void InfoDisplays::UpdateExperienceDisplay(float deltaTime)
{
	if (displayingExperience)
	{
		if (displayTimer > experienceDisplayTime)
		{
			focusedUnit->AddExperience(gainedExperience);
			displayingExperience = false;
			displayTimer = 0;
			if (state == ADD_EXPERIENCE)
			{
				focusedUnit = nullptr;
				state = NONE;
				if (capturing)
				{
					endBattle.notify(4);
					capturing = false;
				}
				else
				{
					endBattle.notify(0);
				}
			}
		}
	}
	else
	{
		if (displayTimer > experienceTime)
		{
			displayedExperience++;
			if (displayedExperience >= 100)
			{
				displayedExperience -= 100;
			}
			if (displayedExperience == finalExperience)
			{
				displayingExperience = true;
				//Gotta do a double check on this to make sure the music stops properly
				if (battleDisplay && focusedUnit->experience + gainedExperience >= 100)
				{
					Mix_HookMusicFinished(nullptr);
					Mix_FadeOutMusic(500);
				}
			}
			displayTimer = 0;
		}
	}
}

void InfoDisplays::Draw(Camera* camera, TextRenderer* Text, int shapeVAO, SpriteRenderer* renderer)
{
	switch (state)
	{
	case NONE:
		break;
	case ADD_EXPERIENCE:
		DrawExperienceDisplay(camera, shapeVAO, Text, renderer);
		break;
	case LEVEL_UP_NOTE:
	{
		glm::vec2 drawPosition = levelUpPosition;
		if (battleDisplay)
		{
			drawPosition = glm::vec2(drawPosition.x / 256.0f * 800, drawPosition.y / 224.0f * 600);
			DrawBattleExperience(camera, shapeVAO, Text, renderer);
		}
		else
		{
			drawPosition = camera->worldToRealScreen(drawPosition, SCREEN_WIDTH, SCREEN_HEIGHT);
		}
		Text->RenderText("LEVEL UP", drawPosition.x, drawPosition.y, 0.85f, glm::vec3(0.95f, 0.95f, 0.0f));
		break;
	}
	case MAP_LEVEL_UP:
		DrawLevelUpDisplay(camera, shapeVAO, Text);
		break;
	case BATTLE_LEVEL_UP:
	case BATTLE_LEVEL_DELAY:
	case LEVEL_UP_BLOCK:
		DrawBattleLevelUpDisplay(camera, shapeVAO, Text, renderer);
		break;
	case BATTLE_FADE_THING:
	{
		DrawBattleExperience(camera, shapeVAO, Text, renderer);
		ResourceManager::GetShader("shape").Use().SetFloat("alpha", levelUpBlockAlpha);
		glm::mat4 model = glm::mat4();
		model = glm::translate(model, glm::vec3(0, 127, 0.0f));

		model = glm::scale(model, glm::vec3(256, 80, 0.0f));

		ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.0f, 0.0f));

		ResourceManager::GetShader("shape").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
		break;
	}
	case HEALING_ANIMATION:
		DrawHealAnimation(camera, shapeVAO);
		break;
	case HEALING_BAR:
		DrawHealthBar(camera, shapeVAO, Text);
		break;
	case ENEMY_USE:
		Text->RenderText(focusedUnit->name + " used " + focusedUnit->inventory[itemToUse]->name, 300, 300, 1);
		break;
	case ENEMY_TRADE:
		Text->RenderText(focusedUnit->name + " recieved " + focusedUnit->inventory[itemToUse]->name, 300, 300, 1);
		break;
	case ENEMY_BUY:
		Text->RenderText(focusedUnit->name + " bought " + focusedUnit->GetEquippedItem()->name, 300, 300, 1);
		break;
	case GOT_ITEM:
	{
		Text->RenderText(ItemManager::itemManager.items[itemToUse].name, 325, 289, 1);
		ResourceManager::GetShader("Nsprite").Use();
		ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());
		auto texture = ResourceManager::GetTexture("icons");

		renderer->setUVs(MenuManager::menuManager.itemIconUVs[itemToUse]);
		renderer->DrawSprite(texture, glm::vec2(88, 106), 0.0f, glm::ivec2(16));
		break;
	}
	case TURN_CHANGE:
	{
		ResourceManager::GetShader("instance").Use().SetFloat("subtractValue", turnDisplayAlpha);
		ResourceManager::GetShader("NSprite").Use().SetFloat("subtractValue", turnDisplayAlpha);
		ResourceManager::GetShader("sprite").Use().SetFloat("subtractValue", turnDisplayAlpha);
		std::string thisTurn;
		if (turn == 0)
		{
			thisTurn = "Player Turn";
		}
		else
		{
			thisTurn = "Enemy Turn";
		}
		//Using text now, I think I'll use a sprite ultimately
		Text->RenderText(thisTurn, turnTextX, 300, 1);
		break;
	}
	case UNIT_ESCAPED:
	{
		ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
		ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
		glm::mat4 model = glm::mat4();
		model = glm::translate(model, glm::vec3(96, 96, 0.0f));
		model = glm::scale(model, glm::vec3(66, 34, 0.0f));

		ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.5f, 1.0f));

		ResourceManager::GetShader("shape").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		Text->RenderText("Unit Escaped", 325, 289, 1);
		break;
	}
	case STAT_BOOST:
	{
		if (!statDelay)
		{
			int x = SCREEN_WIDTH * 0.5f;
			int y = SCREEN_HEIGHT * 0.5f;
			ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
			ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
			glm::mat4 model = glm::mat4();
			model = glm::translate(model, glm::vec3(80, 96, 0.0f));

			model = glm::scale(model, glm::vec3(100, 50, 0.0f));

			ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.5f, 1.0f));

			ResourceManager::GetShader("shape").SetMatrix4("model", model);
			glBindVertexArray(shapeVAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			glBindVertexArray(0);

			Text->RenderTextRight("HP", x - 130, y - 30, 1, 25);
			Text->RenderTextRight("STR", x - 130, y - 5, 1, 25);
			Text->RenderTextRight("MAG", x - 130, y + 20, 1, 25);
			Text->RenderTextRight("SKL", x - 130, y + 45, 1, 25);
			Text->RenderTextRight("SPD", x - 10, y - 30, 1, 25);
			Text->RenderTextRight("LCK", x - 10, y - 5, 1, 25);
			Text->RenderTextRight("DEF", x - 10, y + 20, 1, 25);
			Text->RenderTextRight("BLD", x - 10, y + 45, 1, 25);

			auto unit = focusedUnit;
			Text->RenderTextRight(intToString(preLevelStats->maxHP), x - 90, y - 30, 1, 14);
			if (unit->maxHP > preLevelStats->maxHP)
			{
				Text->RenderText(intToString(unit->maxHP - preLevelStats->maxHP), x - 65, y - 30, 1);
			}
			Text->RenderTextRight(intToString(preLevelStats->strength), x - 90, y - 5, 1, 14);
			if (unit->strength > preLevelStats->strength)
			{
				Text->RenderText(intToString(unit->strength - preLevelStats->strength), x - 65, y - 5, 1);
			}
			Text->RenderTextRight(intToString(preLevelStats->magic), x - 90, y + 20, 1, 14);
			if (unit->magic > preLevelStats->magic)
			{
				Text->RenderText(intToString(unit->magic - preLevelStats->magic), x - 65, y + 20, 1);
			}
			Text->RenderTextRight(intToString(preLevelStats->skill), x - 90, y + 45, 1, 14);
			if (unit->skill > preLevelStats->skill)
			{
				Text->RenderText(intToString(unit->skill - preLevelStats->skill), x - 65, y + 45, 1);
			}
			Text->RenderTextRight(intToString(preLevelStats->speed), x + 30, y - 30, 1, 14);
			if (unit->speed > preLevelStats->speed)
			{
				Text->RenderText(intToString(unit->speed - preLevelStats->speed), x + 55, y - 30, 1);
			}
			Text->RenderTextRight(intToString(preLevelStats->luck), x + 30, y - 5, 1, 14);
			if (unit->luck > preLevelStats->luck)
			{
				Text->RenderText(intToString(unit->luck - preLevelStats->luck), x + 55, y - 5, 1);
			}
			Text->RenderTextRight(intToString(preLevelStats->defense), x + 30, y + 20, 1, 14);
			if (unit->defense > preLevelStats->defense)
			{
				Text->RenderText(intToString(unit->defense - preLevelStats->defense), x + 55, y + 20, 1);
			}
			Text->RenderTextRight(intToString(preLevelStats->build), x + 30, y + 45, 1, 14);
			if (unit->build > preLevelStats->build)
			{
				Text->RenderText(intToString(unit->build - preLevelStats->build), x + 55, y + 45, 1);
			}
		}
		break;
	}
	}
}

void InfoDisplays::DrawHealthBar(Camera* camera, int shapeVAO, TextRenderer* Text)
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(80, 98, 0.0f));
	model = glm::scale(model, glm::vec3(100 * (float(displayedHP) / float(focusedUnit->maxHP)), 5, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.5f, 1.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	Text->RenderText("HP", 215, 260, 1);
	Text->RenderText(intToString(displayedHP), 565, 260, 1);
}

void InfoDisplays::DrawHealAnimation(Camera* camera, int shapeVAO)
{
	if (!healDelay)
	{
		ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getCameraMatrix());
		ResourceManager::GetShader("shape").SetFloat("alpha", 0.5f);
		glm::mat4 model = glm::mat4();
		auto unitPosition = focusedUnit->sprite.getPosition();
		model = glm::translate(model, glm::vec3(unitPosition.x, unitPosition.y, 0.0f));

		model = glm::scale(model, glm::vec3(16, 16, 0.0f));

		ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.5f, 1.0f));

		ResourceManager::GetShader("shape").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
	}
}

void InfoDisplays::DrawLevelUpDisplay(Camera* camera, int shapeVAO, TextRenderer* Text)
{
	int x = SCREEN_WIDTH * 0.5f;
	int y = SCREEN_HEIGHT * 0.5f;
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(80, 96, 0.0f));

	model = glm::scale(model, glm::vec3(100, 50, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.5f, 1.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	Text->RenderTextRight("HP", x - 130, y - 30, 1, 25);
	Text->RenderTextRight("STR", x - 130, y - 5, 1, 25);
	Text->RenderTextRight("MAG", x - 130, y + 20, 1, 25);
	Text->RenderTextRight("SKL", x - 130, y + 45, 1, 25);
	Text->RenderTextRight("SPD", x - 10, y - 30, 1, 25);
	Text->RenderTextRight("LCK", x - 10, y - 5, 1, 25);
	Text->RenderTextRight("DEF", x - 10, y + 20, 1, 25);
	Text->RenderTextRight("BLD", x - 10, y + 45, 1, 25);

	auto unit = focusedUnit;
	Text->RenderTextRight(intToString(preLevelStats->maxHP), x - 90, y - 30, 1, 14);
	if (unit->maxHP > preLevelStats->maxHP)
	{
		Text->RenderText(intToString(1), x - 65, y - 30, 1);
	}
	Text->RenderTextRight(intToString(preLevelStats->strength), x - 90, y - 5, 1, 14);
	if (unit->strength > preLevelStats->strength)
	{
		Text->RenderText(intToString(1), x - 65, y - 5, 1);
	}
	Text->RenderTextRight(intToString(preLevelStats->magic), x - 90, y + 20, 1, 14);
	if (unit->magic > preLevelStats->magic)
	{
		Text->RenderText(intToString(1), x - 65, y + 20, 1);
	}
	Text->RenderTextRight(intToString(preLevelStats->skill), x - 90, y + 45, 1, 14);
	if (unit->skill > preLevelStats->skill)
	{
		Text->RenderText(intToString(1), x - 65, y + 45, 1);
	}
	Text->RenderTextRight(intToString(preLevelStats->speed), x + 30, y - 30, 1, 14);
	if (unit->speed > preLevelStats->speed)
	{
		Text->RenderText(intToString(1), x + 55, y - 30, 1);
	}
	Text->RenderTextRight(intToString(preLevelStats->luck), x + 30, y - 5, 1, 14);
	if (unit->luck > preLevelStats->luck)
	{
		Text->RenderText(intToString(1), x + 55, y - 5, 1);
	}
	Text->RenderTextRight(intToString(preLevelStats->defense), x + 30, y + 20, 1, 14);
	if (unit->defense > preLevelStats->defense)
	{
		Text->RenderText(intToString(1), x + 55, y + 20, 1);
	}
	Text->RenderTextRight(intToString(preLevelStats->build), x + 30, y + 45, 1, 14);
	if (unit->build > preLevelStats->build)
	{
		Text->RenderText(intToString(1), x + 55, y + 45, 1);
	}
}

void InfoDisplays::DrawBattleLevelUpDisplay(Camera* camera, int shapeVAO, TextRenderer* Text, SpriteRenderer* renderer)
{
	auto unit = focusedUnit;

	ResourceManager::GetShader("Nsprite").Use();
	ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());

	Texture2D texture = ResourceManager::GetTexture("BattleLevelBackground");
	renderer->setUVs();
	renderer->DrawSprite(texture, glm::vec2(9, 129), 0, glm::vec2(238, 78));

	texture = ResourceManager::GetTexture("Portraits");
	renderer->setUVs(UnitResources::portraitUVs[unit->portraitID][0]);
	renderer->DrawSprite(texture, glm::vec2(16, 136), 0, glm::vec2(48, 64), glm::vec4(1), true);

	Text->RenderText(unit->name, 275, 375, 1);
	Text->RenderText("LV", 500, 377, 1);
	Text->RenderTextRight(intToString(unit->level), 550, 375, 1, 14);

	int xString1 = 225;
	int xString2 = 500;
	int y = 450;
	int x1 = 400;
	int x2 = 675;
	Text->RenderText("MHP", xString1, y, 1);
	y += 21;
	Text->RenderText("STR", xString1, y, 1);
	y += 21;
	Text->RenderText("MAG", xString1, y, 1);
	y += 21;
	Text->RenderText("SKL", xString1, y, 1);
	y = 450;
	Text->RenderText("SPD", xString2, y, 1);
	y += 21;
	Text->RenderText("LCK", xString2, y, 1);
	y += 21;
	Text->RenderText("DEF", xString2, y, 1);
	y += 21;
	Text->RenderText("BLD", xString2, y , 1);
	y = 450;
	Text->RenderTextRight(intToString(preLevelStats->maxHP), x1, y, 1, 14);
	if (unit->maxHP > preLevelStats->maxHP)
	{
	//	Text->RenderText(intToString(1), x - 65, y - 30, 1);
	}
	y += 21;
	Text->RenderTextRight(intToString(preLevelStats->strength), x1, y, 1, 14);
	if (unit->strength > preLevelStats->strength)
	{
	//	Text->RenderText(intToString(1), x - 65, y - 5, 1);
	}
	y += 21;
	Text->RenderTextRight(intToString(preLevelStats->magic), x1, y, 1, 14);
	if (unit->magic > preLevelStats->magic)
	{
	//	Text->RenderText(intToString(1), x - 65, y + 20, 1);
	}
	y += 21;
	Text->RenderTextRight(intToString(preLevelStats->skill), x1, y, 1, 14);
	if (unit->skill > preLevelStats->skill)
	{
//		Text->RenderText(intToString(1), x - 65, y + 45, 1);
	}
	y = 450;
	Text->RenderTextRight(intToString(preLevelStats->speed), x2, y, 1, 14);
	if (unit->speed > preLevelStats->speed)
	{
	//	Text->RenderText(intToString(1), x + 55, y - 30, 1);
	}
	y += 21;
	Text->RenderTextRight(intToString(preLevelStats->luck), x2, y, 1, 14);
	if (unit->luck > preLevelStats->luck)
	{
	//	Text->RenderText(intToString(1), x + 55, y - 5, 1);
	}
	y += 21;
	Text->RenderTextRight(intToString(preLevelStats->defense), x2, y, 1, 14);
	if (unit->defense > preLevelStats->defense)
	{
	//	Text->RenderText(intToString(1), x + 55, y + 20, 1);
	}
	y += 21;
	Text->RenderTextRight(intToString(preLevelStats->build), x2, y, 1, 14);
	if (unit->build > preLevelStats->build)
	{
	//	Text->RenderText(intToString(1), x + 55, y + 45, 1);
	}

	ResourceManager::GetShader("shape").Use().SetFloat("alpha", levelUpBlockAlpha);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(0, 127, 0.0f));

	model = glm::scale(model, glm::vec3(256, 80, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.0f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

void InfoDisplays::DrawExperienceDisplay(Camera* camera, int shapeVAO, TextRenderer* Text, SpriteRenderer* renderer)
{
	if (battleDisplay)
	{
		DrawBattleExperience(camera, shapeVAO, Text, renderer);
	}
	else
	{
		DrawMapExperience(camera, shapeVAO, Text);
	}
}

void InfoDisplays::DrawMapExperience(Camera* camera, int shapeVAO, TextRenderer* Text)
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(40, 96, 0.0f));

	model = glm::scale(model, glm::vec3(160, 26, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.0f, 1.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	model = glm::mat4();
	model = glm::translate(model, glm::vec3(71, 106, 0.0f));

	model = glm::scale(model, glm::vec3(102, 6, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 1.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	model = glm::mat4();
	model = glm::translate(model, glm::vec3(71, 106, 0.0f));

	model = glm::scale(model, glm::vec3(1 * displayedExperience, 6, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.5f, 1.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	Text->RenderText("EXP", 150, 278, 1);
	Text->RenderTextRight(intToString(displayedExperience), 550, 281, 1, 14);
}

void InfoDisplays::DrawBattleExperience(Camera* camera, int shapeVAO, TextRenderer* Text, SpriteRenderer* renderer)
{
	ResourceManager::GetShader("Nsprite").Use();
	ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());

	Texture2D texture = ResourceManager::GetTexture("BattleExperienceBackground");
	renderer->setUVs();
	renderer->DrawSprite(texture, glm::vec2(5, 140), 0, glm::vec2(246, 32));

	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(72, 154, 0.0f));

	model = glm::scale(model, glm::vec3(159, 5, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.5f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	model = glm::mat4();
	model = glm::translate(model, glm::vec3(72, 154, 0.0f));

	model = glm::scale(model, glm::vec3(1 * displayedExperience, 5, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.7f, 1.0f, 1.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	Text->RenderText("EXP", 75, 407, 1);
	Text->RenderTextRight(intToString(displayedExperience), 175, 409, 1, 14);
}
