#include "InfoDisplays.h"
#include "Unit.h"
#include "ResourceManager.h"
#include "TextRenderer.h"
#include "Globals.h"
#include "Camera.h"
#include "InputManager.h"
#include <glm.hpp>
#include <SDL.h>


#include "Items.h"
#include "EnemyManager.h"

void InfoDisplays::AddExperience(Unit* unit, Unit* foe)
{
	if (unit->isDead)
	{
		leveledUnit = nullptr;
		state = NONE;
		subject.notify(0);
	}
	else
	{
		leveledUnit = unit;

		gainedExperience = unit->CalculateExperience(foe);
		leveledUnit = unit;
		displayedExperience = leveledUnit->experience;
		finalExperience = gainedExperience + displayedExperience;
		if (finalExperience >= 100)
		{
			finalExperience -= 100;
		}
		state = ADD_EXPERIENCE;
	}
}

void InfoDisplays::OnUnitLevel(Unit* unit)
{
	leveledUnit = unit;
	preLevelStats = new StatGrowths{ leveledUnit->maxHP, leveledUnit->strength, leveledUnit->magic,
		leveledUnit->skill, leveledUnit->speed, leveledUnit->luck,leveledUnit->defense,leveledUnit->build,leveledUnit->move };
	state = LEVEL_UP_NOTE;
}

void InfoDisplays::StartUse(Unit* unit, int index, Camera* camera)
{
	//When I have other usable items, can check/pass in what type here to determine what to do.
	usedItem = true;
	StartUnitHeal(unit, unit->maxHP, camera);
}

void InfoDisplays::EnemyUse(Unit* unit, int index)
{
	state = ENEMY_USE;
	leveledUnit = unit;
	itemToUse = index;
}

void InfoDisplays::EnemyTrade(EnemyManager* enemyManager)
{
	state = ENEMY_TRADE;
	this->enemyManager = enemyManager;
	leveledUnit = enemyManager->enemies[enemyManager->currentEnemy];
	itemToUse = enemyManager->healIndex;
}

//Call this from StartUse
void InfoDisplays::StartUnitHeal(Unit* unit, int healAmount, Camera* camera)
{
	state = HEALING_ANIMATION;
	healDelay = true;
	leveledUnit = unit;
	displayedHP = leveledUnit->currentHP;
	healedHP = healAmount;
	camera->SetCenter(leveledUnit->sprite.getPosition());
}

void InfoDisplays::ChangeTurn(int currentTurn)
{
	turn = currentTurn;
	state = TURN_CHANGE;
	turnChangeStart = true;
	turnDisplayAlpha = 0.0f;
	turnTextX = -100;
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
			state = LEVEL_UP;
		}
		break;
	case LEVEL_UP:
		UpdateLevelUpDisplay(deltaTime);
		break;
	case HEALING_ANIMATION:
		if (healDelay)
		{
			if (displayTimer > healDelayTime)
			{
				healDelay = false;
				displayTimer = 0.0f;
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
			ItemManager::itemManager.UseItem(leveledUnit, itemToUse);
		}
	case ENEMY_TRADE:
		if (displayTimer > textDisplayTime)
		{
			displayTimer = 0.0f;
			state = ENEMY_USE;
		}
		break;
	case TURN_CHANGE:
		TurnChangeUpdate(inputManager, deltaTime);
		break;
	}
}

