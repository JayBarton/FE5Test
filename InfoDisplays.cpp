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

	arrowUV = ResourceManager::GetTexture("UIItems").GetUVs(8, 54, 7, 8, 1, 1)[0];
	levelUpUV = ResourceManager::GetTexture("UIItems").GetUVs(96, 96, 30, 8, 1, 1)[0];
	healUV = ResourceManager::GetTexture("UIItems").GetUVs(96, 104, 20, 20, 1, 1)[0];

	turnTextUVs = ResourceManager::GetTexture("UIItems").GetUVs(256, 0, 134, 22, 1, 6);
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
			this->levelUpPosition = focusedUnit->sprite.getPosition() + glm::vec2(-8, 8);
			levelUpStartY = this->levelUpPosition.y;
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
	mapStats = false;
	mapNames = false;
	moveText = false;
	showArrow = false;
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
	healStart = true;
	focusedUnit = unit;
	displayedHP = focusedUnit->currentHP;
	healedHP = healAmount;
	healCircleRadius = 0.0f;
	healGlow = 0.0f;
	camera->SetCenter(focusedUnit->sprite.getPosition());
}

void InfoDisplays::ChangeTurn(int currentTurn)
{
	turn = currentTurn;
	if (Settings::settings.unitSpeed < 3)
	{
		state = TURN_CHANGE;
		turnChangeStart = true;
		turnDisplayAlpha = 0.0f;
		turnTextX = 28;
		turnTextAlpha1 = 0.0f;
		turnTextAlpha2 = 0.0f;
		displayTimer = 0.0f;
		turnText2 = 0.0f;
		secondTurnText = false;
		playTurnChange = true;
	}
	else
	{
		state = FAST_TURN_CHANGE;
	}
}

void InfoDisplays::PlayerUnitDied(Unit* unit, bool battleScene)
{
	battleDisplay = battleScene;
	int id = 1;
	if (battleScene)
	{
		id = 3;
		if (unit->team > 0)
		{
			textManager->continueBattle = true;
		}
	}
	else
	{
		textManager->textObjects[1].fadeIn = true;
	}
	textManager->textLines.clear();
	textManager->textLines.push_back(SpeakerText{ nullptr, id, unit->deathMessage, unit->portraitID });
	textManager->init();
	textManager->active = true;
	textManager->talkActivated = true;
	state = BATTLE_SPEECH;
}

