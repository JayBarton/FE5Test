#include "PlayerManager.h"
#include "Items.h"
#include "ResourceManager.h"
#include "TileManager.h"
#include "InputManager.h"
#include "SpriteRenderer.h"
#include <sstream>
#include <fstream>
#include <nlohmann/json.hpp>
#include "SBatch.h"
using json = nlohmann::json;

void PlayerManager::init(std::mt19937* gen, std::uniform_int_distribution<int>* distribution, Observer<Unit*>* unitEvents, std::unordered_map<int, Unit*>* sceneUnits)
{
	playerUVs.resize(8);
	//Leif
	/*
	* AB.reserve( A.size() + B.size() ); // preallocate memory
AB.insert( AB.end(), A.begin(), A.end() );
AB.insert( AB.end(), B.begin(), B.end() );
	*/
	playerUVs[0] = ResourceManager::GetTexture("sprites").GetUVs(0, 0, TileManager::TILE_SIZE, TileManager::TILE_SIZE, 3, 1);
	auto extras = ResourceManager::GetTexture("movesprites").GetUVs(0, 0, 32, 32, 4, 4);
	playerUVs[0].insert(playerUVs[0].end(), extras.begin(), extras.end());
	//Finn
	playerUVs[1] = ResourceManager::GetTexture("sprites").GetUVs(0, 80, TileManager::TILE_SIZE, 20, 3, 1);
	extras = ResourceManager::GetTexture("movesprites").GetUVs(768, 0, 32, 32, 4, 4);
	playerUVs[1].insert(playerUVs[1].end(), extras.begin(), extras.end());
	//Eyvale
	playerUVs[2] = ResourceManager::GetTexture("sprites").GetUVs(45, 64, 15, TileManager::TILE_SIZE, 3, 1);
	extras = ResourceManager::GetTexture("movesprites").GetUVs(384, 128, 32, 32, 4, 4);
	playerUVs[2].insert(playerUVs[2].end(), extras.begin(), extras.end());
	//Halvan/Othin
	playerUVs[3] = ResourceManager::GetTexture("sprites").GetUVs(0, 64, 15, TileManager::TILE_SIZE, 3, 1);
	playerUVs[4] = ResourceManager::GetTexture("sprites").GetUVs(0, 64, 15, TileManager::TILE_SIZE, 3, 1);
	extras = ResourceManager::GetTexture("movesprites").GetUVs(640, 0, 32, 32, 4, 4);
	playerUVs[3].insert(playerUVs[3].end(), extras.begin(), extras.end());
	playerUVs[4].insert(playerUVs[4].end(), extras.begin(), extras.end());
	//Dagda
	playerUVs[5] = ResourceManager::GetTexture("sprites").GetUVs(96, 16, TileManager::TILE_SIZE, TileManager::TILE_SIZE, 3, 1);
	extras = ResourceManager::GetTexture("movesprites").GetUVs(512, 0, 32, 32, 4, 4);
	playerUVs[5].insert(playerUVs[5].end(), extras.begin(), extras.end());
	//Tanya
	playerUVs[6] = ResourceManager::GetTexture("sprites").GetUVs(0, 48, TileManager::TILE_SIZE, TileManager::TILE_SIZE, 3, 1);
	extras = ResourceManager::GetTexture("movesprites").GetUVs(512, 128, 32, 32, 4, 4);
	playerUVs[6].insert(playerUVs[6].end(), extras.begin(), extras.end());
	//Marty
	playerUVs[7] = ResourceManager::GetTexture("sprites").GetUVs(48, 16, TileManager::TILE_SIZE, TileManager::TILE_SIZE, 3, 1);
	extras = ResourceManager::GetTexture("movesprites").GetUVs(384, 0, 32, 32, 4, 4);
	playerUVs[7].insert(playerUVs[7].end(), extras.begin(), extras.end());
	this->gen = gen;
	this->distribution = distribution;
	this->unitEvents = unitEvents;
	this->sceneUnits = sceneUnits;
	weaponNameMap["Sword"] = WeaponData::TYPE_SWORD;
	weaponNameMap["Axe"] = WeaponData::TYPE_AXE;
	weaponNameMap["Lance"] = WeaponData::TYPE_LANCE;
	weaponNameMap["Bow"] = WeaponData::TYPE_BOW;
	weaponNameMap["Thunder"] = WeaponData::TYPE_THUNDER;
	weaponNameMap["Fire"] = WeaponData::TYPE_FIRE;
	weaponNameMap["Wind"] = WeaponData::TYPE_WIND;
	weaponNameMap["Dark"] = WeaponData::TYPE_DARK;
	weaponNameMap["Light"] = WeaponData::TYPE_LIGHT;
	weaponNameMap["Staff"] = WeaponData::TYPE_STAFF;
}

void PlayerManager::Update(float deltaTime, int idleFrame, InputManager& inputManager)
{
	for (int i = 0; i < playerUnits.size(); i++)
	{
		playerUnits[i]->Update(deltaTime, idleFrame);
	//	playerUnits[i]->UpdateMovement(deltaTime, inputManager);
	}
}

