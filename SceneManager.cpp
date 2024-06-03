#include "SceneManager.h"
#include "Camera.h"
#include "Unit.h"
#include "PlayerManager.h"
#include "PathFinder.h"
#include "InputManager.h"
#include "Cursor.h"
#include "InfoDisplays.h"
#include "MenuManager.h"

#include <sstream>
#include <fstream>

#include "ResourceManager.h"

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

	UVs.resize(3);
	UVs[0] = ResourceManager::GetTexture("sprites").GetUVs(0, 16, TileManager::TILE_SIZE, TileManager::TILE_SIZE, 3, 1);
	auto extras = ResourceManager::GetTexture("movesprites").GetUVs(640, 128, 32, 32, 4, 4);
	UVs[0].insert(UVs[0].end(), extras.begin(), extras.end());
	UVs[1] = ResourceManager::GetTexture("sprites").GetUVs(48, 48, TileManager::TILE_SIZE, TileManager::TILE_SIZE, 3, 1);
	extras = ResourceManager::GetTexture("movesprites").GetUVs(768, 128, 32, 32, 4, 4);
	UVs[1].insert(UVs[1].end(), extras.begin(), extras.end());
	UVs[2] = ResourceManager::GetTexture("sprites").GetUVs(96, 0, TileManager::TILE_SIZE, TileManager::TILE_SIZE, 3, 1);
	extras = ResourceManager::GetTexture("movesprites").GetUVs(256, 0, 32, 32, 4, 4);
	UVs[2].insert(UVs[2].end(), extras.begin(), extras.end());
}

void Scene::extraSetup(Subject<int>* subject)
{
	auto type = static_cast<EnemyTurnEnd*>(activation);
	type->roundEvents->enemyTurnEnd = type;
	type->subject = subject;
	subject->addObserver(type->roundEvents);
}

void Scene::Update(float deltaTime, PlayerManager* playerManager, std::unordered_map<int, Unit*>& sceneUnits, 
	Camera& camera, InputManager& inputManager, Cursor& cursor, InfoDisplays& displays)
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
				activeUnit->sprite.moveAnimate = false;
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
		case GET_ITEM:
			displays.Update(deltaTime, inputManager);
			if (displays.state == NONE)
			{
				state = WAITING;
				auto action = static_cast<ItemAction*>(actions[actionIndex]);
				//Going to in future want the item action to have a property to specify a specific unit
				//Currently, whoever began the scene will get the item, but I can imagine scenarios in which a scene has many actions that result
				//In some other unit getting the item.
				if (initiator->inventory.size() < 2)
				{
					initiator->addItem(action->ID);
				}
				else
				{
					//Open storage menu
					MenuManager::menuManager.AddFullInventoryMenu(action->ID);
				}
				actionIndex++;
			}
			break;
		case SCENE_UNIT_MOVE:
			bool moving = false;
			for (int i = 0; i < introUnits.size(); i++)
			{
				auto currentUnit = introUnits[i];
				if (currentUnit->movementComponent.moving)
				{
					currentUnit->movementComponent.Update(deltaTime, inputManager);
					moving = true;
				}
				else
				{
					currentUnit->sprite.moveAnimate = false;
				}
			}
			if (!moving)
			{
				actionIndex++;
				state = WAITING;
			}
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
			activeUnit->movementComponent.startMovement(path, 0);
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
			activeUnit->movementComponent.startMovement(path, 0);
			state = UNIT_MOVE;

			break;
		}
		case DIALOGUE_ACTION:
		{
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

						textManager.textLines.push_back(SpeakerText{ speaker, dialogue["location"], dialogue["speech"] });
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
		case GET_ITEM:
		{
			auto action = static_cast<ItemAction*>(currentAction);
			displays.GetItem(action->ID);
			state = GET_ITEM;
			break;
		}
		case NEW_SCENE_UNIT_ACTION:
		{
			auto action = static_cast<AddSceneUnit*>(currentAction);
			
		/*	std::ifstream f("BaseStats.json");
			json data = json::parse(f);
			json bases;
			if (action->team == 0)
			{
				bases = data["PlayerUnits"];
			}
			else
			{
				bases = data["enemies"];
			}*/
			SceneUnit* newUnit = new SceneUnit;
			newUnit->sprite.uv = &UVs[action->unitID];
			newUnit->team = action->team;

			introUnits.push_back(newUnit);
			newUnit->sprite.SetPosition(action->start);
			auto path = pathFinder.findPath(action->start, action->end, 99);
			newUnit->movementComponent.owner = &newUnit->sprite;
			newUnit->sprite.setSize(glm::vec2(16));
			newUnit->movementComponent.startMovement(path, 99);
			state = SCENE_UNIT_MOVE;
			break;
		}
		}
	}
	else
	{
		if (initiator)
		{
			//I don't think this check really works in the case of an ai unit initiating dialogue. That won't happen in the first level,
			//But it is worth noting I think
			if (cursor.selectedUnit->isMounted() && cursor.selectedUnit->mount->remainingMoves > 0)
			{
				cursor.GetRemainingMove();
				MenuManager::menuManager.mustWait = true;
			}
			else
			{
				cursor.Wait();
			}
			initiator = nullptr;
		}
		playingScene = false;
		if (repeat)
		{
			actionIndex = 0;
		}
		else
		{
			ClearActions();
		}
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

	if (visit)
	{
		visit->toDelete = true;
		visit = nullptr;
	}
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

VisitActivation::VisitActivation(Scene* owner, int type) : Activation(owner, type)
{

}

bool SceneManager::PlayingScene()
{
	if (scenes.size() > 0 && scenes[currentScene]->playingScene)
	{
		return true;
	}
	return false;
}
