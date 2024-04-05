#include "SceneManager.h"
#include "Camera.h"
#include "Unit.h"
#include "PlayerManager.h"
#include "PathFinder.h"
#include "InputManager.h"

SceneManager::SceneManager()
{
	actions.push_back(0);
	actions.push_back(1);
	actions.push_back(2);
	actions.push_back(3);
}

void SceneManager::init()
{
	playingScene = true;
	testText.position = glm::vec2(100.0f, 100.0f);
	testText.displayedPosition = testText.position;
	testText.charsPerLine = 55;
	testText.nextIndex = 55;

	testText2.position = glm::vec2(100.0f, 400.0f);
	testText2.charsPerLine = 55;
	testText2.nextIndex = 55;
	testText2.displayedPosition = testText2.position;

	textManager.textLines.push_back({ 0, "Dad, it's already started!<0" });
	textManager.textLines.push_back({ 1, "This is bad.<0" });
	textManager.textLines.push_back({ 1, "Even as strong as Eyvale is,\nthese are imperial soldiers...Let's go.<0" });

	textManager.textObjects.push_back(testText);
	textManager.textObjects.push_back(testText2);
	textManager.init();
}

void SceneManager::Update(float deltaTime, PlayerManager* playerManager, Camera& camera, InputManager& inputManager)
{
	if (state!= WAITING)
	{
		switch (state)
		{
		case CAMERA_MOVE:
			if (camera.moving)
			{
				camera.MoveTo(deltaTime, 1.0f);
			}
			else
			{
				actionIndex++;
				state = WAITING;
			}
			break;
		case UNIT_MOVE:
			if (activeUnit->movementComponent.moving)
			{
				//	activeUnit->UpdateMovement(deltaTime, inputManager);
			}
			else
			{
				auto adsfga = activeUnit->sprite.getPosition();
				activeUnit->placeUnit(adsfga.x, adsfga.y);
				activeUnit = nullptr;
				actionIndex++;
				state = WAITING;
			}
			break;
		case TEXT:
			textManager.Update(deltaTime, inputManager);
			if (!textManager.active)
			{
				actionIndex++;
				state = WAITING;
			}
			break;
		}
	}
	else if (actionIndex < actions.size())
	{
		auto currentAction = actions[actionIndex];
		if (currentAction == 0)
		{
			camera.SetCenter(glm::vec2(176, 144));
			state = CAMERA_MOVE;
		}
		else if(currentAction == 1)
		{
			glm::vec2 position = glm::vec2(32, 64);
			playerManager->AddUnit(6, position);
			activeUnit = playerManager->playerUnits[playerManager->playerUnits.size() - 1];
			auto path = pathFinder.findPath(position, glm::vec2(80, 128), 99);
			activeUnit->movementComponent.startMovement(path, 0, false);
			state = UNIT_MOVE;
		}
		else if (currentAction == 2)
		{
			glm::vec2 position = glm::vec2(32, 96);
			playerManager->AddUnit(5, position);
			activeUnit = playerManager->playerUnits[playerManager->playerUnits.size() - 1];
			auto path = pathFinder.findPath(position, glm::vec2(64, 112), 99);
			activeUnit->movementComponent.startMovement(path, 0, false);
			state = UNIT_MOVE;
		}
		else if (currentAction == 3)
		{
			state = TEXT;
			textManager.active = true;
		}
	}
	else
	{
		playingScene = false;
	}
}
