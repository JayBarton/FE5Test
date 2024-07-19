#pragma once
#include <vector>
#include "Globals.h"
#include "TextAdvancer.h"
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
	TURN_CHANGE,
	ENEMY_BUY,
	GOT_ITEM,
	PLAYER_DIED,
	UNIT_ESCAPED,
	STAT_BOOST
};

class EnemyManager;
struct InfoDisplays
{
	DisplayState state = NONE;
	Subject<int> subject;
	Unit* focusedUnit = nullptr;
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
	float statDisplayTime = 0.5f;
	float statViewTime = 5.0f;

	float turnDisplayAlpha = 0.0f;
	float turnDisplayMaxAlpha = 58.0f;

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
	bool unitDeathFadeBack = false;
	bool statDelay = false;

	bool capturing = false;

	bool playTurnChange = false;
	
	float levelUpNoteTime = 1.0f;

	int itemToUse;
	int displayedHP;
	int healedHP;
	int turn;

	float textDisplayTime = 1.0f;

	//I seriously need to fix this stuff, just getting repetitive
	TextObjectManager* textManager;
	TextObject testText;

	void init(TextObjectManager* textManager);

	void AddExperience(Unit* unit, Unit* foe);
	void StartUse(Unit* unit, int index, Camera* camera);
	void StartUnitStatBoost(Unit* unit, Camera* camera);
	void EnemyUse(Unit* enemy, int index);
	void EnemyTrade(EnemyManager* enemyManager);
	void EnemyBuy(EnemyManager* enemyManager);
	void GetItem(int itemID);
	void StartUnitHeal(Unit*, int healAmount, Camera* camera);
	void ChangeTurn(int currentTurn);
	void PlayerUnitDied(Unit* unit);
	void PlayerLost(int messageID);
	void UnitEscaped(EnemyManager* enemyManager);

	void OnUnitLevel(Unit* unit);

	void Update(float deltaTime, class InputManager& inputManager);
	void TurnChangeUpdate(InputManager& inputManager, float deltaTime);
	void UpdateHealthBarDisplay(float deltaTime);
	void UpdateLevelUpDisplay(float deltaTime);
	void UpdateExperienceDisplay(float deltaTime);
	void Draw(Camera* camera, TextRenderer* Text, int shapeVAO, struct SpriteRenderer* renderer);

	void DrawHealthBar(Camera* camera, int shapeVAO, TextRenderer* Text);

	void DrawHealAnimation(Camera* camera, int shapeVAO);

	void DrawLevelUpDisplay(Camera* camera, int shapeVAO, TextRenderer* Text);

	void DrawExperienceDisplay(Camera* camera, int shapeVAO, TextRenderer* Text);
};