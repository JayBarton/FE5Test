#include "Items.h"
#include "Unit.h"
#include "Items.h"

#include <iostream>
ItemManager ItemManager::itemManager;

void ItemManager::SetUpItems()
{
	items.push_back({ 0 , "Herb", 3, 3, 0, false, "Heals all HP" });
	items.push_back({ 1 , "Iron Sword", 40, 40, -1, true, "" });
	items.push_back({ 2 , "Light Brand", 60, 60, 1, true, "Used to heal all HP\nRanged, Luck+10" });
	items.push_back({ 3 , "Iron Bow", 40, 40, -1, true, "Effective against flying enemies" });
	items.push_back({ 4 , "Short Bow", 20, 20, -1, true, "Effective against flying enemies" });

	weaponData[1] = { 1, 1, 1, 70, 6, 0, 1, 1, 6, 1 };
	weaponData[2] = { 2, 1, 6, 80, 12, 0, 2, 1, 10, 1 };
	weaponData[3] = { 3, 2, 1, 65, 7, 0, 2, 2, 6, 1 };
	weaponData[4] = { 4, 2, 1, 75, 5, 0, 2, 2, 6, 1 };
}

void ItemManager::UseItem(Unit* unit, int index, int ID)
{
	switch (ID)
	{
	case HEAL:
		Heal(unit, index);
		break;
	}
}

void ItemManager::Heal(Unit* unit, int index)
{
	//Send message to play animation of health bar filling up...Eventually
	unit->currentHP = unit->maxHP;
	auto item = unit->inventory[index];
	item->remainingUses--;
	if (item->remainingUses == 0)
	{
		if (item->isWeapon)
		{
			//break weapon
		}
		else
		{
			unit->inventory.erase(unit->inventory.begin() + index);
		}
	}
}
