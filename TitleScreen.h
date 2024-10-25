#pragma once
#include "Globals.h"

class InputManager;
class SpriteRenderer;
class TextRenderer;
class Camera;
class Cursor;

enum TitleState
{
	FADE_TITLE_IN,
	SHOW_TITLE,
	FADE_TO_MENU,
	IN_MENU
};

struct TitleScreen
{
	TitleScreen()
	{
		state = FADE_TITLE_IN;
	}

	void init();

	void TitleStart();

	void InitialLoad();

	void Update(float deltaTime, InputManager& inputManager);

	void Draw(SpriteRenderer* Renderer, TextRenderer* Text, Camera& camera);

	Subject<int> subject;

	TitleState state;

	float wholeTitleFade = 255.0f;
	float FEAlpha = 1.0f;
	float volumeModifier = 1.0f;
	float menuDelayTimer = 0.0f;

	float flameAnim = 0.0f;

	bool showMenu = false;
	bool foundSuspend = false;
};