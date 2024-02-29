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

struct searchCell
{
	glm::vec2 position;
	int moveCost;
};
struct pathPoint
{
	glm::ivec2 position;
	glm::ivec2 previousPosition;
};
struct vec2Hash
{
	size_t operator()(const glm::vec2& vec) const
	{
		return ((std::hash<float>()(vec.x) ^ (std::hash<float>()(vec.y) << 1)) >> 1);
	}
};

struct Observer
{
	virtual ~Observer() {}
	virtual void onNotify(class Unit* unit) = 0;
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
	void notify(class Unit* unit)
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
	//Values calculated based on unit stats and equipped weapon
	int attackDamage;
	int hitAccuracy;
	int hitAvoid;
	int hitCrit;
	int attackSpeed;
};

struct Mount
{
	Mount(int movementType, int str, int skl, int spd, int def, int mov) : movementType(movementType), str(str), skl(skl), spd(spd), def(def), mov(mov)
	{
	}
	int movementType;
	int str;
	int skl;
	int spd;
	int def;
	int mov;
	bool mounted = true;
};

//I am not sure this needs to be a component of the unit. Since only one unit will ever be moving at a time,
//and all of them will move in the same way, I wonder if I can't just have this be a stand alone struct that
//is used to move any unit.
struct MovementComponent
{
	Unit* owner = nullptr;
	glm::vec2 nextNode;
	glm::vec2 direction;
	std::vector<glm::ivec2> path;
	int current;
	int end;
	bool moving = false;
	void startMovement(const std::vector<glm::ivec2>& path);
	void getNewDirection();
	void Update(float deltaTime);
};

class WeaponData;
struct Unit
{
	const static int PRAYER = 0;
	const static int CONTINUE = 1;
	const static int WRATH = 2;
	const static int VANTAGE = 3;
	const static int ACCOST = 4;
	const static int CHARISMA = 5;

	const static int FOOT = 0;
	const static int HORSE = 1;
	const static int FLYING = 2;

	Unit();
	Unit(std::string Class, std::string Name, int HP, int str, int mag, int skl, int spd, int lck, int def, int bld, int mov) : 
		unitClass(Class), name(Name), maxHP(HP), strength(str), magic(mag), skill(skl), speed(spd), luck(lck), defense(def), build(bld), move(mov)
	{
		currentHP = maxHP;
		movementType = FOOT;
	}
	~Unit();

	//Not really sure where I'm passing this in, but the units should have a reference to the generator I think
	void init(std::mt19937* gen, std::uniform_int_distribution<int>* distribution);

	//Unit properties
	std::string name;
	std::string unitClass; //Not really sure what I'm doing with this
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
	int weaponProficiencies[10] = { 0 };
	std::vector<int> uniqueWeapons;
	std::vector<int> skills;

	int currentHP;
	int experience = 0;

	int maxRange = 0;
	int minRange = 5;

	int equippedWeapon = -1;

	//0 = player
	//1 = enemy
	int team = 0;

	int movementType;

	bool hasMoved = false;

	const static int INVENTORY_SLOTS = 8;
	std::vector<class Item*> inventory;
	std::vector<class Item*> weapons;

	BattleStats battleStats;

	Sprite sprite;
	MovementComponent movementComponent;
	StatGrowths growths;

	Subject subject;

	Mount* mount = nullptr;

	std::mt19937 *gen = nullptr;
	std::uniform_int_distribution<int> *distribution = nullptr;

	void placeUnit(int x, int y);
	void Update(float deltaTime);
	void Draw(SpriteRenderer* Renderer);

	void LevelUp();
	void AddExperience(int exp);
	void LevelEnemy(int level);

	std::vector<Unit*> inRangeUnits(int minRange, int maxRange, int team);

	void addItem(int ID);
	void dropItem(int index);
	void CalculateUnitRange();
	void swapItem(Unit* otherUnit, int otherIndex, int thisIndex);

	void findWeapon();
	//This will only work for equipping from the menu, if I wanted a unit to equip something they picked up this is no good.
	//Also won't handle trading, need to come back to that
	void equipWeapon(int index);
	bool tryEquip(int index);
	bool canUse(const WeaponData& weapon);

	bool hasSkill(int ID);

	int getMovementType();

	void MountAction(bool on);

	Item* GetEquippedItem();

	BattleStats CalculateBattleStats(int weaponID = -1);

	WeaponData GetWeaponData(Item* item);

	//Feel like I don't really want all of this here but it is working for now

	//Both of these are used to draw the movement and attack range of a unit
	std::vector<glm::ivec2> foundTiles;
	std::vector<glm::ivec2> attackTiles;
	//Temporary, just using to visualize tile costs
	std::vector<int> costTile;

	//This can probably be a map of vec2s rather than this pathPoint thing
	std::unordered_map<glm::vec2, pathPoint, vec2Hash> path;
	//Temporary, just using to visualize the path taken
	std::vector<glm::ivec2> drawnPath;

	std::unordered_map<glm::vec2, pathPoint, vec2Hash> FindUnitMoveRange();
	void ClearPathData();
	void addToOpenSet(searchCell newCell, std::vector<searchCell>& checking, std::vector<std::vector<bool>>& checked, std::vector<std::vector<int>>& costs);
	void removeFromOpenList(std::vector<searchCell>& checking);

	void CheckExtraRange(glm::ivec2& checkingTile, std::vector<std::vector<bool>>& checked);
	void CheckAdjacentTiles(glm::vec2& checkingTile, std::vector<std::vector<bool>>& checked, std::vector<searchCell>& checking, searchCell startCell, std::vector<std::vector<int>>& costs);

};