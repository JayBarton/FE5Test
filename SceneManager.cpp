#include "SceneManager.h"
#include "Camera.h"
#include "Unit.h"
#include "PlayerManager.h"
#include "PathFinder.h"
#include "InputManager.h"
#include "Cursor.h"
#include "InfoDisplays.h"
#include "MenuManager.h"
#include "SDL.h"
#include "TextRenderer.h"

#include <sstream>
#include <fstream>

#include "ResourceManager.h"
#include "UnitResources.h"

Scene::Scene(TextObjectManager* textManager) : textManager(textManager)
{

}

Scene::~Scene()
{
	ClearActions();
}

void Scene::init()
{
	playingScene = true;
	owner->currentScene = ID;
	currentDelay = introDelay;
	if (activation->type == 3)
	{
		hideUnits = true;
	}
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
	if (state != WAITING)
	{
		switch (state)
		{
		case CAMERA_MOVE:
			if (camera.moving)
			{
				camera.MoveTo(deltaTime, 2.0f);
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
			if (!textManager->active)
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
				if (initiator->inventory.size() < 7)
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
		{
			bool moving = false;
			if (movementDelay >= 0)
			{
				delayTimer += deltaTime;
				if (delayTimer >= movementDelay)
				{
					delayTimer = 0;
					actionIndex++;
					//need error checking here
					if (actionIndex < actions.size() && actions[actionIndex]->type == NEW_SCENE_UNIT_ACTION)
					{
						AddNewSceneUnit(actions[actionIndex]);
					}
				}
			}
			for (int i = 0; i < introUnits.size(); i++)
			{
				auto currentUnit = introUnits[i];
				if (currentUnit->movementComponent.moving)
				{
					currentUnit->movementComponent.Update(deltaTime, inputManager, currentUnit->speed);
					moving = true;
				}
				else
				{
					currentUnit->sprite.moveAnimate = false;
				}
			}
			if (!moving)
			{
				currentDelay = actions[actionIndex]->nextActionDelay;
				actionIndex++;
				state = WAITING;
			}
			break;
		}
		case SHOWING_TITLE:
		{
			if (openBox)
			{
				boxSize -= 2.4f;
				if (boxSize <= 0)
				{
					boxSize = 0;
				}
				alpha += deltaTime;
				if (alpha >= 0.525f)
				{
					alpha = 0.525f;
				}
				titleTimer += deltaTime;
				if (titleTimer >= 2.0f)
				{
					openBox = false;
					closeBox = true;
					alpha = 0.525f;
				}
			}
			else if (closeBox)
			{
				alpha -= deltaTime;
				if (alpha <= 0)
				{
					alpha = 0;
					actionIndex++;
					state = WAITING;
				}
				boxSize += 2.4f;
				if (boxSize >= 24)
				{
					boxSize = 24;

				}
			}
			ResourceManager::GetShader("instance").SetFloat("backgroundFade", alpha, true);

			break;
		}
		}
	}
	else if (actionIndex < actions.size())
	{
		auto currentAction = actions[actionIndex];
		delayTimer += deltaTime;
		if (delayTimer >= currentDelay)
		{
			delayTimer = 0;
			currentDelay = actions[actionIndex]->nextActionDelay;
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
				activeUnit = playerManager->units[playerManager->units.size() - 1];
				auto path = pathFinder.findPath(action->start, action->end, 99);
				activeUnit->movementComponent.startMovement(path);
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
				activeUnit->movementComponent.startMovement(path);
				state = UNIT_MOVE;

				break;
			}
			case DIALOGUE_ACTION:
			{
				auto action = static_cast<DialogueAction*>(currentAction);
				textManager->textLines.clear();
				std::ifstream f("Levels/Level1Dialogue.json");
				json data = json::parse(f);
				json texts = data["text"];
				for (const auto& text : texts)
				{
					int ID = text["ID"];
					if (ID == action->ID)
					{
						textManager->textObjects[0].portraitID = -1;
						textManager->textObjects[1].portraitID = -1;
						if (text.find("Start_Portraits") != text.end())
						{
							textManager->textObjects[1].portraitID = text["Start_Portraits"][0];
						}
						auto dialogues = text["dialogue"];
						for (const auto& dialogue : dialogues)
						{
							Sprite* speaker;
							if (introUnits.size() > 0)
							{
								if (dialogue["speaker"] >= 0)
								{
									speaker = &introUnits[dialogue["speaker"]]->sprite; //For the intro, we are just going to use insert order as scene ID
								}
								else
								{
									speaker = nullptr;
								}
							}
							else
							{
								speaker = &sceneUnits[dialogue["speaker"]]->sprite;
							}
							int BG = -1;
							if (dialogue.find("BG") != dialogue.end())
							{
								BG = dialogue["BG"];
							}
							SpeakerText text{ speaker, dialogue["location"], dialogue["speech"], dialogue["portrait"] };
							text.BG = BG;
							textManager->textLines.push_back(text); // gotta figure this out
						}
						break;
					}
				}
				/*textManager->textObjects.clear();
				textManager->textObjects.push_back(testText);
				textManager->textObjects.push_back(testText2);*/
				textManager->textObjects[0].fadeIn = true;
				textManager->textObjects[1].fadeIn = true;
				textManager->init();
				textManager->active = true;
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
				AddNewSceneUnit(currentAction);
				state = SCENE_UNIT_MOVE;
				break;
			}
			case SCENE_UNIT_MOVE_ACTION:
			{
				auto action = static_cast<SceneUnitMove*>(currentAction);
				auto currentUnit = introUnits[action->unitID];
				std::reverse(action->path.begin(), action->path.end());
				currentUnit->movementComponent.startMovement(action->path, action->facing);
				state = SCENE_UNIT_MOVE;
				movementDelay = -1; //In the future it is possible I will also want to incorporate this into here, but for now I do not need it
				currentUnit->speed = action->moveSpeed;
				break;
			}
			case SCENE_UNIT_REMOVE_ACTION:
			{
				auto action = static_cast<SceneUnitRemove*>(currentAction);
				introUnits[action->unitID]->draw = false;
				actionIndex++;
				break;
			}
			case START_MUSIC:
			{
				auto action = static_cast<StartMusic*>(currentAction);
				int musicID = action->ID;
				if (musicID == 2)
				{
					ResourceManager::PlayMusic("TurnEndSceneStart", "TurnEndSceneLoop");
				}
				else if (musicID == 3)
				{
					ResourceManager::PlayMusic("RaydrickStart", "RaydrickLoop");
				}
				else if (musicID == 4)
				{
					ResourceManager::PlayMusic("HeroesEnterStart", "HeroesEnterLoop");
				}
				actionIndex++;
				playingMusic = true;
				break;
			}
			case STOP_MUSIC:
				Mix_HookMusicFinished(nullptr);
				Mix_FadeOutMusic(500.0f);
				actionIndex++;
				playingMusic = false;
				break;
			case SHOW_MAP_TITLE:
				titleTimer = 0.0f;
				boxSize = 24;
				openBox = true;
				closeBox = false;
				state = SHOWING_TITLE;
				break;
			}

		}
	}
	else
	{
		delayTimer += deltaTime;
		if (delayTimer >= currentDelay)
		{
			EndScene(cursor);
		}
	}
}

