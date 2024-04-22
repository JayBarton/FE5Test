#pragma once
#include <vector>
#include "Unit.h"
#include <nlohmann/json.hpp>
struct PlayerManager
{
	std::vector<Unit*> playerUnits;
	std::vector<std::vector<glm::vec4>> playerUVs;
	std::unordered_map<std::string, int> weaponNameMap;

	std::mt19937* gen;
	std::uniform_int_distribution<int>* distribution;
	Observer<class Unit*>* unitEvents;
	std::unordered_map<int, Unit*>* sceneUnits;

	void init(std::mt19937* gen, std::uniform_int_distribution<int>* distribution, Observer<Unit*>* unitEvents, std::unordered_map<int, Unit*>* sceneUnits);
	void Update(float deltaTime, int idleFrame, class InputManager& inputManager);

	void LoadUnits(std::ifstream& map);
	Unit* LoadUnit(nlohmann::json& bases, int unitID, glm::vec2& position);
	void AddUnit(int unitID, glm::vec2& position);

	void Draw(class SpriteRenderer* Renderer);

	void Clear();
};