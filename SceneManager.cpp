#include "SceneManager.h"
#include "Camera.h"
#include "Unit.h"
#include "PlayerManager.h"
#include "PathFinder.h"
#include "InputManager.h"
#include "Cursor.h"

#include <sstream>
#include <fstream>

Scene::Scene()
{

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
	owner->currentScene = ID;
}

void Scene::extraSetup(Subject<int>* subject)
{
	auto type = static_cast<EnemyTurnEnd*>(activation);
	type->roundEvents->enemyTurnEnd = type;
	type->subject = subject;
	subject->addObserver(type->roundEvents);
}

void Scene::Update(float deltaTime, PlayerManager* playerManager, std::unordered_map<int, Unit*>& sceneUnits, Camera& camera, InputManager& inputManager, Cursor& cursor)
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
				activeUnit->UpdateMovement(deltaTime, inputManager);
			}
			else
			{
				auto position = activeUnit->sprite.getPosition();
				activeUnit->placeUnit(position.x, position.y);
				activeUnit->moveAnimate = false;
				activeUnit = nullptr;
				actionIndex++;
				state = WAITING;
			}
			break;
		case TEXT:
			textManager.Update(deltaTime, inputManager, cursor);
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
			activeUnit = sceneUnits[action->unitID];
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
						Unit* speaker = sceneUnits[dialogue["speaker"]];

						textManager.textLines.push_back(SpeakerText{ speaker, dialogue["location"], dialogue["speech"]});
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

TalkActivation::TalkActivation(Scene* owner, int type, int talker, int listener) : Activation(owner, type), talker(talker), listener(listener)
{
}
