#pragma once
#include <vector>
#include "Unit.h"
#include <nlohmann/json.hpp>
struct PlayerManager
{
	std::vector<Unit*> playerUnits;
	std::vector<glm::vec4> playerUVs;
	std::unordered_map<std::string, int> weaponNameMap;

	std::mt19937* gen;
	std::uniform_int_distribution<int>* distribution;
	Observer* unitEvents;

	void init(std::mt19937* gen, std::uniform_int_distribution<int>* distribution, Observer* unitEvents);
	void Update(float deltaTime, class InputManager& inputManager);

	void LoadUnits(std::ifstream& map);
	Unit* LoadUnit(nlohmann::json& bases, int unitID, glm::vec2& position);
	void AddUnit(int unitID, glm::vec2& position);

	void Draw(class SpriteRenderer* Renderer);

	void Clear();
};