#pragma once
#include <random>
#include "Unit.h"
#include "TextAdvancer.h"

class TextRenderer;
class InfoDisplays;

struct Attack
{
	bool firstAttacker = true;
	bool continuedAttack = false;
	bool vantageAttack = false;
	bool wrathAttack = false;
};

struct MissText
{
	std::string message;
	glm::vec2 position;
	glm::vec2 movePosition;
	float lifeTime = 0.5f;
	float scale = 0.5f;
	bool active;
};

struct BattleSceneStats
{
	std::string Hit;
	std::string Atc;
	int Def;
	int Lvl;
};

class Camera;
struct BattleManager
{
	void SetUp(Unit* attacker, Unit* defender, BattleStats attackerStats, BattleStats defenderStats, int attackDistance, bool canDefenderAttack, Camera& camera, bool aiAttack = false, bool capturing = false);

	void GetFacing();

	void Update(float deltaTime, std::mt19937* gen, std::uniform_int_distribution<int>* distribution, class InputManager& inputManager);

	void PrepareCapture();

	void MapUpdate(float deltaTime, InputManager& inputManager, std::uniform_int_distribution<int>* distribution, std::mt19937* gen);

	void CheckAccost();

	void PreBattleChecks(Unit* thisUnit, BattleStats& theseStats, Unit* foe, Attack& attack, int* foeHP, std::uniform_int_distribution<int>* distribution, std::mt19937* gen);

	void DoBattleAction(Unit* thisUnit, Unit* otherUnit, int accuracy, int crit, BattleStats& theseStats, Attack& attack, int foeDefense, std::uniform_int_distribution<int>* distribution, std::mt19937* gen);

	void EndAttack();
	
	void EndBattle(class Cursor* cursor, class EnemyManager* enemyManager, Camera& camera);

	void DropHeldUnit();

	void CaptureUnit();

	void GetUVs();

	void CalculateFinalStats(BattleStats& unitNormalStats, BattleStats& enemyNormalStats, Unit* unit, Unit* enemy, WeaponData& unitWeapon, WeaponData& enemyWeapon);

	void Draw(TextRenderer* text, Camera& camera, class SpriteRenderer* Renderer, class Cursor* cursor, class SBatch* Batch, int shapeVAO, class TextObjectManager* textManager);

	void DrawSceneHealthbars(Camera& camera, int shapeVAO);

	void StencilWindow(Camera& camera, int boxY, int shapeVAO);

	Unit* attacker = nullptr;
	Unit* defender = nullptr;
	Unit* deadUnit = nullptr;
	Unit* unitToDrop = nullptr;
	Unit* actingUnit = nullptr;
	Unit* talkingUnit = nullptr;

	InfoDisplays* displays = nullptr;

	glm::vec2 movementDirection;
	glm::vec2 startPosition;

	std::vector<glm::vec4> mapBattleBoxUVs;

	BattleStats attackerStats;
	BattleStats defenderStats;

	glm::vec2 cameraStart;
	glm::vec2 battleDirection;

	int attackDistance;

	float actionDelay = 1.0f;
	float actionTimer = 0.0f;

	bool moveBack = false;
	bool battleActive = false;
	bool canDefenderAttack = true;
	bool attackerTurn = true;
	bool defenderTurn = false;
	bool checkDouble = true;
	bool accostFired = false;
	bool unitDied = false;
	bool aiDelay = false;
	bool capturing = false;
	bool unitCaptured = false;
	bool droppingUnit = false;

	bool captureAnimation = false;
	//Not crazy about this either, just seems the easiest way to handle drawing at 12:06 am
	bool drawInfo = true;
	//These two are for handling the fade in of the name/display boxes during map battles
	bool fadeBoxIn = false;
	bool fadeBoxOut = false;

	//god there's so many fucking bools here dude
	//Battle scene variables
	bool battleScene = true;
	bool transitionIn = false;
	bool fadeInBattle = false;
	bool fadeOutBattle = false;
	bool fadeBackMap = false;

	int sceneUnitID = -1;

	int attackerDisplayHealth;
	int defenderDisplayHealth;
	int targetHealth;
	int* displayHealth = nullptr;
	int* rightDisplayHealth;
	int* leftDisplayHealth;
	int leftMaxHealth;
	int rightMaxHealth;

	BattleSceneStats leftStats;
	BattleSceneStats rightStats;

	glm::vec2 leftPosition;
	glm::vec2 rightPosition;
	Unit* leftUnit = nullptr;
	Unit* rightUnit = nullptr;
	glm::vec2* actingPosition = nullptr;
	float fadeOutDelay = 1.0f;
	float fadeTimer = 0.0f;
	float transitionX = -286.0f;
	float fadeAlpha = 0.0f;

	float boxThing = 16;


	//end battle scene variables

	float delayTimer = 0.0f;
	float delay = 0.75f;

	std::vector<Attack> battleQueue;
	std::vector<Attack> accostQueue;

	Subject<Unit*, Unit*> endAttackSubject;
	Subject<Unit*> unitDiedSubject;
	Subject<int> resumeMusic;

	MissText missedText;
};