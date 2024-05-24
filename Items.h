#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "Globals.h"
const static int HEAL = 0;

struct Item
{
	int ID = -1;
	std::string name;
	int maxUses;
	int remainingUses;
	int useID;
	int value;
	bool isWeapon;
	bool canDrop = true;
	std::string description;
	//Also need a use action for things that can do it, not sure what that will look like
};

struct WeaponData
{
	const static int TYPE_SWORD = 0;
	const static int TYPE_AXE = 1;
	const static int TYPE_LANCE = 2;
	const static int TYPE_BOW = 3;
	const static int TYPE_FIRE = 4;
	const static int TYPE_THUNDER = 5;
	const static int TYPE_WIND = 6;
	const static int TYPE_LIGHT = 7;
	const static int TYPE_DARK = 8;
	const static int TYPE_STAFF = 9;

	//IDs for unique weapons
	const static int LIGHT_BRAND = 6;
	const static int BRAVE_LANCE = 7;

	//int ID; Not sure I need this
	int type = -1;
	int rank = 0;
	int hit = 0;
	int might = 0;
	int crit = 0;
	int maxRange = 0;
	int minRange = 0;
	int weight = 0;
	bool isMagic = false;
	bool isTome = false;
	bool consecutive = false;
	bool flyEffect = false;
	bool horseEffect = false;
	bool armorEffect = false;
	int bonus = 0; //No idea how this is going to work
};
class Unit;


struct ItemManager
{
	std::vector<Item> items;
	std::unordered_map<int, WeaponData> weaponData;

	void SetUpItems();

	void LoadWeaponData();

	void LoadItems();

	WeaponData GetWeaponFromID(int ID);

	void UseItem(Unit* unit, int index);
	void Heal(Unit* unit, int index);

	Subject<Unit*, int> subject;

	static ItemManager itemManager;
};