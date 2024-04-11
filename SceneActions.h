#pragma once
#include <glm.hpp>
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