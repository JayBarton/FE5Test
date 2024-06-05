#pragma once
#include <vector>
#include <glm.hpp>
#include "Unit.h"
//Want turnsubject, move to it's own file later
#include "MenuManager.h"
#include "PathFinder.h"
class SpriteRenderer;
class PathFinder;
struct AttackPosition
{
	glm::vec2 position;
	int distance;
};

struct Target
{
	int ID;
	int priority = 0;
	int range = 0;
	Item* weaponToUse = nullptr;
	std::vector<AttackPosition> attackPositions;
	BattleStats battleStats;
};

enum ActionState
{
	GET_TARGET,
	ATTACK,
	END_ATTACK,
	CANTO,
	HEALING,
	TRADING,
	APPROACHING,
	SHOPPING
};

class InfoDisplays;
class Camera;
struct EnemyManager
{
	std::vector<Unit*> enemies;
	int currentEnemy = 0;
	int attackRange = 0;
	int healIndex = -1;

	float timer = 0.0f;
	float turnStartDelay = 0.25f;
	float actionDelay = 0.25f;

	bool canCounter = true;
	bool enemyMoving = false;
	bool followCamera = false;
	bool canAct = false;
	bool skippedUnit = false;
	Unit* otherUnit = nullptr;
	InfoDisplays* displays = nullptr;

	Subject<int> subject;
	BattleStats battleStats;

	ActionState state = GET_TARGET;

	std::vector<Unit*>* playerUnits;
	std::vector<struct Vendor>* vendors;

	PathFinder pathFinder;

	void GetPriority(Unit* enemy, std::unordered_map<glm::vec2, pathCell, vec2Hash>& path, std::vector<Unit*>& otherUnits);
	void ApproachNearest(glm::vec2& position, Unit* enemy);
	void NoMove(Unit* enemy, glm::vec2& position);
	void NextUnit();
	void SetUp(std::ifstream& map, std::mt19937* gen, std::uniform_int_distribution<int>* distribution, std::vector<Unit*>* playerUnits, std::vector<Vendor>* vendors);
	void Draw(SpriteRenderer* renderer);
	void Draw(class SBatch* Batch);
	void Update(float deltaTime, BattleManager& battleManager, Camera& camera, class InputManager& inputManager);
	void DefaultUpdate(float deltaTime, Unit* enemy, Camera& camera, BattleManager& battleManager);
	bool CheckStores(Unit* enemy);
	void GoShopping(glm::vec2& position, Unit* enemy);
	void StationaryUpdate(Unit* enemy, BattleManager& battleManager, Camera& camera);
	void RangeActivation(Unit* enemy);
	void DoNothing(Unit* enemy, glm::vec2& position);
	void FindUnitInAttackRange(Unit* enemy, std::unordered_map<glm::vec2, pathCell, vec2Hash>& path, Camera& camera);
	void FindHealItem(Unit* enemy, std::unordered_map<glm::vec2, pathCell, vec2Hash>& path);
	void HealSelf(Unit* enemy, std::unordered_map<glm::vec2, pathCell, vec2Hash>& path);
	void CantoMove();
	void FinishMove();
	void UpdateEnemies(float deltaTime, int idleFrame);
	void EndTurn();
	void RemoveDeadUnits(std::unordered_map<int, Unit*>& sceneUnits);
	void Clear();
	Unit* GetCurrentUnit();
	std::vector<Unit*> GetOtherUnits(Unit* enemy);

	std::vector<AttackPosition> ValidAdjacentPositions(Unit* toAttack, const std::unordered_map<glm::vec2, pathCell, vec2Hash>& path,
		int minRange, int maxRange);

	//horrible code duplication
	void addToOpenSet(pathCell newCell, std::vector<pathCell>& checking, std::vector<std::vector<bool>>& checked, std::vector<std::vector<int>>& costs);
	void removeFromOpenList(std::vector<pathCell>& checking);

	void CheckAdjacentTiles(glm::vec2& checkingTile, std::vector<std::vector<bool>>& checked, std::vector<pathCell>& checking, pathCell startCell,
		std::vector<std::vector<int>>& costs, std::vector<glm::vec2>& foundTiles, std::vector<AttackPosition>& rangeTiles,
		const std::unordered_map<glm::vec2, pathCell, vec2Hash>& path, int minRange, int maxRange);

};