void InfoDisplays::TurnChangeUpdate(InputManager& inputManager, float deltaTime)
{
	if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		state = NONE;
		displayTimer = 0.0f;
	}
	else
	{
		if (turnChangeStart)
		{
			turnDisplayAlpha += deltaTime;
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
			turnDisplayAlpha -= deltaTime;
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
}

void InfoDisplays::UpdateHealthBarDisplay(float deltaTime)
{
	if (finishedHealing)
	{
		if (displayTimer > healAnimationTime)
		{
			leveledUnit->currentHP = healedHP;
			finishedHealing = false;
			displayTimer = 0;
			state = NONE;
			if (usedItem)
			{
				usedItem = false;
				//I don't know about this gravy...
				if (leveledUnit->team == 0)
				{
					subject.notify(1);
				}
				else
				{
					subject.notify(2);
				}
			}
			else
			{
				subject.notify(3);
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
			displayTimer = 0;
		}
	}
}

void InfoDisplays::UpdateLevelUpDisplay(float deltaTime)
{
	if (displayTimer > 2.0f)
	{
		displayTimer = 0;
		delete preLevelStats;
		leveledUnit = nullptr;
		state = NONE;
		subject.notify(0);
	}
}

void InfoDisplays::UpdateExperienceDisplay(float deltaTime)
{
	if (displayingExperience)
	{
		if (displayTimer > experienceDisplayTime)
		{
			leveledUnit->AddExperience(gainedExperience);
			displayingExperience = false;
			if (state == ADD_EXPERIENCE)
			{
				leveledUnit = nullptr;
				state = NONE;
				subject.notify(0);
			}
			displayTimer = 0;
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
			}
			displayTimer = 0;
		}
	}
}

void InfoDisplays::Draw(Camera* camera, TextRenderer* Text, int shapeVAO)
{
	switch (state)
	{
	case NONE:
		break;
	case ADD_EXPERIENCE:
		DrawExperienceDisplay(camera, shapeVAO, Text);
		break;
	case LEVEL_UP_NOTE:
	{
		leveledUnit->sprite.getPosition();
		glm::vec2 drawPosition = leveledUnit->sprite.getPosition();
		drawPosition = camera->worldToRealScreen(drawPosition, SCREEN_WIDTH, SCREEN_HEIGHT);
		Text->RenderText("LEVEL UP", drawPosition.x, drawPosition.y, 0.85f, glm::vec3(0.95f, 0.95f, 0.0f));
		break;
	}
	case LEVEL_UP:
		DrawLevelUpDisplay(camera, shapeVAO, Text);
		break;
	case HEALING_ANIMATION:
		DrawHealAnimation(camera, shapeVAO);
		break;
	case HEALING_BAR:
		DrawHealthBar(camera, shapeVAO, Text);
		break;
	case ENEMY_USE:
		Text->RenderText(leveledUnit->name + " used " + leveledUnit->inventory[itemToUse]->name, 300, 300, 1);
		break;
	case ENEMY_TRADE:
		Text->RenderText(leveledUnit->name + " recieved " + leveledUnit->inventory[itemToUse]->name, 300, 300, 1);
		break;
	case TURN_CHANGE:
		ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
		ResourceManager::GetShader("shape").SetFloat("alpha", turnDisplayAlpha);
		glm::mat4 model = glm::mat4();
		model = glm::translate(model, glm::vec3(0, 0, 0.0f));
		model = glm::scale(model, glm::vec3(256, 224, 0.0f));

		ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.0f, 0.0f));

		ResourceManager::GetShader("shape").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
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
}

void InfoDisplays::DrawHealthBar(Camera* camera, int shapeVAO, TextRenderer* Text)
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(80, 98, 0.0f));
	model = glm::scale(model, glm::vec3(100 * (float(displayedHP) / float(leveledUnit->maxHP)), 5, 0.0f));

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
		auto unitPosition = leveledUnit->sprite.getPosition();
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
	//This needs to be redone, need the leveled unit, not the focused unit
	auto unit = leveledUnit;
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

void InfoDisplays::DrawExperienceDisplay(Camera* camera, int shapeVAO, TextRenderer* Text)
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(80, 98, 0.0f));

	model = glm::scale(model, glm::vec3(1 * displayedExperience, 5, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.5f, 1.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	Text->RenderText("EXP", 215, 260, 1);
	Text->RenderText(intToString(displayedExperience), 565, 260, 1);
}
