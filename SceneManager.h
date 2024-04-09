#pragma once
#include <vector>
#include "PathFinder.h"
#include "TextAdvancer.h"
#include <unordered_map>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

class Unit;
class Camera;

struct SceneAction
{
	int type;
	SceneAction(int type) : type(type)
	{
	}
};

struct CameraMove : public SceneAction
{
	CameraMove(int type, glm::vec2 position) : SceneAction(type), position(position)
	{
	}
	glm::vec2 position;
};

struct AddUnit : public SceneAction
{
	AddUnit(int type, int unitID, glm::vec2 start, glm::vec2 end) : SceneAction(type), unitID(unitID), start(start), end(end)
	{
	}
	int unitID;
	glm::vec2 start;
	glm::vec2 end;
};

struct UnitMove : public SceneAction
{
	UnitMove(int type, int unitID, glm::vec2 end) : SceneAction(type), unitID(unitID), end(end)
	{
	}
	int unitID;
	glm::vec2 end;
};

//not really sure what we're doing with this one
//Maybe it will point to a file the dialogue is in?
struct DialogueAction : public SceneAction
{
	DialogueAction(int type, int ID) : SceneAction(type), ID(ID)
	{}
	int ID;
};

enum SceneState
{
	WAITING,
	CAMERA_MOVE,
	UNIT_MOVE,
	TEXT
};
struct SceneManager
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

	SceneManager();
	~SceneManager();
	void init();
	void RoundEnded(int currentRound);
	//Want this to be able to handle any unit manager, so might need to make that something that can be inherited.
	void Update(float deltaTime, class PlayerManager* playerManager, Camera& camera, class InputManager& inputManager);

	void ClearActions();
};

struct Activation
{
	int type;
	SceneManager* owner;
	Activation(SceneManager* owner, int type) : type(type), owner(owner)
	{}
	virtual void CheckActivation() = 0;

};

struct EnemyTurnEnd : public Activation
{
	int currentRound;
	int round;
	EnemyTurnEnd(SceneManager* owner, int type, int round) : Activation(owner, type), round(round)
	{
		currentRound = 0;
	}

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