//A lot of duplicated stuff he I don't feel like refactoring right now
void InfoDisplays::UnitBattleMessage(Unit* unit, bool battleScene, bool continuing, bool playMusic)
{
	battleDisplay = battleScene;
	int id = 1;
	if (battleScene)
	{
		id = 3;
	}
	else
	{
		textManager->textObjects[1].fadeIn = true;
		textManager->playMusic = playMusic;
	}
	textManager->textLines.clear();
	textManager->textLines.push_back(SpeakerText{ nullptr, id, unit->battleMessage, unit->portraitID });
	textManager->init();
	textManager->active = true;
	textManager->continueBattle = continuing;
	textManager->talkActivated = true;
	unit->battleMessage = "";
	state = BATTLE_SPEECH;
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
	textManager->init();
	textManager->active = true;
	textManager->talkActivated = true;
	state = BATTLE_SPEECH;
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
		if (!battleDisplay)
		{
			if (levelT < 1)
			{
				levelT += (deltaTime) / (0.183f);
				levelUpPosition.y = glm::mix(levelUpStartY, levelUpStartY - 8, sin(levelT * glm::pi<float>()));
			}
		}
		if (displayTimer > levelUpNoteTime)
		{
			displayTimer = 0.0f;
			levelT = 0;
			if (battleDisplay)
			{
				Mix_FadeInMusic(ResourceManager::Music["LevelUpTheme"], -1, 500);
				state = BATTLE_FADE_THING;
			}
			else
			{
				state = MAP_LEVEL_UP;
				mapStats = false;
				mapNames = false;
				moveText = false;
				showArrow = false;
				Mix_VolumeMusic(128);
			}
		}
		break;
	case MAP_LEVEL_UP:
		UpdateMapLevelUpDisplay(deltaTime, inputManager);
		break;
	case BATTLE_LEVEL_UP:
		UpdateBattleLevelUpDisplay(deltaTime);
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
		else
		{
			if (healStart)
			{
				healCircleRadius += deltaTime * 3;
				if (healCircleRadius > 0.5f)
				{
					healCircleRadius = 0.5f;
					healStart = false;
					displayTimer = 0.0f;
				}
			}
			else
			{
				//glow effect here
				healGlow += deltaTime * 3.5f;
				if (displayTimer >= 0.83)
				{
					healCircleRadius -= deltaTime * 3;
					if (healCircleRadius < 0.0f)
					{
						healCircleRadius = 0.0f;
					}
				}
				if (displayTimer > healAnimationTime)
				{
					displayTimer = 0.0f;
					state = HEALING_BAR;
				}
			}
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
	case FAST_TURN_CHANGE:
		if (displayTimer >= 1.0f)
		{
			displayTimer = 0.0f;
			endTurn.notify(0);
			state = NONE;
		}
		break;
	case BATTLE_SPEECH:
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
			}
		}
		else if (!mapStats)
		{
			if (displayTimer >= 0.27f)
			{
				mapStats = true;
				displayTimer = 0;
			}
		}
		else if (!mapNames)
		{
			if (displayTimer >= 0.27f)
			{
				mapNames = true;
				displayTimer = 0;
			}
		}
		else if (!moveText)
		{
			textOffset += 6;
			if (textOffset >= 0)
			{
				textOffset = 0;
				moveText = true;
			}
		}
		else if (!showArrow)
		{
			if (displayTimer >= 0.3f)
			{
				showArrow = true;
				displayTimer = 0;
				ResourceManager::PlaySound("pointUp");
			}
		}
		else
		{
			if (inputManager.isKeyPressed(SDLK_RETURN))
			{
				displayTimer = 5.0f;
			}

			float t = pow(sin(arrowT), 2);

			arrowY = glm::mix(0.0f, 3.0f, glm::smoothstep(0.2f, 0.8f, t));
			arrowT += 3.5f * deltaTime;
			arrowT = fmod(arrowT, glm::pi<float>());

			if (displayTimer >= 5.0f)
			{
				displayTimer = 0;

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
			float t = displayTimer / turnTextIn;
			if (t >= 1)
			{
				t = 1;
				secondTurnText = true;
			}
			turnTextX = glm::mix(28, 62, t);
			turnTextAlpha1 = glm::mix(0.0f, 1.0f, t);

			if (secondTurnText)
			{
				turnText2 += deltaTime;
				t = turnText2 / turnTextIn;
				if (t >= 1)
				{
					t = 1;
				}
				turnTextAlpha2 = glm::mix(0.0f, 1.0f, t);
			}
		}
		else
		{
			turnDisplayAlpha -= 186 * deltaTime;
			if (turnDisplayAlpha <= 0)
			{
				turnDisplayAlpha = 0;
			}
			float t = displayTimer / turnTextIn;
			if (t >= 1)
			{
				t = 1;
			}
			turnTextAlpha1 = glm::mix(1.0f, 0.0f, displayTimer);
			turnTextAlpha2 = glm::mix(1.0f, 0.0f, t);
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
				secondTurnText = false;
				state = NONE;
			}
		}
	}
	if (state == NONE)
	{
		endTurn.notify(0);
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

void InfoDisplays::UpdateMapLevelUpDisplay(float deltaTime, InputManager& inputManager)
{
	if (!mapStats)
	{
		if (displayTimer >= 0.27f)
		{
			mapStats = true;
			displayTimer = 0;
		}
	}
	else if (!mapNames)
	{
		if (displayTimer >= 0.27f)
		{
			mapNames = true;
			displayTimer = 0;
		}
	}
	else if (!moveText)
	{
		textOffset += 6;
		if (textOffset >= 0)
		{
			textOffset = 0;
			moveText = true;
		}
	}
	else if (!showArrow)
	{
		if (displayTimer >= 0.3f)
		{
			showArrow = true;
			displayTimer = 0;
			ResourceManager::PlaySound("pointUp");
		}
	}
	else
	{
		if (inputManager.isKeyPressed(SDLK_RETURN))
		{
			displayTimer = 5.0f;
		}

		float t = pow(sin(arrowT), 2);
		
		arrowY = glm::mix(0.0f, 3.0f, glm::smoothstep(0.2f, 0.8f, t));
		arrowT += 3.5f * deltaTime;
		arrowT = fmod(arrowT, glm::pi<float>());

		if (displayTimer >= 5.0f)
		{
			displayTimer = 0;

			ClearLevelUpDisplay();

			EndBattle();
		}
	}
}

void InfoDisplays::UpdateBattleLevelUpDisplay(float deltaTime)
{
	if (activeStat >= 0)
	{
		angle += 10 * deltaTime;

		// Calculate the new position using the parametric equation of a circle
		statPos = center + glm::vec2(40 * cos(angle), 25 * sin(angle));

		if (displayTimer >= 0.5f)
		{
			displayTimer = 0;
			changedStat[activeStat] = false;
			activeStat = -1;
		}
	}
	else
	{
		auto unit = focusedUnit;
		if (unit->maxHP > preLevelStats->maxHP)
		{
			activeStat = 0;
			preLevelStats->maxHP++;
			statPos = glm::vec2(431, 423);
		}
		else if (unit->strength > preLevelStats->strength)
		{
			activeStat = 1;
			preLevelStats->strength++;
			statPos = glm::vec2(431, 444);
		}
		else if (unit->magic > preLevelStats->magic)
		{
			activeStat = 2;
			preLevelStats->magic++;
			statPos = glm::vec2(431, 465);
		}
		else if (unit->skill > preLevelStats->skill)
		{
			activeStat = 3;
			preLevelStats->skill++;
			statPos = glm::vec2(431, 486);
		}
		else if (unit->speed > preLevelStats->speed)
		{
			activeStat = 4;
			preLevelStats->speed++;
			statPos = glm::vec2(706, 423);
		}
		else if (unit->luck > preLevelStats->luck)
		{
			activeStat = 5;
			preLevelStats->luck++;
			statPos = glm::vec2(706, 444);
		}
		else if (unit->defense > preLevelStats->defense)
		{
			activeStat = 6;
			preLevelStats->defense++;
			statPos = glm::vec2(706, 465);
		}
		else if (unit->build > preLevelStats->build)
		{
			activeStat = 7;
			preLevelStats->build++;
			statPos = glm::vec2(706, 486);
		}
		if (activeStat >= 0)
		{
			changedStat[activeStat] = true;
			center = glm::vec2(statPos.x + 10.0f, statPos.y + 27.0f);
			angle = 180.0f;
			ResourceManager::PlaySound("pointUp");
		}
		else if (displayTimer > 2.0f)
		{
			displayTimer = 0;

			state = BATTLE_LEVEL_DELAY;

			EndBattle();
		}
	}
}

void InfoDisplays::EndBattle()
{
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

void InfoDisplays::ClearLevelUpDisplay()
{
	delete preLevelStats;
	preLevelStats = nullptr;
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
				if (battleDisplay)
				{
					state = BATTLE_EXPERIENCE_DELAY;
				}
				else
				{
					state = NONE;
				}
				EndBattle();
			}
		}
	}
	else
	{
		if (displayTimer > experienceTime)
		{
			ResourceManager::PlaySound("experience", 1);
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
	case BATTLE_EXPERIENCE_DELAY:
		DrawExperienceDisplay(camera, shapeVAO, Text, renderer);
		break;
	case LEVEL_UP_NOTE:
	{
		glm::vec2 drawPosition = levelUpPosition;
		ResourceManager::GetShader("Nsprite").Use();
		if (battleDisplay)
		{
			ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());
			DrawBattleExperience(camera, shapeVAO, Text, renderer);
		}
		else
		{
			ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getCameraMatrix());
		}
		auto texture = ResourceManager::GetTexture("UIItems");
		renderer->setUVs(levelUpUV);
		renderer->DrawSprite(texture, drawPosition, 0.0f, glm::ivec2(30, 8));
		break;
	}
	case MAP_LEVEL_UP:
		DrawLevelUpDisplay(camera, shapeVAO, Text, renderer);
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
		DrawHealAnimation(camera, renderer);
		break;
	case HEALING_BAR:
		DrawHealthBar(camera, shapeVAO, Text, renderer);
		break;
	case ENEMY_USE:
	{
		float boxY = 16;
		float textY = 75;
		if (camera->position.y <= 112)
		{
			boxY = 174;
			textY = 498;
		}
		DrawBox(glm::vec2(88, boxY), 74, 34, renderer, camera);

		Text->RenderText("Used " + focusedUnit->inventory[itemToUse]->name, 300, textY, 1);
		break;
	}
	case ENEMY_TRADE:
	{
		DrawBox(glm::vec2(88, 102), 74, 34, renderer, camera);
		ResourceManager::GetShader("Nsprite").Use();
		ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());
		auto texture = ResourceManager::GetTexture("icons");

		renderer->setUVs(MenuManager::menuManager.itemIconUVs[focusedUnit->inventory[itemToUse]->ID]);
		renderer->DrawSprite(texture, glm::vec2(96, 112), 0.0f, glm::ivec2(16));
		Text->RenderText("Recieved " + focusedUnit->inventory[itemToUse]->name, 350, 300, 1);
		break;
	}
	case ENEMY_BUY:
		DrawBox(glm::vec2(88, 102), 74, 34, renderer, camera);
		Text->RenderText(focusedUnit->GetEquippedItem()->name, 350, 300, 1);
		break;
	case GOT_ITEM:
	{
		DrawBox(glm::vec2(80, 96), 90, 34, renderer, camera);
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

		ResourceManager::GetShader("Nsprite").Use();
		ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());
		auto texture = ResourceManager::GetTexture("UIItems");
		renderer->setUVs(turnTextUVs[turn]);
		renderer->DrawSprite(texture, glm::vec2(turnTextX, 97), 0.0f, glm::ivec2(134, 22), glm::vec4(1, 1, 1, turnTextAlpha1));

		renderer->shader = ResourceManager::GetShader("outline");
		renderer->setUVs(turnTextUVs[turn + 4]);

		ResourceManager::GetShader("outline").Use().SetMatrix4("projection", camera->getOrthoMatrix());

		float step = 3.0f;

		ResourceManager::GetShader("outline").SetVector2f
		("stepSize", step / ResourceManager::GetTexture("UIItems").Width, step / ResourceManager::GetTexture("UIItems").Height);
		glm::vec2 size(134, 22);

		ResourceManager::GetShader("outline").SetVector4f("bounds", turnTextUVs[turn]);
		ResourceManager::GetShader("outline").SetInteger("turn", turn);

		renderer->DrawSprite(texture, glm::vec2(turnTextX, 97), 0.0f, glm::ivec2(134, 22), glm::vec4(1, 1, 1, turnTextAlpha2));

		renderer->shader = ResourceManager::GetShader("Nsprite");

		std::string thisTurn;
		if (turn == 0)
		{
			thisTurn = "Player Turn";
		}
		else
		{
			thisTurn = "Enemy Turn";
		}

		break;
	}
	case UNIT_ESCAPED:
	{
		DrawBox(glm::vec2(96, 96), 66, 34, renderer, camera);

		Text->RenderText("Unit Escaped", 325, 289, 1);
		break;
	}
	case STAT_BOOST:
	{
		if (!statDelay)
		{
			DrawPattern(glm::vec2(102, 38), glm::vec2(77, 101), renderer, camera);

			renderer->shader = ResourceManager::GetShader("slice");
			ResourceManager::GetShader("slice").Use();
			ResourceManager::GetShader("slice").SetMatrix4("projection", camera->getOrthoMatrix());
			auto uiTexture = ResourceManager::GetTexture("UIStuff");

			glm::vec4 uvs = MenuManager::menuManager.boxesUVs[2];
			glm::vec2 size = glm::vec2(112, 48);
			float borderSize = 6.0f;
			ResourceManager::GetShader("slice").SetVector2f("u_dimensions", borderSize / size.x, borderSize / size.y);
			ResourceManager::GetShader("slice").SetVector2f("u_border", borderSize / 32.0f, borderSize / 32.0f);
			ResourceManager::GetShader("slice").SetVector4f("bounds", uvs.x, uvs.y, uvs.z, uvs.w);
			renderer->setUVs();
			renderer->DrawSprite(uiTexture, glm::vec2(72, 96), 0.0f, size);
			renderer->shader = ResourceManager::GetShader("Nsprite");

			ResourceManager::GetShader("Nsprite").Use();
			ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());

			float textSize = 0.5f;
			int y = 278;

			if (mapNames)
			{
				int x1 = 253;
				int x2 = 403;
				Text->RenderTextRight("HP", x1 + textOffset, y, textSize, 42, glm::vec3(1), glm::vec2(x1, 0));
				Text->RenderTextRight("STR", x1 + textOffset, y + 21, textSize, 42, glm::vec3(1), glm::vec2(x1, 0));
				Text->RenderTextRight("MAG", x1 + textOffset, y + 42, textSize, 42, glm::vec3(1), glm::vec2(x1, 0));
				Text->RenderTextRight("SKL", x1 + textOffset, y + 63, textSize, 42, glm::vec3(1), glm::vec2(x1, 0));
				Text->RenderTextRight("SPD", x2 + textOffset, y, textSize, 42, glm::vec3(1), glm::vec2(x2, 0));
				Text->RenderTextRight("LCK", x2 + textOffset, y + 21, textSize, 42, glm::vec3(1), glm::vec2(x2, 0));
				Text->RenderTextRight("DEF", x2 + textOffset, y + 42, textSize, 42, glm::vec3(1), glm::vec2(x2, 0));
				Text->RenderTextRight("BLD", x2 + textOffset, y + 63, textSize, 42, glm::vec3(1), glm::vec2(x2, 0));
			}
			if (mapStats)
			{
				auto unit = focusedUnit;
				Text->RenderTextRight(intToString(preLevelStats->maxHP), 309, y, textSize, 28);
				Text->RenderTextRight(intToString(preLevelStats->strength), 309, y + 21, textSize, 28);
				Text->RenderTextRight(intToString(preLevelStats->magic), 309, y + 42, textSize, 28);
				Text->RenderTextRight(intToString(preLevelStats->skill), 309, y + 63, textSize, 28);
				Text->RenderTextRight(intToString(preLevelStats->speed), 459, y, textSize, 28);
				Text->RenderTextRight(intToString(preLevelStats->luck), 459, y + 21, textSize, 28);
				Text->RenderTextRight(intToString(preLevelStats->defense), 459, y + 42, textSize, 28);
				Text->RenderTextRight(intToString(preLevelStats->build), 459, y + 63, textSize, 28);

				//I am thinking I'll do some sort of instanced rendering for the arrows, because I don't like all of the shader switching here
				auto texture = ResourceManager::GetTexture("UIItems");
				renderer->setUVs(arrowUV);
				if (showArrow)
				{
					if (unit->maxHP > preLevelStats->maxHP)
					{
						Text->RenderText(intToString(unit->maxHP - preLevelStats->maxHP), 375, y, textSize, glm::vec3(0.7529f, 0.685f, 0.9725f));

						renderer->DrawSprite(texture, glm::vec2(111, 104 - arrowY), 0.0f, glm::ivec2(7, 8));
					}
					if (unit->strength > preLevelStats->strength)
					{
						Text->RenderText(intToString(unit->strength - preLevelStats->strength), 375, y + 21, textSize, glm::vec3(0.7529f, 0.685f, 0.9725f));

						renderer->DrawSprite(texture, glm::vec2(111, 112 - arrowY), 0.0f, glm::ivec2(7, 8));
					}
					if (unit->magic > preLevelStats->magic)
					{
						Text->RenderText(intToString(unit->magic - preLevelStats->magic), 375, y + 42, textSize, glm::vec3(0.7529f, 0.685f, 0.9725f));

						renderer->DrawSprite(texture, glm::vec2(111, 120 - arrowY), 0.0f, glm::ivec2(7, 8));
					}
					if (unit->skill > preLevelStats->skill)
					{
						Text->RenderText(intToString(unit->skill - preLevelStats->skill), 375, y + 63, textSize, glm::vec3(0.7529f, 0.685f, 0.9725f));

						renderer->DrawSprite(texture, glm::vec2(111, 128 - arrowY), 0.0f, glm::ivec2(7, 8));
					}
					if (unit->speed > preLevelStats->speed)
					{
						Text->RenderText(intToString(unit->speed - preLevelStats->speed), 525, y, textSize, glm::vec3(0.7529f, 0.685f, 0.9725f));

						renderer->DrawSprite(texture, glm::vec2(159, 104 - arrowY), 0.0f, glm::ivec2(7, 8));
					}
					if (unit->luck > preLevelStats->luck)
					{
						Text->RenderText(intToString(unit->luck - preLevelStats->luck), 525, y + 21, textSize, glm::vec3(0.7529f, 0.685f, 0.9725f));

						renderer->DrawSprite(texture, glm::vec2(159, 112 - arrowY), 0.0f, glm::ivec2(7, 8));
					}
					if (unit->defense > preLevelStats->defense)
					{
						Text->RenderText(intToString(unit->defense - preLevelStats->defense), 525, y + 42, textSize, glm::vec3(0.7529f, 0.685f, 0.9725f));

						renderer->DrawSprite(texture, glm::vec2(159, 120 - arrowY), 0.0f, glm::ivec2(7, 8));
					}
					if (unit->build > preLevelStats->build)
					{
						Text->RenderText(intToString(unit->build - preLevelStats->build), 525, y + 63, textSize, glm::vec3(0.7529f, 0.685f, 0.9725f));

						renderer->DrawSprite(texture, glm::vec2(159, 128 - arrowY), 0.0f, glm::ivec2(7, 8));
					}
				}
			}
		}
		break;
	}
	}
}

