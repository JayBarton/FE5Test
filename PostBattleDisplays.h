#pragma once
#include <vector>
class Unit;
class StatGrowths;
class TextRenderer;
class Camera;

struct PostBattleObserver
{
	virtual ~PostBattleObserver() {}
	virtual void onNotify() = 0;
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
	void notify()
	{
		for (int i = 0; i < observers.size(); i++)
		{
			observers[i]->onNotify();
		}
	}
};

enum DisplayState
{
	NONE,
	ADD_EXPERIENCE,
	LEVEL_UP_NOTE,
	LEVEL_UP
};

struct PostBattleDisplays
{
	DisplayState state = NONE;
	PostBattleSubject subject;
	Unit* leveledUnit = nullptr;

	StatGrowths* preLevelStats = nullptr; //just using this because it has all the data I need

	float levelUpTimer;
	float levelUpTime = 2.5f;

	float experienceTimer;
	float experienceTime = 0.0025f;
	float experienceDisplayTime = 1.0f;
	int displayedExperience = 0;
	int gainedExperience = 0;
	int finalExperience = 0;
	bool displayingExperience = false;

	float levelUpNoteTimer;
	float levelUpNoteTime = 1.0f;

	void AddExperience(Unit* unit, Unit* foe);

	void OnUnitLevel(Unit* unit);

	void Update(float deltaTime);
	void UpdateLevelUpDisplay(float deltaTime);
	void UpdateExperienceDisplay(float deltaTime);
	void Draw(Camera* camera, TextRenderer* Text, int shapeVAO);

	void DrawLevelUpDisplay(Camera* camera, int shapeVAO, TextRenderer* Text);

	void DrawExperienceDisplay(Camera* camera, int shapeVAO, TextRenderer* Text);


};