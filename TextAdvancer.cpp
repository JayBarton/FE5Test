#include "TextAdvancer.h"
#include "TextRenderer.h"
#include "InputManager.h"
#include "TextRenderer.h"
#include "Globals.h"
#include "Unit.h"
#include "MenuManager.h"
#include "SpriteRenderer.h"

#include "ResourceManager.h"
#include "UnitResources.h"
#include "Camera.h"

#include <SDL.h>

TextObject::TextObject()
{
	index = 0;
	displayedText = "";
}

void TextObject::Draw(TextRenderer* textRenderer, SpriteRenderer* Renderer, Camera* camera)
{
	textRenderer->RenderText(displayedText, displayedPosition.x, displayedPosition.y, 1, glm::vec3(1), position.y - 10);

	if (showPortrait)
	{
		Texture2D portraitTexture = ResourceManager::GetTexture("Portraits");
		auto portraitUVs = portraitTexture.GetUVs(48, 64);
		ResourceManager::GetShader("Nsprite").Use();
		ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());
		Renderer->setUVs(UnitResources::portraitUVs[portraitID][frame]);
		Renderer->DrawSprite(portraitTexture, portraitPosition, 0, glm::vec2(48, 64), glm::mix(glm::vec4(0, 0, 0, 1), glm::vec4(1, 1, 1, 1), fadeValue), mirrorPortrait);
	}
}

TextObjectManager::TextObjectManager()
{
	focusedObject = 0;
	delay = normalDelay;
}

void TextObjectManager::init(int line/* = 0 */)
{
	currentLine = line;
	auto thisLine = textLines[currentLine];
	focusedObject = thisLine.location;
	textObjects[focusedObject].text = thisLine.text;
	//Proof of concept. Should be error checking here probably
	if (thisLine.speaker)
	{
		if (!talkActivated)
		{
			thisLine.speaker->setFocus();
		}
	}
	textObjects[focusedObject].portraitID = thisLine.portraitID;
	active = false;
	if (textObjects[focusedObject].fadeIn == true)
	{
		state = PORTRAIT_FADE_IN;
		textObjects[focusedObject].showPortrait = true;
	}
	else
	{
		state = READING_TEXT;
		textObjects[focusedObject].fadeValue = 1.0f;
	}
}

void TextObjectManager::Update(float deltaTime, InputManager& inputManager)
{
	switch (state)
	{
	case WAITING_ON_INPUT:
		if (inputManager.isKeyPressed(SDLK_RETURN))
		{
			//Do something here
			//The idea is that dependent on what the next option is, I can end text display, display images, move units, etc
			//0: End line
			//1: End dialouge
			//2: Vendor dialogue line
			//3: Exit vendor dialogue
			//4: Skip directly to the next line without waiting on player input
			if (nextOption == 1)
			{
				state = PORTRAIT_FADE_OUT;
				focusedObject = 0; //Just temporary
			}
			else if (nextOption == 3)
			{
				active = false;
				showAnyway = true;
				textObjects[focusedObject].index = 0;
			}
			else
			{
				GoToNextLine();
			}
		}
		break;
	case REMOVING_TEXT:
		displayTimer += deltaTime;
		textObjects[focusedObject].displayedPosition.y -= 200.0f * deltaTime;
		if (displayTimer >= 0.5f)
		{
			textObjects[focusedObject].displayedText = "";
			textObjects[focusedObject].displayedPosition = textObjects[focusedObject].position;

			displayTimer = 0;
			state = READING_TEXT;
		}
		break;
	case PORTRAIT_FADE_IN:
	{
		auto& currentObject = textObjects[focusedObject];
		currentObject.fadeValue += deltaTime * 1.5f;
		if (currentObject.fadeValue >= 1)
		{
			currentObject.fadeValue = 1;
			currentObject.fadeIn = false;
			focusedObject++;
			if (focusedObject >= textObjects.size())
			{
				focusedObject = textLines[currentLine].location;
				state = READING_TEXT;
			}
			else
			{
				if (textObjects[focusedObject].portraitID >= 0)
				{
					textObjects[focusedObject].showPortrait = true;
				}
				else
				{
					focusedObject = textLines[currentLine].location;
					state = READING_TEXT;
				}
			}
		}
		break;
	}
	case READING_TEXT:
		ReadText(inputManager, deltaTime);
		break;
	case PORTRAIT_FADE_OUT:
	{
		auto& currentObject = textObjects[focusedObject];
		currentObject.fadeValue -= deltaTime * 1.5f;
		if (currentObject.fadeValue <= 0)
		{
			currentObject.fadeValue = 0;
			currentObject.fadeIn = false;
			currentObject.showPortrait = false;
			focusedObject++;
			if (focusedObject >= textObjects.size())
			{
				active = false;
				if (talkActivated)
				{
					talkActivated = false;
				}
				else
				{
					textLines[currentLine].speaker->moveAnimate = false;
				}
			}
			else
			{
				if (textObjects[focusedObject].portraitID >= 0)
				{
					textObjects[focusedObject].showPortrait = true;
				}
				else
				{
					active = false;
					if (talkActivated)
					{
						talkActivated = false;
					}
					else
					{
						textLines[currentLine].speaker->moveAnimate = false;
					}
				}
			}
		}

		break;
	}
	}
}