void InfoDisplays::DrawHealthBar(Camera* camera, int shapeVAO, TextRenderer* Text, SpriteRenderer* renderer)
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(78, 86, 0.0f));

	model = glm::scale(model, glm::vec3(92, 20, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.062f, 0.062f, 0.062f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	Texture2D displayTexture = ResourceManager::GetTexture("UIItems");
	auto mapBattleBoxUVs = ResourceManager::GetTexture("UIItems").GetUVs(0, 128, 96, 32, 2, 2, 3);
	renderer->setUVs(mapBattleBoxUVs[focusedUnit->team]);
	
	glm::vec2 pos(72, 72);

	renderer->DrawSprite(displayTexture, pos, 0, glm::vec2(96, 32));

	ResourceManager::GetShader("shapeSpecial").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(pos.x + 32, pos.y + 19, 0.0f));

	int width = 51 * (displayedHP / float(focusedUnit->maxHP));
	glm::vec2 scale(width, 4);

	model = glm::scale(model, glm::vec3(scale.x, scale.y, 0.0f));

	if (finishedHealing)
	{
		ResourceManager::GetShader("shapeSpecial").SetVector3f("innerColor", glm::vec3(1, 0.984f, 0.352f));
		ResourceManager::GetShader("shapeSpecial").SetVector3f("outerColor", glm::vec3(0.482f, 0.0627f, 0));
	}
	else
	{
		ResourceManager::GetShader("shapeSpecial").SetVector3f("innerColor", glm::vec3(0.9725f, 0.4705f, 0));
		ResourceManager::GetShader("shapeSpecial").SetVector3f("outerColor", glm::vec3(0.345f, 0, 0));
	}
	ResourceManager::GetShader("shapeSpecial").SetVector2f("scale", glm::vec2(scale.x, scale.y));
	ResourceManager::GetShader("shapeSpecial").SetInteger("innerTop", 1);
	ResourceManager::GetShader("shapeSpecial").SetInteger("innerBottom", 3);
	ResourceManager::GetShader("shapeSpecial").SetInteger("shouldSkip", 0);
	ResourceManager::GetShader("shapeSpecial").SetFloat("alpha", 1);
	ResourceManager::GetShader("shapeSpecial").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	glm::vec2 textDraw(275, 203);
	Text->RenderText(focusedUnit->name, textDraw.x, textDraw.y, 1, glm::vec3(1.0f));
	textDraw.y += 35.0f;
	textDraw.x -= 25;
	Text->RenderText(intToString(displayedHP), textDraw.x, textDraw.y, 1, glm::vec3(1.0f));
}

