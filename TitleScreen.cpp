#include "TitleScreen.h"
#include "InputManager.h"
#include "SpriteRenderer.h"
#include "TextRenderer.h"
#include "MenuManager.h"
#include "Camera.h"
#include "ResourceManager.h"
#include "Cursor.h"
#include <fstream>

#include <SDL.h>

void TitleScreen::init()
{
	InitialLoad();

	TitleStart();
}

void TitleScreen::TitleStart()
{
	ResourceManager::PlayMusic("TitleScreen");

	std::ifstream suspend("suspendData.json");
	if (suspend.good())
	{
		suspend.close();
		foundSuspend = true;
	}
}

void TitleScreen::InitialLoad()
{
	ResourceManager::LoadTexture("Textures/Backgrounds/Title1.png", "Title1");
	ResourceManager::LoadTexture("Textures/Backgrounds/Title2.png", "Title2");
	ResourceManager::LoadTexture("Textures/Backgrounds/Title3.png", "Title3");
	ResourceManager::LoadTexture("Textures/Backgrounds/TitleButton.png", "TitleButton");

	ResourceManager::LoadMusic("Sounds/TitleScreen.ogg", "TitleScreen");
	//Need to load these here since they're used in the menu
	ResourceManager::LoadSound("Sounds/optionSelect1.wav", "optionSelect1");
	//Can't get the original resource, this is close enough
	ResourceManager::LoadSound("Sounds/minimapOpen.wav", "minimapOpen");

	ResourceManager::LoadSound("Sounds/titleconfirm.wav", "titleconfirm");
}

void TitleScreen::Update(float deltaTime, InputManager& inputManager)
{
	flameAnim += deltaTime * 5.0f;
	ResourceManager::GetShader("fire").SetFloat("t", flameAnim , true);

	switch (state)
	{
	case FADE_TITLE_IN:
		wholeTitleFade -= 4.25f;
		if (wholeTitleFade <= 0.0f)
		{
			wholeTitleFade = 0.0f;
			state = SHOW_TITLE;
		}
		break;
	case SHOW_TITLE:
		//Whatever needs to be done to handle the fire effect on "Fire Emblem" should be a function called by both this state and the above
		if (inputManager.isKeyPressed(SDLK_RETURN))
		{
			state = FADE_TO_MENU;
			ResourceManager::PlaySound("minimapOpen");
		}
		break;
	case FADE_TO_MENU:
		if (!showMenu)
		{
			FEAlpha -= 0.03333f;
			volumeModifier -= deltaTime * 1.5f;
			if (FEAlpha <= 0)
			{
				volumeModifier = 0.25f;
				FEAlpha = 0;
				showMenu = true;
			}
			Mix_VolumeMusic(128 * volumeModifier);
		}
		else
		{
			menuDelayTimer += deltaTime;
			if (menuDelayTimer >= 0.583f)
			{
				MenuManager::menuManager.AddTitleMenu(&subject, foundSuspend);
				state = IN_MENU;
			}
		}

		break;
	case IN_MENU:
		MenuManager::menuManager.menus.back()->CheckInput(inputManager, deltaTime);
		break;
	}
}

void TitleScreen::Draw(SpriteRenderer* Renderer, TextRenderer* Text, Camera& camera)
{
	ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera.getOrthoMatrix());
	ResourceManager::GetShader("Nsprite").SetFloat("subtractValue", wholeTitleFade);

	Renderer->setUVs();
	Texture2D displayTexture = ResourceManager::GetTexture("Title1");
	Renderer->DrawSprite(displayTexture, glm::vec2(0, 0), 0.0f, glm::vec2(256, 224));

	//Draw main background
	if (state != IN_MENU)
	{
		if (state == FADE_TITLE_IN || state == SHOW_TITLE)
		{
			//Draw title2
			displayTexture = ResourceManager::GetTexture("Title2");
			Renderer->DrawSprite(displayTexture, glm::vec2(12, 99), 0.0f, glm::vec2(229, 119));
		}
		//Draw title3
		displayTexture = ResourceManager::GetTexture("Title3");
		Renderer->DrawSprite(displayTexture, glm::vec2(22, 88), 0.0f, glm::vec2(220, 61), glm::vec4(1, 1, 1, FEAlpha));
		if (state != FADE_TO_MENU)
		{
			Renderer->shader = ResourceManager::GetShader("fire");
			Renderer->setUVs();

			ResourceManager::GetShader("fire").Use().SetMatrix4("projection", camera.getOrthoMatrix());
			ResourceManager::GetShader("fire").SetFloat("subtractValue", wholeTitleFade);
			Renderer->DrawSprite(displayTexture, glm::vec2(22, 88), 0.0f, glm::vec2(220, 61), glm::vec4(1, 1, 1, FEAlpha));
			Renderer->shader = ResourceManager::GetShader("Nsprite");

		}
	}
	else
	{
		MenuManager::menuManager.menus.back()->Draw();
	}
}