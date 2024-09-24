#pragma once
#include <vector>
#include "Globals.h"
#include "SceneActions.h"
#include "PathFinder.h"
#include "TextAdvancer.h"
#include <unordered_map>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

class Unit;
class Camera;
class Cursor;
class InputManager;
class PlayerManager;
class InfoDisplays;

enum SceneState
{
	WAITING,
	CAMERA_MOVE,
	UNIT_MOVE,
	TEXT,
	GET_ITEM,
	SCENE_UNIT_MOVE,
	SHOWING_TITLE
};
struct Scene
{
	struct Activation* activation = nullptr;

	bool playingScene = false;
	bool repeat = false;
	bool playingMusic = false;
	bool hideUnits = false;
	int actionIndex = 0;
	int ID = 0;
	std::vector<SceneAction*> actions;
	PathFinder pathFinder;
	SceneState state = WAITING;
	Unit* activeUnit = nullptr;
	Unit* initiator = nullptr;
	//Not crazy about this. Need some way of telling the scene manager what scene is playing
	class SceneManager* owner = nullptr;

	class VisitObject* visit = nullptr;

	//TextObject testText;
	//TextObject testText2;
	TextObjectManager* textManager;

	//Hopefully here temporarily
	std::vector<struct SceneUnit*> introUnits;
	std::vector<std::vector<glm::vec4>> UVs;

	//The initial delay before starting the scene
	//This is only going to be relevant in a few cases
	float introDelay = 0.0f;
	float delayTimer = 0.0f;
	float currentDelay = 0.0f;
	//The delay between a new movement action starting, since multiple enemies can move at once
	float movementDelay = -1.0f;

	//Not sure where to put this. Don't really want it here but where else could it be?
	float titleTimer = 0.0f;
	float boxSize = 24;
	float alpha = 0.0f;
	bool openBox = false;
	bool closeBox = false;

	Scene(TextObjectManager* textManager);
	~Scene();
	//I imagine a lot of this will ne set up in the map editor, so this is temporary
	void extraSetup(Subject<int>* subject);
	void init();
	//Want this to be able to handle any unit manager, so might need to make that something that can be inherited.
	void Update(float deltaTime, PlayerManager* playerManager, std::unordered_map<int, Unit*>& sceneUnits,
		Camera& camera, InputManager& inputManager, Cursor& cursor, InfoDisplays& displays);

	void EndScene(Cursor& cursor);

	void AddNewSceneUnit(SceneAction* currentAction);

	void ClearActions();
};

struct SceneManager
{
	int currentScene = 0;
	std::vector<Scene*> scenes;

	void Update(float deltaTime, PlayerManager* playerManager, std::unordered_map<int, Unit*>& sceneUnits,
		Camera& camera, InputManager& inputManager, Cursor& cursor, InfoDisplays& displays);

	void DrawTitle(SpriteRenderer* Renderer, TextRenderer* Text, Camera& camera, int shapeVAO, const glm::vec4& uvs);

	void ExitScene(Cursor& cursor);

	bool PlayingScene();
	bool HideUnits();
};

struct Activation
{
	int type;
	Scene* owner;
	Activation(Scene* owner, int type) : type(type), owner(owner)
	{}
	virtual ~Activation() {}
	virtual void CheckActivation() = 0;

};

struct EnemyTurnEnd : public Activation
{
	int currentRound;
	int round;
	//This has ended up as a bit of a mess, but it works...
	//Need this for cleanup so roundEvents is properly deleted and removed from the subject observers vector
	Subject<int>* subject = nullptr;
	class RoundEvents* roundEvents = nullptr;
	EnemyTurnEnd(Scene* owner, int type, int round);

	//need virtual destructor
	~EnemyTurnEnd() override;

	virtual void CheckActivation() override
	{
		if (currentRound == round)
		{
			owner->init();
			owner->activation = nullptr;
			delete this;
		}
	}
};

struct TalkActivation : public Activation
{
	int talker;
	int listener;

	TalkActivation(Scene* owner, int type, int talker, int listener);
	virtual void CheckActivation() override
	{
		owner->textManager->talkActivated = true;
		owner->init();
		owner->activation = nullptr;
		delete this;
	}
};

struct VisitActivation : public Activation
{
	VisitActivation(Scene* owner, int type);
	virtual void CheckActivation() override
	{
		owner->textManager->talkActivated = true;
		owner->init();
	}
};

struct IntroActivation : public Activation
{
	IntroActivation(Scene* owner, int type);
	virtual void CheckActivation() override
	{
		owner->init();
	}
};
//I understand this and intro are identical. Not sure if I may need different data for the two in the future, so leave em like this for now
//In the future I can imagine a level that has multiple seize points.
//Currently, the level only has one, which will automatically trigger the ending scene,
//If I wanted to expand and have a level with multiple scene points, I would probably need some extra data for what capturing a seize point does
struct EndingActivation : public Activation
{
	EndingActivation(Scene* owner, int type);
	virtual void CheckActivation() override
	{
		owner->textManager->talkActivated = true;
		owner->init();
	}
};


struct RoundEvents : public Observer<int>
{
	EnemyTurnEnd* enemyTurnEnd;
	virtual void onNotify(int round)
	{
		enemyTurnEnd->currentRound = round;
		enemyTurnEnd->CheckActivation();
	}
};