void InfoDisplays::DrawHealAnimation(Camera* camera, SpriteRenderer* renderer)
{
	if (!healDelay)
	{
		//healUV
		auto unitPosition = focusedUnit->sprite.getPosition() - glm::vec2(2.0f, 3.0f);
		renderer->shader = ResourceManager::GetShader("circle");
		ResourceManager::GetShader("circle").Use().SetMatrix4("projection", camera->getCameraMatrix());
		ResourceManager::GetShader("circle").SetFloat("radius", healCircleRadius);
		float t = pow(sin(healGlow * 2.0), 2) * 0.25f;
		ResourceManager::GetShader("circle").SetFloat("glow", t);

		Texture2D texture = ResourceManager::GetTexture("UIItems");
		renderer->setUVs(healUV);
		renderer->DrawSprite(texture, glm::vec2(unitPosition.x, unitPosition.y), 0, glm::vec2(20, 20), glm::vec4(1, 1, 1, 0.5f));

		renderer->shader = ResourceManager::GetShader("Nsprite");
	}
}

void InfoDisplays::DrawLevelUpDisplay(Camera* camera, int shapeVAO, TextRenderer* Text, SpriteRenderer* renderer)
{
	DrawPattern(glm::vec2(102, 38), glm::vec2(77, 101), renderer, camera);

	renderer->shader = ResourceManager::GetShader("slice");
	ResourceManager::GetShader("slice").Use();
	ResourceManager::GetShader("slice").SetMatrix4("projection", camera->getOrthoMatrix());
	auto uiTexture = ResourceManager::GetTexture("UIStuff");

	glm::vec4 uvs = MenuManager::menuManager.boxesUVs[2];
	glm::vec2 size = glm::vec2(112, 48);
	float borderSize = 6.0f;
	ResourceManager::GetShader("slice").SetVector2f("u_dimensions", borderSize / size.x, borderSize / size.y);
	ResourceManager::GetShader("slice").SetVector2f("u_border", borderSize / 32.0f, borderSize / 32.0f);
	ResourceManager::GetShader("slice").SetVector4f("bounds", uvs.x, uvs.y, uvs.z, uvs.w);
	renderer->setUVs();
	renderer->DrawSprite(uiTexture, glm::vec2(72, 96), 0.0f, size);
	renderer->shader = ResourceManager::GetShader("Nsprite");

	ResourceManager::GetShader("Nsprite").Use();
	ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());

	float textSize = 0.5f;
	int y = 278;

	if (mapNames)
	{
		int x1 = 253;
		int x2 = 403;
		Text->RenderTextRight("HP", x1 + textOffset, y, textSize, 42, glm::vec3(1), glm::vec2(x1, 0));
		Text->RenderTextRight("STR", x1 + textOffset, y + 21, textSize, 42, glm::vec3(1), glm::vec2(x1, 0));
		Text->RenderTextRight("MAG", x1 + textOffset, y + 42, textSize, 42, glm::vec3(1), glm::vec2(x1, 0));
		Text->RenderTextRight("SKL", x1 + textOffset, y + 63, textSize, 42, glm::vec3(1), glm::vec2(x1, 0));
		Text->RenderTextRight("SPD", x2 + textOffset, y, textSize, 42, glm::vec3(1), glm::vec2(x2, 0));
		Text->RenderTextRight("LCK", x2 + textOffset, y + 21, textSize, 42, glm::vec3(1), glm::vec2(x2, 0));
		Text->RenderTextRight("DEF", x2 + textOffset, y + 42, textSize, 42, glm::vec3(1), glm::vec2(x2, 0));
		Text->RenderTextRight("BLD", x2 + textOffset, y + 63, textSize, 42, glm::vec3(1), glm::vec2(x2, 0));
	}
	if (mapStats)
	{
		auto unit = focusedUnit;
		Text->RenderTextRight(intToString(preLevelStats->maxHP), 309, y, textSize, 28);
		Text->RenderTextRight(intToString(preLevelStats->strength), 309, y + 21, textSize, 28);
		Text->RenderTextRight(intToString(preLevelStats->magic), 309, y + 42, textSize, 28);
		Text->RenderTextRight(intToString(preLevelStats->skill), 309, y + 63, textSize, 28);
		Text->RenderTextRight(intToString(preLevelStats->speed), 459, y, textSize, 28);
		Text->RenderTextRight(intToString(preLevelStats->luck), 459, y + 21, textSize, 28);
		Text->RenderTextRight(intToString(preLevelStats->defense), 459, y + 42, textSize, 28);
		Text->RenderTextRight(intToString(preLevelStats->build), 459, y + 63, textSize, 28);

		if (showArrow)
		{
			//I am thinking I'll do some sort of instanced rendering for the arrows, because I don't like all of the shader switching here
			auto texture = ResourceManager::GetTexture("UIItems");
			renderer->setUVs(arrowUV);

			if (unit->maxHP > preLevelStats->maxHP)
			{
				Text->RenderText(intToString(1), 375, y, textSize, glm::vec3(0.7529f, 0.685f, 0.9725f));

				renderer->DrawSprite(texture, glm::vec2(111, 104 - arrowY), 0.0f, glm::ivec2(7, 8));
			}
			if (unit->strength > preLevelStats->strength)
			{
				Text->RenderText(intToString(1), 375, y + 21, textSize, glm::vec3(0.7529f, 0.685f, 0.9725f));

				renderer->DrawSprite(texture, glm::vec2(111, 112 - arrowY), 0.0f, glm::ivec2(7, 8));
			}
			if (unit->magic > preLevelStats->magic)
			{
				Text->RenderText(intToString(1), 375, y + 42, textSize, glm::vec3(0.7529f, 0.685f, 0.9725f));

				renderer->DrawSprite(texture, glm::vec2(111, 120 - arrowY), 0.0f, glm::ivec2(7, 8));
			}
			if (unit->skill > preLevelStats->skill)
			{
				Text->RenderText(intToString(1), 375, y + 63, textSize, glm::vec3(0.7529f, 0.685f, 0.9725f));

				renderer->DrawSprite(texture, glm::vec2(111, 128 - arrowY), 0.0f, glm::ivec2(7, 8));
			}
			if (unit->speed > preLevelStats->speed)
			{
				Text->RenderText(intToString(1), 525, y, textSize, glm::vec3(0.7529f, 0.685f, 0.9725f));

				renderer->DrawSprite(texture, glm::vec2(159, 104 - arrowY), 0.0f, glm::ivec2(7, 8));
			}
			if (unit->luck > preLevelStats->luck)
			{
				Text->RenderText(intToString(1), 525, y + 21, textSize, glm::vec3(0.7529f, 0.685f, 0.9725f));

				renderer->DrawSprite(texture, glm::vec2(159, 112 - arrowY), 0.0f, glm::ivec2(7, 8));
			}
			if (unit->defense > preLevelStats->defense)
			{
				Text->RenderText(intToString(1), 525, y + 42, textSize, glm::vec3(0.7529f, 0.685f, 0.9725f));

				renderer->DrawSprite(texture, glm::vec2(159, 120 - arrowY), 0.0f, glm::ivec2(7, 8));
			}
			if (unit->build > preLevelStats->build)
			{
				Text->RenderText(intToString(1), 525, y + 63, textSize, glm::vec3(0.7529f, 0.685f, 0.9725f));

				renderer->DrawSprite(texture, glm::vec2(159, 128 - arrowY), 0.0f, glm::ivec2(7, 8));
			}
		}
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
	Text->RenderTextRight(intToString(unit->level), 550, 375, 1, 28);

	int xString1 = 225;
	int xString2 = 500;
	int y = 450;
	int x1 = 400;
	int x2 = 675;
	y = 450;
	glm::vec3 color(1);
	glm::vec3 changedColor(0.7529f, 0.596f, 0.3764f);
	if (changedStat[0])
	{
		color = changedColor;
	}
	Text->RenderTextRight(intToString(preLevelStats->maxHP), x1, y, 1, 28, color);
	y += 21;
	color = glm::vec3(1);
	if (changedStat[1])
	{
		color = changedColor;
	}
	Text->RenderTextRight(intToString(preLevelStats->strength), x1, y, 1, 28, color);
	y += 21;
	color = glm::vec3(1);
	if (changedStat[2])
	{
		color = changedColor;
	}
	Text->RenderTextRight(intToString(preLevelStats->magic), x1, y, 1, 28, color);
	y += 21;
	color = glm::vec3(1);
	if (changedStat[3])
	{
		color = changedColor;
	}
	Text->RenderTextRight(intToString(preLevelStats->skill), x1, y, 1, 28, color);
	y = 450;
	color = glm::vec3(1);
	if (changedStat[4])
	{
		color = changedColor;
	}
	Text->RenderTextRight(intToString(preLevelStats->speed), x2, y, 1, 28, color);
	y += 21;
	color = glm::vec3(1);
	if (changedStat[5])
	{
		color = changedColor;
	}
	Text->RenderTextRight(intToString(preLevelStats->luck), x2, y, 1, 28, color);
	y += 21;
	color = glm::vec3(1);
	if (changedStat[6])
	{
		color = changedColor;
	}
	Text->RenderTextRight(intToString(preLevelStats->defense), x2, y, 1, 28, color);
	y += 21;
	color = glm::vec3(1);
	if (changedStat[7])
	{
		color = changedColor;
	}
	Text->RenderTextRight(intToString(preLevelStats->build), x2, y, 1, 28, color);

	if (activeStat >= 0)
	{
		Text->RenderText("+1", statPos.x, statPos.y, 1, changedColor);
	}

	DrawStatBars(camera, shapeVAO);

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

void InfoDisplays::DrawStatBars(Camera* camera, int shapeVAO)
{
	ResourceManager::GetShader("shapeSpecial").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(97, 171, 0.0f));

	int width = (preLevelStats->maxHP / 80.0f) * 29;
	glm::vec2 scale(width, 3);

	model = glm::scale(model, glm::vec3(scale.x, scale.y, 0.0f));

	ResourceManager::GetShader("shapeSpecial").SetVector3f("innerColor", glm::vec3(0.7098f, 0.9843f, 1.0f));
	ResourceManager::GetShader("shapeSpecial").SetVector3f("outerColor", glm::vec3(0.1294f, 0.4431f, 0.0627f));
	ResourceManager::GetShader("shapeSpecial").SetInteger("innerTop", 1);
	ResourceManager::GetShader("shapeSpecial").SetInteger("innerBottom", 2);
	ResourceManager::GetShader("shapeSpecial").SetInteger("shouldSkip", 0);
	ResourceManager::GetShader("shapeSpecial").SetFloat("alpha", 1);
	ResourceManager::GetShader("shapeSpecial").SetVector2f("scale", glm::vec2(scale.x, scale.y));
	ResourceManager::GetShader("shapeSpecial").SetMatrix4("model", model);

	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	model = glm::mat4();
	model = glm::translate(model, glm::vec3(97, 179, 0.0f));

	width = (preLevelStats->strength / 20.0f) * 29;
	scale.x = width;

	model = glm::scale(model, glm::vec3(scale.x, scale.y, 0.0f));

	ResourceManager::GetShader("shapeSpecial").SetVector2f("scale", glm::vec2(scale.x, scale.y));
	ResourceManager::GetShader("shapeSpecial").SetMatrix4("model", model);

	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	model = glm::mat4();
	model = glm::translate(model, glm::vec3(97, 187, 0.0f));

	width = (preLevelStats->magic / 20.0f) * 29;
	scale.x = width;

	model = glm::scale(model, glm::vec3(scale.x, scale.y, 0.0f));

	ResourceManager::GetShader("shapeSpecial").SetVector2f("scale", glm::vec2(scale.x, scale.y));
	ResourceManager::GetShader("shapeSpecial").SetMatrix4("model", model);

	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	model = glm::mat4();
	model = glm::translate(model, glm::vec3(97, 195, 0.0f));

	width = (preLevelStats->skill / 20.0f) * 29;
	scale.x = width;

	model = glm::scale(model, glm::vec3(scale.x, scale.y, 0.0f));

	ResourceManager::GetShader("shapeSpecial").SetVector2f("scale", glm::vec2(scale.x, scale.y));
	ResourceManager::GetShader("shapeSpecial").SetMatrix4("model", model);

	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	model = glm::mat4();
	model = glm::translate(model, glm::vec3(185, 171, 0.0f));

	width = (preLevelStats->speed / 20.0f) * 29;
	scale.x = width;

	model = glm::scale(model, glm::vec3(scale.x, scale.y, 0.0f));

	ResourceManager::GetShader("shapeSpecial").SetVector2f("scale", glm::vec2(scale.x, scale.y));
	ResourceManager::GetShader("shapeSpecial").SetMatrix4("model", model);

	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	model = glm::mat4();
	model = glm::translate(model, glm::vec3(185, 179, 0.0f));

	width = (preLevelStats->luck / 20.0f) * 29;
	scale.x = width;

	model = glm::scale(model, glm::vec3(scale.x, scale.y, 0.0f));

	ResourceManager::GetShader("shapeSpecial").SetVector2f("scale", glm::vec2(scale.x, scale.y));
	ResourceManager::GetShader("shapeSpecial").SetMatrix4("model", model);

	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	model = glm::mat4();
	model = glm::translate(model, glm::vec3(185, 187, 0.0f));

	width = (preLevelStats->defense / 20.0f) * 29;
	scale.x = width;

	model = glm::scale(model, glm::vec3(scale.x, scale.y, 0.0f));

	ResourceManager::GetShader("shapeSpecial").SetVector2f("scale", glm::vec2(scale.x, scale.y));
	ResourceManager::GetShader("shapeSpecial").SetMatrix4("model", model);

	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	model = glm::mat4();
	model = glm::translate(model, glm::vec3(185, 195, 0.0f));

	width = (preLevelStats->build / 20.0f) * 29;
	scale.x = width;

	model = glm::scale(model, glm::vec3(scale.x, scale.y, 0.0f));

	ResourceManager::GetShader("shapeSpecial").SetVector2f("scale", glm::vec2(scale.x, scale.y));
	ResourceManager::GetShader("shapeSpecial").SetMatrix4("model", model);

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
		DrawMapExperience(camera, shapeVAO, Text, renderer);
	}
}

