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

	if (portraitID >= 0)
	{
		Texture2D portraitTexture = ResourceManager::GetTexture("Portraits");
		auto portraitUVs = portraitTexture.GetUVs(48, 64);
		ResourceManager::GetShader("Nsprite").Use();
		ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());
		Renderer->setUVs(UnitResources::portraitUVs[portraitID][frame]);
		Renderer->DrawSprite(portraitTexture, portraitPosition, 0, glm::vec2(48, 64), glm::vec4(1), mirrorPortrait);
	}
}

TextObjectManager::TextObjectManager()
{
	focusedObject = 0;
	delay = normalDelay;
}

void TextObjectManager::init(int line/* = 0 */ )
{
	currentLine = line;
	textObjects[focusedObject].text = textLines[currentLine].text;
	//Proof of concept. Should be error checking here probably
	if (textLines[currentLine].speaker)
	{
		if (!talkActivated)
		{
			textLines[currentLine].speaker->setFocus();
		}
	}
	//So I'm thinking textObjects will have a bool that will tell them to fade the portrait in, and I'll set both of them here
	//Going forward, if the portraitID changes, that bool will be set to true again.
	textObjects[focusedObject].portraitID = textLines[currentLine].portraitID;
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
			//0: End line
			//1: End dialouge
			//2: 
			if (nextOption == 1)
			{
				active = false;
				waitingOnInput = false;
				if (talkActivated)
				{
					talkActivated = false;
				}
				else
				{
					textLines[currentLine].speaker->moveAnimate = false;
				}
			}
			else if (nextOption == 3)
			{
				active = false;
				waitingOnInput = false;
				showAnyway = true;
				textObjects[focusedObject].index = 0;
			}
			else
			{
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
						removingText = true;
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
				currentObject->displayedText += currentObject->text[currentObject->index];
				currentObject->index++;
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
					else
					{
						waitingOnInput = true;
					}
					frame = 0;
					frameDirection = 1;
				}
				else if (nextChar == '\n')
				{
					
				}
			}
		}
		currentObject->frame = frame;
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
