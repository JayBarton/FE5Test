#include "SceneManager.h"
#include "Camera.h"
#include "Unit.h"
#include "PlayerManager.h"
#include "PathFinder.h"
#include "InputManager.h"

#include <sstream>
#include <fstream>

Scene::Scene()
{
	//Completely temporary
	nameMap[5] = "Dagda";
	nameMap[6] = "Tania";
	nameMap[7] = "Marty";
}

Scene::~Scene()
{
	ClearActions();
}

void Scene::init()
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
}

void Scene::extraSetup(Subject<int>* subject)
{
	auto type = new EnemyTurnEnd(this, 1, 2);
	type->roundEvents->enemyTurnEnd = type;
	type->subject = subject;
	subject->addObserver(type->roundEvents);
	activation = type;
}

void Scene::Update(float deltaTime, PlayerManager* playerManager, Camera& camera, InputManager& inputManager)
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
		switch (currentAction->type)
		{
		case CAMERA_ACTION:
		{
			auto action = static_cast<CameraMove*>(currentAction);
			camera.SetCenter(action->position);
			state = CAMERA_MOVE;
			break;
		}
		case NEW_UNIT_ACTION:
		{
			auto action = static_cast<AddUnit*>(currentAction);
			playerManager->AddUnit(action->unitID, action->start);
			activeUnit = playerManager->playerUnits[playerManager->playerUnits.size() - 1];
			auto path = pathFinder.findPath(action->start, action->end, 99);
			activeUnit->movementComponent.startMovement(path, 0, false);
			state = UNIT_MOVE;
			break;
		}
		case MOVE_UNIT_ACTION:
		{
			auto action = static_cast<UnitMove*>(currentAction);
			for (int i = 0; i < playerManager->playerUnits.size(); i++)
			{
				auto thisUnit = playerManager->playerUnits[i];
				if (thisUnit->name == nameMap[action->unitID])
				{
					activeUnit = thisUnit;
					break;
				}
			}
			auto position = activeUnit->sprite.getPosition();
			TileManager::tileManager.removeUnit(position.x, position.y);
			auto path = pathFinder.findPath(position, action->end, 99);
			activeUnit->movementComponent.startMovement(path, 0, false);
			state = UNIT_MOVE;
		}
			break;
		case DIALOGUE_ACTION:
			auto action = static_cast<DialogueAction*>(currentAction);
			textManager.textLines.clear();
			std::ifstream f("Levels/Level1Dialogue.json");
			json data = json::parse(f);
			json texts = data["text"];
			for (const auto& text : texts)
			{
				int ID = text["ID"];
				if (ID == action->ID)
				{
					auto dialogues = text["dialogue"];
					for (const auto& dialogue : dialogues)
					{
						textManager.textLines.push_back(SpeakerText{ dialogue["location"], dialogue["speech"]});
					}
					break;
				}
			}
			textManager.textObjects.clear();
			textManager.textObjects.push_back(testText);
			textManager.textObjects.push_back(testText2);
			textManager.init();
			textManager.active = true;
			state = TEXT;

			break;
		}
	}
	else
	{
		playingScene = false;
		ClearActions();
	}
}

void Scene::ClearActions()
{
	for (int i = 0; i < actions.size(); i++)
	{
		delete actions[i];
	}
	actions.clear();

	if (activation)
	{
		delete activation;
	}
	activation = nullptr;
}

EnemyTurnEnd::EnemyTurnEnd(Scene* owner, int type, int round) : Activation(owner, type), round(round)
{
	currentRound = 0;
	roundEvents = new RoundEvents();
}

EnemyTurnEnd::~EnemyTurnEnd()
{
	subject->removeObserver(roundEvents);
}