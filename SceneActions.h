#pragma once
#include <glm.hpp>

const static int CAMERA_ACTION = 0;
const static int NEW_UNIT_ACTION = 1;
const static int MOVE_UNIT_ACTION = 2;
const static int DIALOGUE_ACTION = 3;
const static int ITEM_ACTION = 4;
const static int NEW_SCENE_UNIT_ACTION = 5;
const static int SCENE_UNIT_MOVE_ACTION = 6;

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

struct AddSceneUnit : public SceneAction
{
	AddSceneUnit(int type, int unitID, int team, std::vector<glm::ivec2> path) : 
		SceneAction(type), unitID(unitID), team(team), path(path)
	{
	}
	int unitID;
	int team;
	std::vector<glm::ivec2> path;
};

struct UnitMove : public SceneAction
{
	UnitMove(int type, int unitID, glm::vec2 end) : SceneAction(type), unitID(unitID), end(end)
	{
	}
	int unitID;
	glm::vec2 end;
};

struct SceneUnitMove : public SceneAction
{
	SceneUnitMove(int type, int unitID, std::vector<glm::ivec2> path) : SceneAction(type), unitID(unitID), path(path)
	{
	}
	int unitID;
	std::vector<glm::ivec2> path;
};

//not really sure what we're doing with this one
//Maybe it will point to a file the dialogue is in?
struct DialogueAction : public SceneAction
{
	DialogueAction(int type, int ID) : SceneAction(type), ID(ID)
	{}
	int ID;
};

struct ItemAction : public SceneAction
{
	ItemAction(int type, int ID) : SceneAction(type), ID(ID)
	{}
	int ID;
};