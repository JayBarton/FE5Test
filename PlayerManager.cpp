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

#include "UnitResources.h"
using json = nlohmann::json;

void PlayerManager::init(std::mt19937* gen, std::uniform_int_distribution<int>* distribution, Observer<Unit*>* unitEvents, std::unordered_map<int, Unit*>* sceneUnits)
{
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
	//This needs some work. It only works by chance in my level because the units happen to be
	//In the correct order(0-8) in the map file, it screws up if they aren't.
	//This would be problematic anyway if I got to a point where I was placing units at the level start.
	for (int i = 0; i < numberOfStarts; i++)
	{
		map >> type >> position.x >> position.y;
		auto newUnit = LoadUnit(bases, type, position);
		playerUnits[currentUnit] = newUnit;
		currentUnit++;
	}
	playerUnits[0]->currentHP = 1;
	playerUnits[0]->build = 2;
	playerUnits[2]->currentHP = 1;

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
			int classID = unit["ClassID"];
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
			newUnit->classID = classID;
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
			if (unit.find("SpecialWeapons") != unit.end()) 
			{
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
			if (unit.find("DeathMsg") != unit.end())
			{
				newUnit->deathMessage = unit["DeathMsg"];
			}
			newUnit->subject.addObserver(unitEvents);
			newUnit->init(gen, distribution);

			if (unit.find("Mount") != unit.end())
			{
				json mount = unit["Mount"];

				newUnit->movementType = Unit::FOOT;
				newUnit->mount = new Mount(Unit::HORSE, mount["AnimID"], mount["Str"], mount["Skl"], mount["Spd"], mount["Def"], mount["Mov"]);
				json mountProf = mount["WeaponProf"];
				for (auto it = mountProf.begin(); it != mountProf.end(); ++it)
				{
					newUnit->mount->weaponProficiencies[weaponNameMap[it.key()]] = int(it.value());
				}
				newUnit->MountAction(true);
			}
			else
			{
				newUnit->sprite.uv = &UnitResources::unitUVs[classID];
				AnimData animData = UnitResources::animData[classID];
				newUnit->sprite.focusedFacing = animData.facing;
				newUnit->sprite.setSize(animData.size);
				newUnit->sprite.drawOffset = animData.offset;
			}
			newUnit->portraitID = ID;
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

void PlayerManager::Draw(SBatch* Batch, std::vector<Sprite>& carrySprites)
{
	for (int i = 0; i < playerUnits.size(); i++)
	{
		playerUnits[i]->Draw(Batch);
		if (!playerUnits[i]->sprite.moveAnimate && playerUnits[i]->carriedUnit)
		{
			Sprite carrySprite;
			carrySprite.SetPosition(playerUnits[i]->sprite.getPosition() + 6.0f);
			carrySprite.setSize(glm::vec2(8));
			carrySprite.currentFrame = playerUnits[i]->carriedUnit->team;
			carrySprites.push_back(carrySprite);
		}
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