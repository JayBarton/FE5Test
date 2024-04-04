#pragma once
#include <vector>
#include "PathFinder.h"
class Unit;
class Camera;

enum SceneState
{
	WAITING,
	CAMERA_MOVE,
	UNIT_MOVE
};
struct SceneManager
{
	bool playingScene = false;
	int actionIndex = 0;
	std::vector<int> actions;
	PathFinder pathFinder;
	SceneState state = WAITING;
	Unit* activeUnit = nullptr;

	SceneManager();

	//Want this to be able to handle any unit manager, so might need to make that something that can be inherited.
	void Update(float deltaTime, class PlayerManager* playerManager, Camera& camera, class InputManager& inputManager);
	
};