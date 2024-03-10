#include "PostBattleDisplays.h"
#include "Unit.h"
#include "ResourceManager.h"
#include "TextRenderer.h"
#include "Globals.h"
#include "Camera.h"
#include <glm.hpp>

void PostBattleDisplays::AddExperience(Unit* unit, Unit* foe)
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

void PostBattleDisplays::OnUnitLevel(Unit* unit)
{
	leveledUnit = unit;
	preLevelStats = new StatGrowths{ leveledUnit->maxHP, leveledUnit->strength, leveledUnit->magic,
		leveledUnit->skill, leveledUnit->speed, leveledUnit->luck,leveledUnit->defense,leveledUnit->build,leveledUnit->move };
	state = LEVEL_UP_NOTE;
}

void PostBattleDisplays::Update(float deltaTime)
{
	switch (state)
	{
	case NONE:
		break;
	case ADD_EXPERIENCE:
		UpdateExperienceDisplay(deltaTime);
		break;
	case LEVEL_UP_NOTE:
		levelUpNoteTimer += deltaTime;
		if (levelUpNoteTimer > levelUpNoteTime)
		{
			levelUpNoteTimer = 0.0f;
			state = LEVEL_UP;
		}
		break;
	case LEVEL_UP:
		UpdateLevelUpDisplay(deltaTime);
		break;
	default:
		break;
	}
}

void PostBattleDisplays::UpdateLevelUpDisplay(float deltaTime)
{
	levelUpTimer += deltaTime;
	if (levelUpTimer > 2.0f)
	{
		levelUpTimer = 0;
		delete preLevelStats;
		leveledUnit = nullptr;
		state = NONE;
		subject.notify();
	}
}

void PostBattleDisplays::UpdateExperienceDisplay(float deltaTime)
{
	experienceTimer += deltaTime;
	if (displayingExperience)
	{
		if (experienceTimer > experienceDisplayTime)
		{
			leveledUnit->AddExperience(gainedExperience);
			displayingExperience = false;
			if (state == ADD_EXPERIENCE)
			{
				leveledUnit = nullptr;
				state = NONE;
				subject.notify();
			}
			experienceTimer = 0;
		}
	}
	else
	{
		if (experienceTimer > experienceTime)
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
			else 
			experienceTimer = 0;
		}
	}
}

void PostBattleDisplays::Draw(Camera* camera, TextRenderer* Text, int shapeVAO)
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
	default:
		break;
	}
}

void PostBattleDisplays::DrawLevelUpDisplay(Camera* camera, int shapeVAO, TextRenderer* Text)
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

void PostBattleDisplays::DrawExperienceDisplay(Camera* camera, int shapeVAO, TextRenderer* Text)
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
