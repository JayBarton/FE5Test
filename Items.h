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
	int bonus = 0; //No idea how this is going to work
};
class Unit;

struct ItemUseObserver
{
	virtual ~ItemUseObserver() {}
	virtual void onNotify(Unit* unit, int index) = 0;
};

struct ItemUseSubject
{
	std::vector<ItemUseObserver*> observers;

	void addObserver(ItemUseObserver* observer)
	{
		observers.push_back(observer);
	}
	void removeObserver(ItemUseObserver* observer)
	{
		auto it = std::find(observers.begin(), observers.end(), observer);
		if (it != observers.end())
		{
			delete* it;
			*it = observers.back();
			observers.pop_back();
		}
	}
	void notify(Unit* unit, int index)
	{
		for (int i = 0; i < observers.size(); i++)
		{
			observers[i]->onNotify(unit, index);
		}
	}
};

struct ItemManager
{
	std::vector<Item> items;
	std::unordered_map<int, WeaponData> weaponData;

	void SetUpItems();

	void LoadWeaponData();

	void LoadItems();

	void UseItem(Unit* unit, int index);
	void Heal(Unit* unit, int index);

	ItemUseSubject subject;

	static ItemManager itemManager;
};