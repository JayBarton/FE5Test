#pragma once
#include <glm.hpp>

const static int CAMERA_ACTION = 0;
const static int NEW_UNIT_ACTION = 1;
const static int MOVE_UNIT_ACTION = 2;
const static int DIALOGUE_ACTION = 3;
const static int ITEM_ACTION = 4;
const static int NEW_SCENE_UNIT_ACTION = 5;
const static int SCENE_UNIT_MOVE_ACTION = 6;
const static int SCENE_UNIT_REMOVE_ACTION = 7;
const static int START_MUSIC = 8;
const static int STOP_MUSIC = 9;

struct SceneAction
{
	int type;
	//The delay between this action and the next action
	float nextActionDelay = 0;
	SceneAction(int type, float nextActionDelay = 0) : type(type), nextActionDelay(nextActionDelay)
	{
	}
};

struct CameraMove : public SceneAction
{
	CameraMove(int type, glm::vec2 position, float nextActionDelay = 0) : SceneAction(type, nextActionDelay), position(position)
	{
	}
	glm::vec2 position;
};

struct AddUnit : public SceneAction
{
	AddUnit(int type, int unitID, glm::vec2 start, glm::vec2 end, float nextActionDelay = 0) : SceneAction(type, nextActionDelay), unitID(unitID), start(start), end(end)
	{
	}
	int unitID;
	glm::vec2 start;
	glm::vec2 end;
};

struct AddSceneUnit : public SceneAction
{
	AddSceneUnit(int type, int unitID, int team, std::vector<glm::ivec2> path, float nextActionDelay = 0, float nextMoveDelay = -1) :
		SceneAction(type, nextActionDelay), unitID(unitID), team(team), path(path), nextMoveDelay(nextMoveDelay)
	{
	}
	int unitID;
	int team;
	float nextMoveDelay;
	std::vector<glm::ivec2> path;
};

struct UnitMove : public SceneAction
{
	UnitMove(int type, int unitID, glm::vec2 end, float nextActionDelay = 0) : 
		SceneAction(type, nextActionDelay), unitID(unitID), end(end)
	{
	}
	int unitID;
	glm::vec2 end;
};

struct SceneUnitMove : public SceneAction
{
	SceneUnitMove(int type, int unitID, std::vector<glm::ivec2> path, float nextActionDelay = 0, float moveSpeed = 1.0f, int facing = -1) :
		SceneAction(type, nextActionDelay), unitID(unitID), path(path), moveSpeed(moveSpeed), facing(facing)
	{
	}
	int unitID;
	int facing;
	float moveSpeed;
	std::vector<glm::ivec2> path;
};

struct SceneUnitRemove : public SceneAction
{
	SceneUnitRemove(int type, int unitID, float nextActionDelay = 0) : 
		SceneAction(type, nextActionDelay), unitID(unitID)
	{
	}
	int unitID;
};

//not really sure what we're doing with this one
//Maybe it will point to a file the dialogue is in?
struct DialogueAction : public SceneAction
{
	DialogueAction(int type, int ID) : SceneAction(type, nextActionDelay), ID(ID)
	{}
	int ID;
};

struct ItemAction : public SceneAction
{
	ItemAction(int type, int ID) : SceneAction(type, nextActionDelay), ID(ID)
	{}
	int ID;
};

struct StartMusic : public SceneAction
{
	StartMusic(int type, int ID) : SceneAction(type, nextActionDelay), ID(ID)
	{}
	int ID;
};

struct StopMusic : public SceneAction
{
	StopMusic(int type, int nextActionDelay) : SceneAction(type, nextActionDelay)
	{}
};