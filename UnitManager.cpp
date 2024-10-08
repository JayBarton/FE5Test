#include "UnitManager.h"
#include "UnitResources.h"
#include "Unit.h"
#include "Items.h"

void UnitManager::init(std::mt19937* gen, std::uniform_int_distribution<int>* distribution)
{
	this->gen = gen;
	this->distribution = distribution;
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

void UnitManager::Load(json saveData)
{
	units.resize(saveData.size());
	int current = 0;
	for (const auto& unit : saveData)
	{
		LoadUnitFromSuspend(unit);
	}
}

Unit* UnitManager::LoadUnitFromSuspend(const json& unit)
{
	int ID = unit["ID"];

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
	newUnit->level = stats["Level"];
	newUnit->levelID = unit["LevelID"];

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
		newUnit->addItem(int(item[0]));
		newUnit->inventory[newUnit->inventory.size() - 1]->remainingUses = item[1];
	}
	if (unit.find("ClassPower") != unit.end())
	{
		newUnit->classPower = unit["ClassPower"];
	}
	if (unit.find("DeathMsg") != unit.end())
	{
		newUnit->deathMessage = unit["DeathMsg"];
	}
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
		if (mount["IsMounted"])
		{
			newUnit->MountAction(true);
		}
		else
		{
			newUnit->MountAction(false);
		}
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
	if (!unit["Carried"])
	{
		newUnit->placeUnit(unit["Position"][0], unit["Position"][1]);
	}
	return newUnit;
}
