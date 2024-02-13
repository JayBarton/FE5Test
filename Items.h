#pragma once
#include <string>
#include <vector>
#include <unordered_map>

const static int HEAL = 0;
struct Item
{
	int ID = -1;
	std::string name;
	int maxUses;
	int remainingUses;
	int useID;
	bool isWeapon;
	std::string description;
	//Also need a use action for things that can do it, not sure what that will look like
};

struct WeaponData
{
	//int ID; Not sure I need this
	int type;
	int rank;
	int hit;
	int might;
	int crit;
	int maxRange;
	int minRange;
	int weight;
	bool isMagic;
	bool isTome;
	int bonus; //No idea how this is going to work
};

class Unit;
struct ItemManager
{
	std::vector<Item> items;
	std::unordered_map<int, WeaponData> weaponData;

	void SetUpItems();

	void LoadWeaponData();

	void LoadItems();

	void UseItem(Unit* unit, int index, int ID);
	void Heal(Unit* unit, int index);

	static ItemManager itemManager;
};