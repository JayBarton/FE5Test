#pragma once
#include <string>
#include <random>
#include <vector>
#include <unordered_map>
#include "Sprite.h"
#include "Globals.h"


#include <nlohmann/json.hpp>

// for convenience
using json = nlohmann::json;

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
	Mount(int movementType, int ID, int str, int skl, int spd, int def, int mov) : movementType(movementType), ID(ID), str(str), skl(skl), spd(spd), def(def), mov(mov)
	{
	}
	int movementType;
	int ID;
	int str;
	int skl;
	int spd;
	int def;
	int mov;
	bool mounted;

	int weaponProficiencies[10] = { 0 };

	int remainingMoves;
};

//While I am still not sure if this needs to be standalone, multiple units can in fact move at one time in some circumstances
struct MovementComponent
{
	class Sprite* owner = nullptr;
	//Need this for the movement type
	class Unit* unit = nullptr;
	int movementType = 0;
	glm::vec2 nextNode;
	glm::vec2 direction;
	glm::vec2 previousDirection;
	std::vector<glm::ivec2> path;
	int current;
	int end;
	int facing;

	int currentChannel = -1;

	bool moving = false;
	void startMovement(const std::vector<glm::ivec2>& path, int facing = -1);
	void getNewDirection(int facing);
	void Update(float deltaTime, InputManager& inputManager, float inputSpeed = 0);
	void HandleMovementSound();
};

struct TalkData
{
	class Scene* scene;
	int talkTarget;
};

//This struct represents just the data that needs to be loaded from json
struct JSONUnit 
{
	JSONUnit(){}
	JSONUnit(std::string Class, std::string Name, int ID, int HP, int str, int mag, int skl, int spd, int lck, int def, int bld, int mov) :
		className(Class), name(Name), ID(ID), HP(HP), str(str), mag(mag), skl(skl), spd(spd), lck(lck), def(def), bld(bld), mov(mov)
	{}
	int ID;
	std::string name;
	std::string className;
	int HP;
	int str;
	int mag;
	int skl;
	int spd;
	int lck;
	int def;
	int bld;
	int mov;
	int classPower = 3;
	int weaponProficiencies[10] = { 0 };
	std::vector<int> skills;
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

	Unit(JSONUnit jUnit) : 
		Unit(jUnit.className, jUnit.name, jUnit.ID, jUnit.HP, jUnit.str, jUnit.mag, jUnit.skl, jUnit.spd, jUnit.lck, jUnit.def, jUnit.bld, jUnit.mov)
	{
		std::copy(std::begin(jUnit.weaponProficiencies), std::end(jUnit.weaponProficiencies), weaponProficiencies);
		skills = jUnit.skills;
		classPower = jUnit.classPower;
	}

	~Unit();

	//Not really sure where I'm passing this in, but the units should have a reference to the generator I think
	void init(std::mt19937* gen, std::uniform_int_distribution<int>* distribution);

	//Unit properties
	std::string name;
	std::string unitClass; //Not really sure what I'm doing with this
	int ID;
	//What I ultimately want is for ID to be an identifyer for the specific unit, while class ID is what class they are. This will hopefully be helpful for mounting/dismounting and promotions/class changes
	//Currently this is not in use for the enemies at all
	int classID;
	//Using this for scene actions/dialogue. This is mainly for the rare case in which a generic enemy does something in a scene
	int sceneID = -1;
	int portraitID;
	//Yet another ID. Need this one for figuring out what units are carrying units when loading a suspended game
	int levelID = -1;

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
	//To see if carrying a unit reduces move speed as well
	int buildMalus = 1;

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

	//Not sure if this is staying here
	bool active = false;
	bool stationary = false;
	bool boss = false;

	//Not crazy about this, want this to hide the unit when they are held. 
	// I can't just use the carried variable because I need them still visible for animation.
	bool hide = false;

	bool tookHit = false;
	bool hitRecover = false;

	float hitA = 0.0f;

	const static int INVENTORY_SLOTS = 8;
	std::vector<class Item*> inventory;
	std::vector<class Item*> weapons;

	BattleStats battleStats;

	Sprite sprite;
	MovementComponent movementComponent;
	StatGrowths growths;

	Subject<Unit*> subject;

	Mount* mount = nullptr;

	//Unit being held by this unit
	Unit* carriedUnit = nullptr;
	//Unit holding this unit
	Unit* carryingUnit = nullptr;

