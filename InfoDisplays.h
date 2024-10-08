#pragma once
#include <vector>
#include "Globals.h"
#include "TextAdvancer.h"
class Unit;
class StatGrowths;
class TextRenderer;
class Camera;

//I think I rework this in the future, and will split off the different states into different structs,
//I think it will help to not have a bunch of variables hanging around that are only used for 1-2 states
//For now I will just deal with this being a mess

enum DisplayState
{
	NONE,
	ADD_EXPERIENCE,
	LEVEL_UP_NOTE,
	MAP_LEVEL_UP,
	BATTLE_LEVEL_UP,
	BATTLE_FADE_THING,
	BATTLE_LEVEL_DELAY,
	BATTLE_EXPERIENCE_DELAY,
	LEVEL_UP_BLOCK,
	HEALING_ANIMATION,
	HEALING_BAR,
	ENEMY_USE,
	ENEMY_TRADE,
	TURN_CHANGE,
	FAST_TURN_CHANGE,
	ENEMY_BUY,
	GOT_ITEM,
	BATTLE_SPEECH,
	UNIT_ESCAPED,
	STAT_BOOST
};

class EnemyManager;
struct InfoDisplays
{
	DisplayState state = NONE;
	Subject<int> endBattle;
	Subject<int> endTurn;
	Unit* focusedUnit = nullptr;
	//Ugh. Figure out how to gain access to this better later
	EnemyManager* enemyManager = nullptr;

	StatGrowths* preLevelStats = nullptr; //just using this because it has all the data I need

	glm::vec4 arrowUV;
	glm::vec4 levelUpUV;
	glm::vec4 healUV;
	std::vector<glm::vec4> turnTextUVs;

	float arrowY;
	float arrowT;

	float displayTimer = 0.0f;
	float levelUpTime = 2.5f;
	float experienceTime = 0.0025f;
	float experienceDisplayTime = 1.0f;
	float healAnimationTime = 1.0f;
	float healDisplayTime = 0.026f;
	float healDelayTime = 0.5f;
	float turnDisplayTime = 1.0f;
	float turnTextIn = 0.3f;
	float turnText2 = 0.0f;
	float statDisplayTime = 0.183f;
	float statViewTime = 5.0f;

	float turnDisplayAlpha = 0.0f;
	float turnDisplayMaxAlpha = 58.0f;

	float turnTextAlpha1 = 0.0f;
	float turnTextAlpha2 = 0.0f;
	float turnTextX = -100;
	int turnTextXFinal = 400;

	int displayedExperience = 0;
	int gainedExperience = 0;
	int finalExperience = 0;

	bool displayingExperience = false;
	bool finishedHealing = false;
	bool usedItem = false;
	bool turnChangeStart = false;
	bool secondTurnText = false;

	bool healDelay = false;
	bool healStart = false;
	bool healEnd = false;

	bool unitDeathFadeBack = false;
	bool statDelay = false;
	bool capturing = false;
	bool playTurnChange = false;
	bool battleDisplay = false;

	//For map level up display
	bool mapStats = false;
	bool mapNames = false;
	bool moveText = false;
	bool showArrow = false;

	bool changedStat[8];
	
	int activeStat = -1;

	glm::vec2 statPos;
	glm::vec2 center;
	float angle;

	int textOffset = -30;

	float levelUpNoteTime = 1.4f;

	int itemToUse;
	int displayedHP;
	int healedHP;
	int turn;

	float textDisplayTime = 1.8f;

	float levelUpBlockAlpha = 0.0f;
	glm::vec2 levelUpPosition;
	float levelUpStartY;
	float levelT = 0.0f;

	//I seriously need to fix this stuff, just getting repetitive
	TextObjectManager* textManager;
//	TextObject testText;

	float healCircleRadius = 0.0f;
	float healGlow = 0.0f;

	void init(TextObjectManager* textManager);

	void AddExperience(Unit* unit, Unit* foe, glm::vec2 levelUpPosition);
	void StartUse(Unit* unit, int index, Camera* camera);
	void StartUnitStatBoost(Unit* unit, Camera* camera);
	void EnemyUse(Unit* enemy, int index);
	void EnemyTrade(EnemyManager* enemyManager);
	void EnemyBuy(EnemyManager* enemyManager);
	void GetItem(int itemID);
	void StartUnitHeal(Unit*, int healAmount, Camera* camera);
	void ChangeTurn(int currentTurn);
	void PlayerUnitDied(Unit* unit, bool battleScene);
	void UnitBattleMessage(Unit* unit, bool battleScene, bool continuing = false, bool playMusic = false);
	void PlayerLost(int messageID);
	void UnitEscaped(EnemyManager* enemyManager);

	void OnUnitLevel(Unit* unit);

	void Update(float deltaTime, class InputManager& inputManager);
	void TurnChangeUpdate(InputManager& inputManager, float deltaTime);
	void UpdateHealthBarDisplay(float deltaTime);
	void UpdateMapLevelUpDisplay(float deltaTime, InputManager& inputManager);
	void UpdateBattleLevelUpDisplay(float deltaTime);
	void EndBattle();
	void ClearLevelUpDisplay();
	void UpdateExperienceDisplay(float deltaTime);
	void Draw(Camera* camera, TextRenderer* Text, int shapeVAO, struct SpriteRenderer* renderer);

	void DrawHealthBar(Camera* camera, int shapeVAO, TextRenderer* Text, SpriteRenderer* renderer);

	void DrawHealAnimation(Camera* camera, SpriteRenderer* renderer);

	void DrawLevelUpDisplay(Camera* camera, int shapeVAO, TextRenderer* Text, SpriteRenderer* renderer);
	void DrawBattleLevelUpDisplay(Camera* camera, int shapeVAO, TextRenderer* Text, SpriteRenderer* renderer);

	void DrawStatBars(Camera* camera, int shapeVAO);

	void DrawExperienceDisplay(Camera* camera, int shapeVAO, TextRenderer* Text, SpriteRenderer* renderer);
	void DrawMapExperience(Camera* camera, int shapeVAO, TextRenderer* Text, SpriteRenderer* renderer);
	void DrawBattleExperience(Camera* camera, int shapeVAO, TextRenderer* Text, SpriteRenderer* renderer);

	//Duped from menumanager
	void DrawBox(glm::ivec2 position, int width, int height, SpriteRenderer* renderer, Camera* camera);
	void DrawPattern(glm::vec2 scale, glm::vec2 pos, SpriteRenderer* Renderer, Camera* camera);
	void SetupBackground(glm::vec2& size);

};