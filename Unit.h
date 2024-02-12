#pragma once
#include <string>
#include <random>
#include <iostream>
#include <chrono>
#include <random>
#include <vector>
#include <unordered_map>
#include "Sprite.h"
class SpriteRenderer;

struct Observer
{
	virtual ~Observer() {}
	virtual void onNotify(const class Unit& unit) = 0;
};

struct Subject
{
	std::vector<Observer*> observers;

	void addObserver(Observer* observer)
	{
		observers.push_back(observer);
	}
	void removeObserver(Observer* observer)
	{
		auto it = std::find(observers.begin(), observers.end(), observer);
		if (it != observers.end())
		{
			delete* it;
			*it = observers.back();
			observers.pop_back();
		}
	}
	void notify(const class Unit& unit)
	{
		for (int i = 0; i < observers.size(); i++)
		{
			observers[i]->onNotify(unit);
		}
	}
};

struct StatGrowths
{
	int maxHP;
	int strength;
	int magic;
	int skill;
	int speed;
	int luck;
	int defense;
	int build;
	int move;
};

struct BattleStats
{
	BattleStats()
	{

	}
	//Values calculated based on unit stats and equipped weapon
	int attackDamage;
	int hitAccuracy;
	int hitAvoid;
	int hitCrit;
	int attackSpeed;
};

struct Unit
{
	Unit();
	~Unit();

	//Not really sure where I'm passing this in, but the units should have a reference to the generator I think
	void init(std::mt19937* gen, std::uniform_int_distribution<int>* distribution);

	//Unit properties
	std::string name;
	int unitClass; //This will determine the growths I think
	int maxHP;
	int strength;
	int magic;
	int skill;
	int speed;
	int luck;
	int defense;
	int build;
	int move;
	int level = 1;
	//Also need weapon ranks. I'm thinking this might be some sort of array
	//Also need skills, no idea how to implement yet.

	int currentHP;
	int experience = 0;

	int maxRange = 0;
	int minRange = 5;

	//0 = player
	//1 = enemy
	int team = 0;

	const static int INVENTORY_SLOTS = 8;
	std::vector<class Item*> inventory;
	std::vector<class Item*> weapons;

	BattleStats battleStats;

	Sprite sprite;
	StatGrowths growths;

	Subject subject;

	std::mt19937 *gen = nullptr;
	std::uniform_int_distribution<int> *distribution = nullptr;

	void placeUnit(int x, int y);
	void Update(float deltaTime);
	void Draw(SpriteRenderer* Renderer);

	void LevelUp();
	void AddExperience(int exp);

	void addItem(int ID);
	void dropItem(int index);
	void swapItem(std::vector<Item*>& otherInventory, int otherIndex, int thisIndex);
	//This will only work for equipping from the menu, if I wanted a unit to equip something they picked up this is no good.
	//Also won't handle trading, need to come back to that
	void equipWeapon(int index);

	BattleStats CalculateBattleStats(int weaponID = -1);

	class WeaponData GetWeaponData(Item* item);
};