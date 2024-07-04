#pragma once
#include <vector>
#include "Unit.h"
#include <nlohmann/json.hpp>
struct PlayerManager
{
	std::vector<Unit*> playerUnits;
	std::unordered_map<std::string, int> weaponNameMap;

	std::mt19937* gen;
	std::uniform_int_distribution<int>* distribution;
	Observer<class Unit*>* unitEvents;
	std::unordered_map<int, Unit*>* sceneUnits;

	int funds = 0;

	void init(std::mt19937* gen, std::uniform_int_distribution<int>* distribution, Observer<Unit*>* unitEvents, std::unordered_map<int, Unit*>* sceneUnits);
	void Update(float deltaTime, int idleFrame, class InputManager& inputManager);

	void LoadUnits(std::ifstream& map);
	Unit* LoadUnit(nlohmann::json& bases, int unitID, glm::vec2& position);
	void Load(json saveData);
	void AddUnit(int unitID, glm::vec2& position);

	void Draw(class SpriteRenderer* Renderer);
	void Draw(class SBatch* Batch, std::vector<Sprite>& carrySprites);

	void Clear();
};