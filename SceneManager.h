#pragma once
#include <vector>
#include "PathFinder.h"
#include "TextAdvancer.h"

class Unit;
class Camera;

enum SceneState
{
	WAITING,
	CAMERA_MOVE,
	UNIT_MOVE,
	TEXT
};
struct SceneManager
{
	bool playingScene = false;
	int actionIndex = 0;
	std::vector<int> actions;
	PathFinder pathFinder;
	SceneState state = WAITING;
	Unit* activeUnit = nullptr;

	TextObject testText;
	TextObject testText2;
	TextObjectManager textManager;

	SceneManager();

	void init();

	//Want this to be able to handle any unit manager, so might need to make that something that can be inherited.
	void Update(float deltaTime, class PlayerManager* playerManager, Camera& camera, class InputManager& inputManager);
	
};