#include "Items.h"
#include "Unit.h"
#include "Items.h"

#include "csv.h"

#include <iostream>
ItemManager ItemManager::itemManager;

void ItemManager::SetUpItems()
{
	LoadItems();
	LoadWeaponData();
}

void ItemManager::LoadItems()
{
	io::CSVReader<7, io::trim_chars<' '>, io::no_quote_escape<':'>> in("items.csv");
	in.read_header(io::ignore_extra_column, "ID", "name", "maxUses", "useID", "isWeapon", "canDrop", "description");
	int ID;
	std::string name;
	int maxUses;
	int useID;
	int isWeapon;
	int canDrop;
	std::string description;
	while (in.read_row(ID, name, maxUses, useID, isWeapon, canDrop, description)) 
	{
		//csv reader reads in new lines wrong for whatever reason, this fixes it.
		size_t found = description.find("\\n");
		while (found != std::string::npos) 
		{
			description.replace(found, 2, "\n");
			found = description.find("\\n", found + 1);
		}
		items.push_back({ ID, name, maxUses, maxUses, useID, bool(isWeapon), bool(canDrop), description });
	}
}

void ItemManager::LoadWeaponData()
{
	io::CSVReader<12, io::trim_chars<' '>, io::no_quote_escape<':'>> in("weapons.csv");
	in.read_header(io::ignore_extra_column, "ID", "type", "rank", "hit", "might", "crit", "maxRange", "minRange", "weight", "isMagic", "isTome", "bonus");
	int ID;
	int type;
	int rank;
	int hit;
	int might;
	int crit;
	int maxRange;
	int minRange;
	int weight;
	int isMagic;
	int isTome;
	int bonus;
	while (in.read_row(ID, type, rank, hit, might, crit, maxRange, minRange, weight, isMagic, isTome, bonus)) {
		weaponData[ID] = { type, rank, hit, might, crit, maxRange, minRange, weight, bool(isMagic), bool(isTome), bonus };
	}
}

void ItemManager::UseItem(Unit* unit, int index)
{
	int ID = unit->inventory[index]->useID;
	switch (ID)
	{
	case HEAL:
		Heal(unit, index);
		break;
	}
}

void ItemManager::Heal(Unit* unit, int index)
{
	//Send message to play animation of health bar filling up.
	subject.notify(unit, index);
	auto item = unit->inventory[index];
	item->remainingUses--;
	if (item->remainingUses == 0)
	{
		if (item->isWeapon)
		{
			//break weapon
			//Will probably need some sort of notifcation that the weapon broke as well...
		}
		else
		{
			unit->inventory.erase(unit->inventory.begin() + index);
		}
	}
}
