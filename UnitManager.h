#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include <random>
#include <nlohmann/json.hpp>
#include "Unit.h"

using json = nlohmann::json;

struct UnitManager
{
	std::vector<Unit*> units;
	std::unordered_map<std::string, int> weaponNameMap;

	std::mt19937* gen;
	std::uniform_int_distribution<int>* distribution;

	void init(std::mt19937* gen, std::uniform_int_distribution<int>* distribution);
	void Load(json saveData);
	Unit* LoadUnitFromSuspend(const json& unit);
};