void Scene::EndScene(Cursor& cursor)
{
	if (playingMusic)
	{
		Mix_HookMusicFinished(nullptr);
		Mix_FadeOutMusic(500.0f);
	}
	for (int i = 0; i < introUnits.size(); i++)
	{
		delete introUnits[i];
	}
	introUnits.clear();
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

void Scene::AddNewSceneUnit(SceneAction* currentAction)
{
	auto action = static_cast<AddSceneUnit*>(currentAction);

	SceneUnit* newUnit = new SceneUnit;
	newUnit->sprite.uv = &UnitResources::unitUVs[action->unitID];
	newUnit->team = action->team;

	introUnits.push_back(newUnit);
	newUnit->sprite.SetPosition(action->path[0]);
	std::reverse(action->path.begin(), action->path.end());

	newUnit->movementComponent.owner = &newUnit->sprite;
	newUnit->sprite.setSize(glm::vec2(16));
	newUnit->movementComponent.startMovement(action->path);

	std::ifstream f("BaseStats.json");
	json data = json::parse(f);
	json bases;

	bases = data["classes"];
	for (const auto& unit : bases)
	{
		int ID = unit["ID"];
		if (ID == action->unitID)
		{
			AnimData animData = UnitResources::animData[ID];
			newUnit->sprite.focusedFacing = animData.facing;
			newUnit->sprite.setSize(animData.size);
			newUnit->sprite.drawOffset = animData.offset;
			if (unit.find("Mount") != unit.end())
			{
				newUnit->movementComponent.movementType = Unit::HORSE;
			}
		}
	}
	movementDelay = action->nextMoveDelay;
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

IntroActivation::IntroActivation(Scene* owner, int type) : Activation(owner, type)
{

}

EndingActivation::EndingActivation(Scene* owner, int type) : Activation(owner, type)
{

}

void SceneManager::Update(float deltaTime, PlayerManager* playerManager, 
	std::unordered_map<int, Unit*>& sceneUnits, Camera& camera, 
	InputManager& inputManager, Cursor& cursor, InfoDisplays& displays)
{
	scenes[currentScene]->Update(deltaTime, playerManager, sceneUnits, camera, inputManager, cursor, displays);
}

void SceneManager::DrawTitle(SpriteRenderer* Renderer, TextRenderer* Text, Camera& camera, int shapeVAO, const glm::vec4& uvs)
{
	float boxSize = scenes[currentScene]->boxSize;
	glEnable(GL_STENCIL_TEST);
	glClear(GL_STENCIL_BUFFER_BIT);
	glStencilFunc(GL_ALWAYS, 1, 0xFF);
	glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_FALSE);
	//Then the shape I draw here is my mask
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera.getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 0.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(24, 80 + boxSize, 0.0f));

	model = glm::scale(model, glm::vec3(208, 48 - boxSize * 2, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	//Then when I turn everything off, everything after is drawn in that mask? No idea man. Works though!
	glStencilFunc(GL_EQUAL, 1, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);

	ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera.getOrthoMatrix());
	Renderer->setUVs(uvs);
	Texture2D displayTexture = ResourceManager::GetTexture("UIItems");
	Renderer->DrawSprite(displayTexture, glm::vec2(24, 80), 0.0f, glm::vec2(208, 48));

	Text->RenderTextCenter("Chapter 1: The Warrior of Fiana", 75, 262, 1, 650);
	glDisable(GL_STENCIL_TEST);
}

void SceneManager::ExitScene(Cursor& cursor)
{
	auto activeScene = scenes[currentScene];
	activeScene->EndScene(cursor);
	activeScene->state = WAITING;
}

bool SceneManager::PlayingScene()
{
	if (scenes.size() > 0 && scenes[currentScene]->playingScene)
	{
		return true;
	}
	return false;
}

bool SceneManager::HideUnits()
{
	if (PlayingScene())
	{
		if (scenes[currentScene]->hideUnits)
		{
			return true;
		}
	}
	return false;
}