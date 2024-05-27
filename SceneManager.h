#pragma once
#include <vector>
#include "Globals.h"
#include "SceneActions.h"
#include "PathFinder.h"
#include "TextAdvancer.h"
#include <unordered_map>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

class Unit;
class Camera;

enum SceneState
{
	WAITING,
	CAMERA_MOVE,
	UNIT_MOVE,
	TEXT,
	GET_ITEM
};
struct Scene
{
	struct Activation* activation = nullptr;

	bool playingScene = false;
	bool repeat = false;
	int actionIndex = 0;
	int ID = 0;
	std::vector<SceneAction*> actions;
	PathFinder pathFinder;
	SceneState state = WAITING;
	Unit* activeUnit = nullptr;
	Unit* initiator = nullptr;
	//Not crazy about this. Need some way of telling the scene manager what scene is playing
	class SceneManager* owner = nullptr;

	class VisitObject* visit = nullptr;

	TextObject testText;
	TextObject testText2;
	TextObjectManager textManager;

	Scene();
	~Scene();
	//I imagine a lot of this will ne set up in the map editor, so this is temporary
	void extraSetup(Subject<int>* subject);
	void init();
	//Want this to be able to handle any unit manager, so might need to make that something that can be inherited.
	void Update(float deltaTime, class PlayerManager* playerManager, std::unordered_map<int, Unit*>& sceneUnits,
		Camera& camera, class InputManager& inputManager, class Cursor& cursor, class InfoDisplays& displays);

	void ClearActions();
};

struct SceneManager
{
	int currentScene = 0;
	std::vector<Scene*> scenes;

	bool PlayingScene();
};

struct Activation
{
	int type;
	Scene* owner;
	Activation(Scene* owner, int type) : type(type), owner(owner)
	{}
	virtual void CheckActivation() = 0;

};

struct EnemyTurnEnd : public Activation
{
	int currentRound;
	int round;
	//This has ended up as a bit of a mess, but it works...
	//Need this for cleanup so roundEvents is properly deleted and removed from the subject observers vector
	Subject<int>* subject = nullptr;
	class RoundEvents* roundEvents = nullptr;
	EnemyTurnEnd(Scene* owner, int type, int round);

	//need virtual destructor
	~EnemyTurnEnd();

	virtual void CheckActivation() override
	{
		if (currentRound == round)
		{
			owner->init();
			owner->activation = nullptr;
			delete this;
		}
	}
};

struct TalkActivation : public Activation
{
	int talker;
	int listener;

	TalkActivation(Scene* owner, int type, int talker, int listener);
	virtual void CheckActivation() override
	{
		owner->textManager.talkActivated = true;
		owner->init();
		owner->activation = nullptr;
		delete this;
	}
};

struct VisitActivation : public Activation
{
	VisitActivation(Scene* owner, int type);
	virtual void CheckActivation() override
	{
		owner->textManager.talkActivated = true;
		owner->init();
	}
};

struct RoundEvents : public Observer<int>
{
	EnemyTurnEnd* enemyTurnEnd;
	virtual void onNotify(int round)
	{
		enemyTurnEnd->currentRound = round;
		enemyTurnEnd->CheckActivation();
	}
};