void InfoDisplays::DrawMapExperience(Camera* camera, int shapeVAO, TextRenderer* Text, SpriteRenderer* renderer)
{
	DrawPattern(glm::vec2(154, 18), glm::vec2(43, 99), renderer, camera);

	ResourceManager::GetShader("Nsprite").Use();
	ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());

	Texture2D texture = ResourceManager::GetTexture("MapExperienceBackground");
	renderer->setUVs();
	renderer->DrawSprite(texture, glm::vec2(40, 96), 0, glm::vec2(160, 26));


	ResourceManager::GetShader("shapeSpecial").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(72, 107, 0.0f));

	int width = displayedExperience;
	glm::vec2 scale(width, 4);

	model = glm::scale(model, glm::vec3(scale.x, scale.y, 0.0f));

	ResourceManager::GetShader("shapeSpecial").SetVector3f("innerColor", glm::vec3(0.9725f, 0.9725f, 0.6588f));
	ResourceManager::GetShader("shapeSpecial").SetVector3f("outerColor", glm::vec3(0.6901f, 0.345f, 0.0627f));
	ResourceManager::GetShader("shapeSpecial").SetVector2f("scale", glm::vec2(scale.x, scale.y));
	ResourceManager::GetShader("shapeSpecial").SetInteger("innerTop", 1);
	ResourceManager::GetShader("shapeSpecial").SetInteger("innerBottom", 3);
	ResourceManager::GetShader("shapeSpecial").SetInteger("shouldSkip", 0);
	ResourceManager::GetShader("shapeSpecial").SetFloat("alpha", 1);
	ResourceManager::GetShader("shapeSpecial").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	Text->RenderText("EXP", 150, 278, 1);
	Text->RenderTextRight(intToString(displayedExperience), 550, 281, 1, 28);
}

