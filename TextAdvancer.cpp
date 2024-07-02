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

void TextObject::Draw(TextRenderer* textRenderer, SpriteRenderer* Renderer, Camera* camera, bool canShow)
{
	textRenderer->RenderText(displayedText, displayedPosition.x, displayedPosition.y, 1, glm::vec3(1), position.y - 10);

	if (canShow && showPortrait)
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
		if (thisLine.BG > 0)
		{
			BG = thisLine.BG;
			state = FADE_GAME_OUT;
			fadeIn = true;
		}
		else
		{
			state = LAYER_1_FADE_IN;
			fadeIn = true;
		}
	}
	else
	{
		state = READING_TEXT;
		textObjects[focusedObject].fadeValue = 1.0f;
	}
}

void TextObjectManager::Update(float deltaTime, InputManager& inputManager, bool finished)
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
				finishing = true;
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
			int portraitID = textLines[currentLine].portraitID;
			if (textObjects[focusedObject].portraitID != portraitID)
			{
				//if object ID was greater than 0, we need to fade out the old portrait and fade in the new one
				if (textObjects[focusedObject].portraitID >= 0)
				{
					//fade old portrait out
					state = PORTRAIT_FADE_OUT;
				}
				else
				{
					state = PORTRAIT_FADE_IN;
					textObjects[focusedObject].showPortrait = true;
					textObjects[focusedObject].portraitID = portraitID;
				}
			}
			else
			{
				state = READING_TEXT;
			}
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
			if (finishing)
			{
				currentObject.fadeIn = false;
				currentObject.showPortrait = false;
				focusedObject++;
				if (focusedObject >= textObjects.size())
				{
					focusedObject = 0;
				}
				if (!textObjects[focusedObject].showPortrait)
				{
					state = LAYER_1_FADE_IN;
					finishing = false;

					fadeIn = false;
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
			else
			{
				if (showBG)
				{
					state = FADE_BG_OUT;
				}
				else if (!finishing)
				{
					state = PORTRAIT_FADE_IN;
					currentObject.portraitID = textLines[currentLine].portraitID;
					currentObject.showPortrait = true;
				}
			}
		}
	}
		break;
	case FADE_GAME_OUT:
		if (fadeIn)
		{
			blackAlpha += deltaTime;
			if (blackAlpha >= 1)
			{
				blackAlpha = 1;
				state = FADE_BG_IN;
				showBG = true;
			}
		}
		else
		{
			blackAlpha -= deltaTime;
			if (blackAlpha <= 0)
			{
				blackAlpha = 0;
				//Not strictly true, need to see if there is more text
				active = false;
			}
		}
		break;
	case FADE_BG_IN:
		BGAlpha += deltaTime;
		if (BGAlpha >= 1)
		{
			BGAlpha = 1;
			state = PORTRAIT_FADE_IN;
			textObjects[focusedObject].showPortrait = true;
		}
		break;
	case FADE_BG_OUT:
		BGAlpha -= deltaTime;
		if (BGAlpha <= 0)
		{
			BGAlpha = 0;
			if (!finished)
			{
				state = FADE_GAME_OUT;
				showBG = false;
				fadeIn = false;
			}
			else
			{
				active = false;
			}
		}
		break;
	case LAYER_1_FADE_IN:
		if (fadeIn)
		{
			layer1Alpha += deltaTime;
			if (layer1Alpha >= layer1MaxAlpha)
			{
				layer1Alpha = layer1MaxAlpha;
				state = PORTRAIT_FADE_IN;
				//Figure out how to put both portraits on
				textObjects[focusedObject].showPortrait = true;
			}
		}
		else
		{
			layer1Alpha -= deltaTime;
			if (layer1Alpha <= 0)
			{
				layer1Alpha = 0;
				active = false;
			}
		}
		break;
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
		else
		{
			int portraitID = textLines[currentLine].portraitID;
			if (textObjects[focusedObject].portraitID != portraitID)
			{
				state = PORTRAIT_FADE_IN;
				textObjects[focusedObject].showPortrait = true;
				textObjects[focusedObject].portraitID = portraitID;
			}
		}
		textObjects[focusedObject].text = textLines[currentLine].text;
		if (!talkActivated)
		{
			textLines[currentLine].speaker->setFocus();
		}
		textObjects[focusedObject].index = 0;

		//Fix this later
		//The idea here was to be able to swtich backgrounds during dialogue, but I'm not sure if I would even want to support that
		//Seems like it would make more sense to just have that be a separate dialogue action.
		if (textLines[currentLine].BG >= 0)
		{
			if (BG != textLines[currentLine].BG)
			{
				BG = textLines[currentLine].BG;
				//showBG = !showBG;
			}
		}
	}
}

void TextObjectManager::Draw(TextRenderer* textRenderer, SpriteRenderer* Renderer, Camera* camera)
{
	if (showBG)
	{
		ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera->getOrthoMatrix());
		Renderer->setUVs();
		Texture2D displayTexture = ResourceManager::GetTexture("EndingBG");
		Renderer->DrawSprite(displayTexture, glm::vec2(0, 72), 0.0f, glm::vec2(256, 88), glm::vec4(1, 1, 1, BGAlpha));
	}

	bool showPortraits = (state != FADE_BG_IN && state != FADE_GAME_OUT && state != FADE_BG_OUT);

	for (int i = 0; i < textObjects.size(); i++)
	{
		textObjects[i].Draw(textRenderer, Renderer, camera, showPortraits);
	}
}

void TextObjectManager::DrawFade(Camera* camera, int shapeVAO)
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", blackAlpha);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(0, 0, 0.0f));
	model = glm::scale(model, glm::vec3(256, 224, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.0f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

void TextObjectManager::DrawLayer1Fade(Camera* camera, int shapeVAO)
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", layer1Alpha);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(0, 0, 0.0f));
	model = glm::scale(model, glm::vec3(256, 224, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.0f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

bool TextObjectManager::ShowText()
{
	return active || showAnyway;
}
