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
	//Need to find a proper way to increment this. I worry it will break if I load a game that has dead enemies, the numbering will be completely off
	int levelID = 0;
	std::vector<Unit*> units;
	std::unordered_map<std::string, int> weaponNameMap;

	std::mt19937* gen;
	std::uniform_int_distribution<int>* distribution;

	void init(std::mt19937* gen, std::uniform_int_distribution<int>* distribution);
	void Load(json saveData);
	Unit* LoadUnitFromSuspend(const json& unit);
};