void InfoDisplays::DrawBattleExperience(Camera* camera, int shapeVAO, TextRenderer* Text, SpriteRenderer* renderer)
{
	ResourceManager::GetShader("Nsprite").Use();
	ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());

	Texture2D texture = ResourceManager::GetTexture("BattleExperienceBackground");
	renderer->setUVs();
	renderer->DrawSprite(texture, glm::vec2(5, 140), 0, glm::vec2(246, 32));

	ResourceManager::GetShader("shapeSpecial").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(74, 155, 0.0f));

	int width = 157 * (displayedExperience / 100.0f);
	glm::vec2 scale(width, 3);

	model = glm::scale(model, glm::vec3(scale.x, scale.y, 0.0f));

	ResourceManager::GetShader("shapeSpecial").SetVector3f("innerColor", glm::vec3(0.7098f, 0.9843f, 1.0f));
	ResourceManager::GetShader("shapeSpecial").SetVector3f("outerColor", glm::vec3(0.1294f, 0.4431f, 0.0627f));
	ResourceManager::GetShader("shapeSpecial").SetVector2f("scale", glm::vec2(scale.x, scale.y));
	ResourceManager::GetShader("shapeSpecial").SetInteger("innerTop", 1);
	ResourceManager::GetShader("shapeSpecial").SetInteger("innerBottom", 2);
	ResourceManager::GetShader("shapeSpecial").SetInteger("shouldSkip", 0);
	ResourceManager::GetShader("shapeSpecial").SetFloat("alpha", 1);
	ResourceManager::GetShader("shapeSpecial").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	Text->RenderTextRight(intToString(displayedExperience), 175, 409, 1, 28);
}

