#pragma once
#include <random>
#include "Unit.h"
#include "TextAdvancer.h"

class TextRenderer;

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

class Camera;
struct BattleManager
{
	void SetUp(Unit* attacker, Unit* defender, BattleStats attackerStats, BattleStats defenderStats, int attackDistance, bool canDefenderAttack, Camera& camera, bool aiAttack = false, bool capturing = false);

	void Update(float deltaTime, std::mt19937* gen, std::uniform_int_distribution<int>* distribution, class InfoDisplays& displays, class InputManager& inputManager);

	void CheckAccost();

	void PreBattleChecks(Unit* thisUnit, BattleStats& theseStats, Unit* foe, Attack& attack, std::uniform_int_distribution<int>* distribution, std::mt19937* gen);

	void DoBattleAction(Unit* thisUnit, Unit* otherUnit, int accuracy, int crit, BattleStats& theseStats, Attack& attack, int foeDefense, std::uniform_int_distribution<int>* distribution, std::mt19937* gen);

	void EndAttack();
	
	void EndBattle(class Cursor* cursor, class EnemyManager* enemyManager, Camera& camera);

	void DropHeldUnit();

	void CaptureUnit();

	void Draw(TextRenderer* text, Camera& camera, class SpriteRenderer* Renderer, class Cursor* cursor);

	Unit* attacker = nullptr;
	Unit* defender = nullptr;
	Unit* deadUnit = nullptr;
	Unit* unitToDrop = nullptr;
	Unit* actingUnit = nullptr;

	glm::vec2 movementDirection;
	glm::vec2 startPosition;
	bool moveBack = false;

	BattleStats attackerStats;
	BattleStats defenderStats;

	glm::vec2 cameraStart;
	glm::vec2 battleDirection;

	int attackDistance;

	float actionDelay = 1.0f;
	float actionTimer = 0.0f;

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

	float delayTimer = 0.0f;
	float delay = 0.75f;

	std::vector<Attack> battleQueue;
	std::vector<Attack> accostQueue;

	Subject<Unit*, Unit*> subject;
	Subject<Unit*> unitDiedSubject;

	MissText missedText;
};