void TextObjectManager::ReadText(InputManager& inputManager, float deltaTime)
{
	if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		delay = fastDelay;
	}
	else if (inputManager.isKeyReleased(SDLK_RETURN))
	{
		delay = normalDelay;
	}

	displayTimer += deltaTime;
	auto currentObject = &textObjects[focusedObject];

	animTime += deltaTime;
	float animationDelay = 0.0f;
	animationDelay = 8.0f / 60.0f;
	if (animTime >= animationDelay)
	{
		animTime = 0;
		if (frameDirection > 0)
		{
			if (frame < 2)
			{
				frame++;
			}
			else
			{
				frameDirection = -1;
				frame--;
			}
		}
		else
		{
			if (frame > 0)
			{
				frame--;
			}
			else
			{
				frameDirection = 1;
				frame++;
			}
		}
	}

	if (displayTimer >= delay)
	{
		if (currentObject->index < currentObject->text.size())
		{
			displayTimer = 0;
			char nextChar = currentObject->text[currentObject->index];
			if (nextChar == '<')
			{
				nextOption = currentObject->text[currentObject->index + 1] - '0';
				if (nextOption == 2)
				{
					active = false;
					showAnyway = true;
					currentObject->index = 0;
				}
				else if (nextOption == 4)
				{
					GoToNextLine();
				}
				else
				{
					state = WAITING_ON_INPUT;
				}
				frame = 0;
				frameDirection = 1;
			}
			//	else if (nextChar == '\n')
			//{

			//	}
			else
			{
				currentObject->displayedText += currentObject->text[currentObject->index];
				currentObject->index++;
			}

		}
	}
	currentObject->frame = frame;
}

void TextObjectManager::GoToNextLine()
{
	state = READING_TEXT;
	if (!talkActivated)
	{
		textLines[currentLine].speaker->moveAnimate = false;
	}
	currentLine++;
	if (currentLine < textLines.size())
	{
		focusedObject = textLines[currentLine].location;
		if (textObjects[focusedObject].displayedText != "")
		{
			state = REMOVING_TEXT;
			textObjects[focusedObject].displayedPosition = textObjects[focusedObject].position;
		}
		textObjects[focusedObject].text = textLines[currentLine].text;
		if (!talkActivated)
		{
			textLines[currentLine].speaker->setFocus();
		}
		textObjects[focusedObject].index = 0;
		textObjects[focusedObject].portraitID = textLines[currentLine].portraitID;

	}
}

void TextObjectManager::Draw(TextRenderer* textRenderer, SpriteRenderer* Renderer, Camera* camera)
{
	for (int i = 0; i < textObjects.size(); i++)
	{
		textObjects[i].Draw(textRenderer, Renderer, camera);
	}
}

bool TextObjectManager::ShowText()
{
	return active || showAnyway;
}