	std::string deathMessage = "";

	std::mt19937 *gen = nullptr;
	std::uniform_int_distribution<int> *distribution = nullptr;

	void placeUnit(int x, int y);
	void SetFocus();
	void Update(float deltaTime, int idleFrame);
	void UpdateMovement(float deltaTime, InputManager& inputManager);
	void Draw(SpriteRenderer* Renderer);
	void Draw(class SBatch* Renderer, glm::vec2 position = glm::vec2(-1), bool drawAnyway = false);

	bool Dying(float deltaTime);

	void TakeDamage(int damage);

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
	bool canUse(int ID);

	bool hasSkill(int ID);

	bool isMounted();

	int getMovementType();

	void MountAction(bool on);

	void startMovement(const std::vector<glm::ivec2>& path, int moveCost, bool remainingMove);

	void carryUnit(Unit* unitToCarry);
	void holdUnit(Unit* unitToCarry);
	void releaseUnit();

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
	std::vector<glm::ivec2> endTiles;

	//All of these are for AI units
	std::vector<Unit*> tradeUnits;
	struct Vendor* storeTarget = nullptr;
	//Will activate the enemy if they enter a specific range from the enemy if true
	//Enemy activates on unit entering attack range otherwise
	int activationType = 0;
	//End AI Units

	//Temporary, just using to visualize tile costs
	std::vector<int> costTile;

	//This can probably be a map of vec2s rather than this pathPoint thing
	std::unordered_map<glm::vec2, pathCell, vec2Hash> path;
	//Temporary, just using to visualize the path taken
	std::vector<glm::ivec2> drawnPath;

	std::vector<TalkData> talkData;

	std::vector<Item*> GetOrderedWeapons();

	void StartTurn(class InfoDisplays& displays, class Camera* camera);
	void EndTurn();

	std::unordered_map<glm::vec2, pathCell, vec2Hash> FindUnitMoveRange();
	void PathSearchSetUp(std::vector<std::vector<int>>& costs, std::vector<std::vector<bool>>& checked, glm::vec2& position, std::vector<pathCell>& checking);
	void ClearPathData();
	void addToOpenSet(pathCell newCell, std::vector<pathCell>& checking, std::vector<std::vector<bool>>& checked, std::vector<std::vector<int>>& costs);
	void removeFromOpenList(std::vector<pathCell>& checking);

	std::unordered_map<glm::vec2, pathCell, vec2Hash> FindRemainingMoveRange();
	void CheckRemainingAdjacentTiles(glm::vec2& checkingTile, std::vector<std::vector<bool>>& checked, std::vector<pathCell>& checking, pathCell startCell, std::vector<std::vector<int>>& costs);

	bool CheckAdjacentTiles(glm::vec2& checkingTile, std::vector<std::vector<bool>>& checked, std::vector<pathCell>& checking, pathCell startCell, std::vector<std::vector<int>>& costs);
	//FUCKING ANOTHER ONE AHHHHHH
	void CheckAttackableTiles(glm::vec2& checkingTile, std::vector<std::vector<bool>>& checked, std::vector<pathCell>& checking, pathCell startCell, std::vector<std::vector<int>>& costs);

	//More horrible duplicate code
	//Really don't like how I have three versions of this, but each of them is different enough that it's hard to figure out how I could make them generic
	std::unordered_map<glm::vec2, pathCell, vec2Hash> FindApproachMoveRange(std::vector<Unit*>& foundUnits, int range);
	void CheckApproachAdjacentTiles(glm::vec2& checkingTile, std::vector<std::vector<bool>>& checked, std::vector<pathCell>& checking, pathCell startCell, std::vector<std::vector<int>>& costs, std::vector<Unit*>& foundUnits, int range);

	//Seriously, another one.
	//REALLY need to find a better way of doing this shit
	//Using this one to find units that are within adjacent attack range
	std::vector<Unit*> inRangeUnits(int team);
	void CheckRangeTiles(glm::vec2& checkingTile, std::vector<std::vector<bool>>& checked, std::vector<pathCell>& checking, pathCell startCell, std::vector<std::vector<int>>& costs, std::vector<glm::vec2>& foundTiles, std::vector<Unit*>& units, int team);
};

struct SceneUnit
{
	bool draw = true;
	int team;
	float speed = 1.0f;
	Sprite sprite;
	MovementComponent movementComponent;
};