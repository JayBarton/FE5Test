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
	TEXT
};
struct Scene
{
	const static int CAMERA_ACTION = 0;
	const static int NEW_UNIT_ACTION = 1;
	const static int MOVE_UNIT_ACTION = 2;
	const static int DIALOGUE_ACTION = 3;

	struct Activation* activation = nullptr;

	bool playingScene = false;
	int actionIndex = 0;
	std::vector<SceneAction*> actions;
	PathFinder pathFinder;
	SceneState state = WAITING;
	Unit* activeUnit = nullptr;

	TextObject testText;
	TextObject testText2;
	TextObjectManager textManager;

	//completely temporary, just using it to find units for right now.
	//No idea how I will do this going forward, but definitely not with this
	std::unordered_map<int, std::string> nameMap;

	Scene();
	~Scene();
	//I imagine a lot of this will ne set up in the map editor, so this is temporary
	void extraSetup(Subject<int>* subject);
	void init();
	//Want this to be able to handle any unit manager, so might need to make that something that can be inherited.
	void Update(float deltaTime, class PlayerManager* playerManager, Camera& camera, class InputManager& inputManager);

	void ClearActions();
};

struct SceneManager
{
	int currentScene = 0;
	std::vector<Scene*> scenes;
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

struct RoundEvents : public Observer<int>
{
	EnemyTurnEnd* enemyTurnEnd;
	virtual void onNotify(int round)
	{
		enemyTurnEnd->currentRound = round;
		enemyTurnEnd->CheckActivation();
	}
};