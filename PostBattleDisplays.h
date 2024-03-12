#pragma once
#include <vector>
class Unit;
class StatGrowths;
class TextRenderer;
class Camera;

struct PostBattleObserver
{
	virtual ~PostBattleObserver() {}
	virtual void onNotify(int ID) = 0;
};

struct PostBattleSubject
{
	std::vector<PostBattleObserver*> observers;

	void addObserver(PostBattleObserver* observer)
	{
		observers.push_back(observer);
	}
	void removeObserver(PostBattleObserver* observer)
	{
		auto it = std::find(observers.begin(), observers.end(), observer);
		if (it != observers.end())
		{
			delete* it;
			*it = observers.back();
			observers.pop_back();
		}
	}
	void notify(int ID)
	{
		for (int i = 0; i < observers.size(); i++)
		{
			observers[i]->onNotify(ID);
		}
	}
};

enum DisplayState
{
	NONE,
	ADD_EXPERIENCE,
	LEVEL_UP_NOTE,
	LEVEL_UP,
	HEALING_ANIMATION,
	HEALING_BAR,
	ENEMY_USE,
	ENEMY_TRADE
};
class EnemyManager;
struct PostBattleDisplays
{
	DisplayState state = NONE;
	PostBattleSubject subject;
	Unit* leveledUnit = nullptr;
	//Ugh. Figure out how to gain access to this better later
	EnemyManager* enemyManager = nullptr;

	StatGrowths* preLevelStats = nullptr; //just using this because it has all the data I need

	float displayTimer = 0.0f;
	float levelUpTime = 2.5f;

	float experienceTime = 0.0025f;
	float experienceDisplayTime = 1.0f;
	int displayedExperience = 0;
	int gainedExperience = 0;
	int finalExperience = 0;
	bool displayingExperience = false;
	bool finishedHealing = false;

	float levelUpNoteTime = 1.0f;

	int itemToUse;
	int displayedHP;
	float healAnimationTime = 1.0f;
	float healDisplayTime = 0.035f;

	float textDisplayTime = 1.0f;

	void AddExperience(Unit* unit, Unit* foe);
	void StartUse(Unit* unit, int index);
	void EnemyUse(Unit* enemy, int index);
	void EnemyTrade(EnemyManager* enemyManager);

	void OnUnitLevel(Unit* unit);

	void Update(float deltaTime);
	void UpdateHealthBarDisplay(float deltaTime);
	void UpdateLevelUpDisplay(float deltaTime);
	void UpdateExperienceDisplay(float deltaTime);
	void Draw(Camera* camera, TextRenderer* Text, int shapeVAO);

	void DrawHealthBar(Camera* camera, int shapeVAO, TextRenderer* Text);

	void DrawHealAnimation(Camera* camera, int shapeVAO);

	void DrawLevelUpDisplay(Camera* camera, int shapeVAO, TextRenderer* Text);

	void DrawExperienceDisplay(Camera* camera, int shapeVAO, TextRenderer* Text);


};