void PlayerManager::LoadUnits(std::ifstream& map)
{
	std::ifstream f("BaseStats.json");
	json data = json::parse(f);
	json bases = data["PlayerUnits"];
	int currentUnit = 0;

	int numberOfStarts = 0;
	map >> numberOfStarts;
	playerUnits.resize(numberOfStarts);
	int type;
	glm::vec2 position;
	for (int i = 0; i < numberOfStarts; i++)
	{
		map >> type >> position.x >> position.y;
		auto newUnit = LoadUnit(bases, type, position);
		playerUnits[currentUnit] = newUnit;
		currentUnit++;
	}
	/*	playerUnits[0]->sprite.setSize(glm::vec2(32, 32));
	playerUnits[0]->sprite.drawOffset = glm::vec2(-8, -8);*/
	playerUnits[1]->movementType = Unit::FOOT;
	playerUnits[1]->mount = new Mount(Unit::HORSE, 1, 1, 2, 1, 3);
	playerUnits[1]->MountAction(true);
	playerUnits[1]->sprite.setSize(glm::vec2(16, 20));
	playerUnits[1]->sprite.drawOffset = glm::vec2(0, -4);
	playerUnits[2]->sprite.setSize(glm::vec2(15, 16));
	playerUnits[2]->sprite.drawOffset = glm::vec2(1, 0);
	playerUnits[3]->sprite.setSize(glm::vec2(15, 16));
	playerUnits[3]->sprite.drawOffset = glm::vec2(1, 0);
	playerUnits[4]->sprite.setSize(glm::vec2(15, 16));
	playerUnits[4]->sprite.drawOffset = glm::vec2(1, 0);

}

Unit* PlayerManager::LoadUnit(json& bases, int unitID, glm::vec2& position)
{
	for (const auto& unit : bases)
	{
		int ID = unit["ID"];
		if (ID == unitID)
		{
			std::string name = unit["Name"];
			std::string unitClass = unit["Class"];
			json stats = unit["Stats"];
			int HP = stats["HP"];
			int str = stats["Str"];
			int mag = stats["Mag"];
			int skl = stats["Skl"];
			int spd = stats["Spd"];
			int lck = stats["Lck"];
			int def = stats["Def"];
			int bld = stats["Bld"];
			int mov = stats["Mov"];
			Unit* newUnit = new Unit(unitClass, name, ID, HP, str, mag, skl, spd, lck, def, bld, mov);
			newUnit->sceneID = ID;
			newUnit->level = stats["Level"];
			(*sceneUnits)[newUnit->sceneID] = newUnit;

			json growths = unit["GrowthRates"];
			HP = growths["HP"];
			str = growths["Str"];
			mag = growths["Mag"];
			skl = growths["Skl"];
			spd = growths["Spd"];
			lck = growths["Lck"];
			def = growths["Def"];
			bld = growths["Bld"];
			mov = growths["Mov"];

			newUnit->growths = StatGrowths{ HP, str, mag, skl, spd, lck, def, bld, mov };
			json weaponProf = unit["WeaponProf"];
			for (auto it = weaponProf.begin(); it != weaponProf.end(); ++it)
			{
				newUnit->weaponProficiencies[weaponNameMap[it.key()]] = int(it.value());
			}
			if (unit.find("SpecialWeapons") != unit.end()) {
				auto specialWeapons = unit["SpecialWeapons"];
				for (const auto& weapon : specialWeapons)
				{
					newUnit->uniqueWeapons.push_back(int(weapon));
				}
			}

			if (unit.find("Skills") != unit.end())
			{
				auto skills = unit["Skills"];
				for (const auto& skill : skills)
				{
					newUnit->skills.push_back(int(skill));
				}
			}
			json inventory = unit["Inventory"];
			for (const auto& item : inventory)
			{
				newUnit->addItem(int(item));
			}
			if (unit.find("ClassPower") != unit.end())
			{
				newUnit->classPower = unit["ClassPower"];
			}
			newUnit->subject.addObserver(unitEvents);
			newUnit->init(gen, distribution);
			newUnit->sprite.uv = &playerUVs[ID];
			newUnit->placeUnit(position.x, position.y);
			return newUnit;
		}
	}
	return nullptr;
}

void PlayerManager::AddUnit(int unitID, glm::vec2& position)
{
	std::ifstream f("BaseStats.json");
	json data = json::parse(f);
	json bases = data["PlayerUnits"];

	auto newUnit = LoadUnit(bases, unitID, position);
	playerUnits.push_back(newUnit);
}

void PlayerManager::Draw(SpriteRenderer* Renderer)
{
	for (int i = 0; i < playerUnits.size(); i++)
	{
		playerUnits[i]->Draw(Renderer);
	}
}

void PlayerManager::Draw(SBatch* Batch)
{
	for (int i = 0; i < playerUnits.size(); i++)
	{
		playerUnits[i]->Draw(Batch);
	}
}

void PlayerManager::Clear()
{
	for (int i = 0; i < playerUnits.size(); i++)
	{
		delete playerUnits[i];
	}
	playerUnits.clear();
}