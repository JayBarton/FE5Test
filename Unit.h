#pragma once
#include <string>
#include <random>
#include <iostream>
#include <chrono>
#include <random>
#include <vector>
#include <unordered_map>
#include "Sprite.h"
#include "Globals.h"
class SpriteRenderer;
class InputManager;

struct pathCell
{
	glm::ivec2 position;
	int moveCost;
	glm::ivec2 previousPosition;

};

struct vec2Hash
{
	size_t operator()(const glm::vec2& vec) const
	{
		return ((std::hash<float>()(vec.x) ^ (std::hash<float>()(vec.y) << 1)) >> 1);
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
	//God I have no idea how to handle this
	//0 physical, 1 magic
	int attackType = 0;
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
	bool mounted;

	int remainingMoves;
};

//I am not sure this needs to be a component of the unit. Since only one unit will ever be moving at a time,
//and all of them will move in the same way, I wonder if I can't just have this be a stand alone struct that
//is used to move any unit.
struct MovementComponent
{
	class Unit* owner = nullptr;
	glm::vec2 nextNode;
	glm::vec2 direction;
	std::vector<glm::ivec2> path;
	int current;
	int end;
	bool moving = false;
	void startMovement(const std::vector<glm::ivec2>& path, int moveCost, bool remainingMove);
	void getNewDirection();
	void Update(float deltaTime, InputManager& inputManager);
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
	Unit(std::string Class, std::string Name, int ID, int HP, int str, int mag, int skl, int spd, int lck, int def, int bld, int mov) : 
		unitClass(Class), name(Name), ID(ID), maxHP(HP), strength(str), magic(mag), skill(skl), speed(spd), luck(lck), defense(def), build(bld), move(mov)
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
	int ID;
	//Using this for scene actions/dialogue. This is mainly for the rare case in which a generic enemy does something in a scene
	int sceneID = -1;
	int maxHP;
	int strength;
	int magic;
	int skill;
	int speed;
	int luck;
	int defense;
	int build;
	int move;

	int mountStr = 0;
	int mountSkl = 0;
	int mountSpd = 0;
	int mountDef = 0;
	int mountMov = 0;

	int carryingMalus = 1;

	int level = 1;
	int weaponProficiencies[10] = { 0 };
	std::vector<int> uniqueWeapons;
	std::vector<int> skills;

	int currentHP;
	int experience = 0;

	//Used for experience calculations
	//Most non-promoted classes have a power of 3
	int classPower = 3;

	int maxRange = 0;
	int minRange = 5;

	int equippedWeapon = -1;

	//0 = player
	//1 = enemy
	int team = 0;

	int movementType;

	bool hasMoved = false;
	//Also for experience calculations, but probably unneeded for this project
	bool isPromoted = false;

	bool isDead = false;
	bool isCarried = false;

	//Not sure if this is staying here
	bool active = false;
	bool stationary = false;
	bool boss = false;
	//Will activate the enemy if they enter a specific range from the enemy if true
	//Enemy activates on unit entering attack range otherwise
	int activationType = 0;


	const static int INVENTORY_SLOTS = 8;
	std::vector<class Item*> inventory;
	std::vector<class Item*> weapons;

	BattleStats battleStats;

	Sprite sprite;
	MovementComponent movementComponent;
	StatGrowths growths;

	Subject<Unit*> subject;

	Mount* mount = nullptr;

	Unit* carriedUnit = nullptr;

	std::mt19937 *gen = nullptr;
	std::uniform_int_distribution<int> *distribution = nullptr;

	void placeUnit(int x, int y);
	void Update(float deltaTime, int idleFrame);
	void UpdateMovement(float deltaTime, InputManager& inputManager);
	void Draw(SpriteRenderer* Renderer);
	void Draw(class SBatch* Renderer);

	bool Dying(float deltaTime);

	void LevelUp();
	int CalculateExperience(Unit* enemy);
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

	bool isMounted();

	int getMovementType();

	void MountAction(bool on);

	Item* GetEquippedItem();
	WeaponData GetEquippedWeapon();

	int getStrength();
	int getMagic();
	int getSkill();
	int getSpeed();
	int getLuck();
	int getDefense();
	int getBuild();
	int getMove();

	BattleStats CalculateBattleStats(int weaponID = -1);
	void CalculateMagicDefense(const WeaponData& unitWeapon, BattleStats& unitNormalStats, float attackDistance);
	WeaponData GetWeaponData(Item* item);

	//Feel like I don't really want all of this here but it is working for now

	//Both of these are used to draw the movement and attack range of a unit
	std::vector<glm::ivec2> foundTiles;
	std::vector<glm::ivec2> attackTiles;
	//Using this to allow enemies to trade with each other
	std::vector<Unit*> tradeUnits;
	//Temporary, just using to visualize tile costs
	std::vector<int> costTile;

	//This can probably be a map of vec2s rather than this pathPoint thing
	std::unordered_map<glm::vec2, pathCell, vec2Hash> path;
	//Temporary, just using to visualize the path taken
	std::vector<glm::ivec2> drawnPath;

	void StartTurn(class InfoDisplays& displays, class Camera* camera);
	void EndTurn();

	std::unordered_map<glm::vec2, pathCell, vec2Hash> FindUnitMoveRange();
	void PathSearchSetUp(std::vector<std::vector<int>>& costs, std::vector<std::vector<bool>>& checked, glm::vec2& position, std::vector<pathCell>& checking);
	void ClearPathData();
	void addToOpenSet(pathCell newCell, std::vector<pathCell>& checking, std::vector<std::vector<bool>>& checked, std::vector<std::vector<int>>& costs);
	void removeFromOpenList(std::vector<pathCell>& checking);

	std::unordered_map<glm::vec2, pathCell, vec2Hash> FindRemainingMoveRange();
	void CheckRemainingAdjacentTiles(glm::vec2& checkingTile, std::vector<std::vector<bool>>& checked, std::vector<pathCell>& checking, pathCell startCell, std::vector<std::vector<int>>& costs);

	void CheckExtraRange(glm::ivec2& checkingTile, std::vector<std::vector<bool>>& checked);
	void CheckAdjacentTiles(glm::vec2& checkingTile, std::vector<std::vector<bool>>& checked, std::vector<pathCell>& checking, pathCell startCell, std::vector<std::vector<int>>& costs);


	//More horrible duplicate code
	//Really don't like how I have three versions of this, but each of them is different enough that it's hard to figure out how I could make them generic
	std::unordered_map<glm::vec2, pathCell, vec2Hash> FindApproachMoveRange(std::vector<Unit*>& foundUnits, int range);
	void CheckApproachAdjacentTiles(glm::vec2& checkingTile, std::vector<std::vector<bool>>& checked, std::vector<pathCell>& checking, pathCell startCell, std::vector<std::vector<int>>& costs, std::vector<Unit*>& foundUnits, int range);

	//Seriously, another one.
	//REALLY need to find a better way of doing this shit
	std::vector<Unit*> inRangeUnits(int team);
	void CheckRangeTiles(glm::vec2& checkingTile, std::vector<std::vector<bool>>& checked, std::vector<pathCell>& checking, pathCell startCell, std::vector<std::vector<int>>& costs, std::vector<glm::vec2>& foundTiles, std::vector<Unit*>& units, int team);


};