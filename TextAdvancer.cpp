#include "TextAdvancer.h"
#include "TextRenderer.h"
#include "InputManager.h"
#include "TextRenderer.h"
#include "Globals.h"
#include "Unit.h"
#include <SDL.h>

TextObject::TextObject()
{
	index = 0;
	displayedText = "";
}

void TextObject::Draw(TextRenderer* textRenderer)
{
	textRenderer->RenderText(displayedText, displayedPosition.x, displayedPosition.y, 1, glm::vec3(1), position.y - 10);
}

TextObjectManager::TextObjectManager()
{
	focusedObject = 0;
	delay = normalDelay;
}

void TextObjectManager::init()
{
	currentLine = 0;
	textObjects[focusedObject].text = textLines[currentLine].text;
	//Proof of concept. Should be error checking here probably
	textLines[currentLine].speaker->sprite.color.r = 0;
	active = false;
}

void TextObjectManager::Update(float deltaTime, InputManager& inputManager)
{
	if (waitingOnInput)
	{
		if (inputManager.isKeyPressed(SDLK_RETURN))
		{
			//Do something here
			//The idea is that dependent on what the next option is, I can end text display, display images, move units, etc
			if (nextOption == 1)
			{
				active = false;
				waitingOnInput = false;
				textLines[currentLine].speaker->sprite.color.r = 1;

			}
			else
			{
				textLines[currentLine].speaker->sprite.color.r = 1;
				currentLine++;
				if (currentLine < textLines.size())
				{
					focusedObject = textLines[currentLine].location;
					if (textObjects[focusedObject].displayedText != "")
					{
						removingText = true;
						textObjects[focusedObject].displayedPosition = textObjects[focusedObject].position;
					}
					textObjects[focusedObject].text = textLines[currentLine].text;
					textLines[currentLine].speaker->sprite.color.r = 0;
					textObjects[focusedObject].index = 0;
				}
				waitingOnInput = false;
			}
		}
	}
	else if (removingText)
	{
		displayTimer += deltaTime;
		textObjects[focusedObject].displayedPosition.y -= 200.0f * deltaTime;
		if (displayTimer >= 0.5f)
		{
			textObjects[focusedObject].displayedText = "";
			textObjects[focusedObject].displayedPosition = textObjects[focusedObject].position;

			displayTimer = 0;
			removingText = false;
		}
	}
	else
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
		if (displayTimer >= delay)
		{
			if (currentObject->index < currentObject->text.size())
			{
				displayTimer = 0;
				currentObject->displayedText += currentObject->text[currentObject->index];
				currentObject->index++;
				char nextChar = currentObject->text[currentObject->index];
				if (nextChar == '<')
				{
					waitingOnInput = true;
					nextOption = currentObject->text[currentObject->index + 1] - '0';
				}
			}
		}
	}
}

void TextObjectManager::Draw(TextRenderer* textRenderer)
{
	for (int i = 0; i < textObjects.size(); i++)
	{
		textObjects[i].Draw(textRenderer);
	}
}