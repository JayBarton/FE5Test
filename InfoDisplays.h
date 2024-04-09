#pragma once
#include <vector>
#include "Globals.h"
class Unit;
class StatGrowths;
class TextRenderer;
class Camera;

enum DisplayState
{
	NONE,
	ADD_EXPERIENCE,
	LEVEL_UP_NOTE,
	LEVEL_UP,
	HEALING_ANIMATION,
	HEALING_BAR,
	ENEMY_USE,
	ENEMY_TRADE,
	TURN_CHANGE
};
class EnemyManager;
struct InfoDisplays
{
	DisplayState state = NONE;
	Subject<int> subject;
	Unit* leveledUnit = nullptr;
	//Ugh. Figure out how to gain access to this better later
	EnemyManager* enemyManager = nullptr;

	StatGrowths* preLevelStats = nullptr; //just using this because it has all the data I need

	float displayTimer = 0.0f;
	float levelUpTime = 2.5f;
	float experienceTime = 0.0025f;
	float experienceDisplayTime = 1.0f;
	float healAnimationTime = 1.0f;
	float healDisplayTime = 0.035f;
	float healDelayTime = 0.5f;
	float turnDisplayTime = 1.0f;

	float turnDisplayAlpha = 0.0f;
	float turnDisplayMaxAlpha = 0.2f;

	float turnTextX = -100;
	int turnTextXFinal = 400;

	int displayedExperience = 0;
	int gainedExperience = 0;
	int finalExperience = 0;
	bool displayingExperience = false;
	bool finishedHealing = false;
	bool usedItem = false;
	bool turnChangeStart = false;
	bool healDelay = false;
	

	float levelUpNoteTime = 1.0f;

	int itemToUse;
	int displayedHP;
	int healedHP;
	int turn;

	float textDisplayTime = 1.0f;

	void AddExperience(Unit* unit, Unit* foe);
	void StartUse(Unit* unit, int index, Camera* camera);
	void EnemyUse(Unit* enemy, int index);
	void EnemyTrade(EnemyManager* enemyManager);
	void StartUnitHeal(Unit*, int healAmount, Camera* camera);
	void ChangeTurn(int currentTurn);

	void OnUnitLevel(Unit* unit);

	void Update(float deltaTime, class InputManager& inputManager);
	void TurnChangeUpdate(InputManager& inputManager, float deltaTime);
	void UpdateHealthBarDisplay(float deltaTime);
	void UpdateLevelUpDisplay(float deltaTime);
	void UpdateExperienceDisplay(float deltaTime);
	void Draw(Camera* camera, TextRenderer* Text, int shapeVAO);

	void DrawHealthBar(Camera* camera, int shapeVAO, TextRenderer* Text);

	void DrawHealAnimation(Camera* camera, int shapeVAO);

	void DrawLevelUpDisplay(Camera* camera, int shapeVAO, TextRenderer* Text);

	void DrawExperienceDisplay(Camera* camera, int shapeVAO, TextRenderer* Text);


};