void InfoDisplays::DrawBox(glm::ivec2 position, int width, int height, SpriteRenderer* renderer, Camera* camera)
{
	renderer->shader = ResourceManager::GetShader("slice");

	ResourceManager::GetShader("slice").Use();
	ResourceManager::GetShader("slice").SetMatrix4("projection", camera->getOrthoMatrix());

	auto texture = ResourceManager::GetTexture("UIStuff");

	glm::vec4 uvs = MenuManager::menuManager.boxesUVs[0];
	glm::vec2 size = glm::vec2(width, height);
	float borderSize = 10.0f;
	ResourceManager::GetShader("slice").SetVector2f("u_dimensions", borderSize / size.x, borderSize / size.y);
	ResourceManager::GetShader("slice").SetVector2f("u_border", borderSize / 32.0f, borderSize / 32.0f);
	ResourceManager::GetShader("slice").SetVector4f("bounds", uvs.x, uvs.y, uvs.z, uvs.w);

	renderer->setUVs();

	SetupBackground(size);

	renderer->DrawSprite(texture, glm::vec2(position.x, position.y), 0.0f, size);

	renderer->shader = ResourceManager::GetShader("Nsprite");
}

void InfoDisplays::DrawPattern(glm::vec2 size, glm::vec2 pos, SpriteRenderer* Renderer, Camera* camera)
{
	//Duplicating this in a couple of places unfortunately
	int patternID = Settings::settings.backgroundPattern;
	auto inColor = Settings::settings.backgroundColors[patternID];
	glm::vec3 topColor = glm::vec3(inColor[0], inColor[1], inColor[2]);
	glm::vec3 bottomColor = glm::vec3(inColor[3], inColor[4], inColor[5]);

	Renderer->shader = ResourceManager::GetShader("patterns");
	ResourceManager::GetShader("patterns").Use();
	ResourceManager::GetShader("patterns").SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("patterns").SetVector3f("topColor", topColor / 255.0f);
	ResourceManager::GetShader("patterns").SetVector3f("bottomColor", bottomColor / 255.0f);
	ResourceManager::GetShader("patterns").SetInteger("index", patternID);

	ResourceManager::GetShader("patterns").SetVector2f("scale", size / glm::vec2(64, 32));
	//ResourceManager::GetShader("patterns").SetVector2f("sheetScale", glm::vec2(64, 32) / glm::vec2(128, 32));

	auto patternTexture = ResourceManager::GetTexture("testpattern");

	Renderer->setUVs(MenuManager::menuManager.patternUVs[patternID]);
	Renderer->DrawSprite(patternTexture, pos, 0.0f, size);

	Renderer->shader = ResourceManager::GetShader("Nsprite");
}

void InfoDisplays::SetupBackground(glm::vec2& size)
{
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
}
