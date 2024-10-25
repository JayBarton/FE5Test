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

void TextObject::Draw(TextRenderer* textRenderer, SpriteRenderer* Renderer, Camera* camera, bool canShow, bool canShowBox, glm::vec3 color)
{
	if (canShow)
	{
		if (showPortrait)
		{
			Texture2D portraitTexture = ResourceManager::GetTexture("Portraits");
			ResourceManager::GetShader("Nsprite").Use();
			ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());
			Renderer->setUVs(UnitResources::portraitUVs[portraitID][frame]);
			Renderer->DrawSprite(portraitTexture, portraitPosition, 0, glm::vec2(48, 64), glm::mix(glm::vec4(0, 0, 0, 1), glm::vec4(1), fadeValue), mirrorPortrait);
		}
	}
	//Use of mix here is hacky thing to handle battle transitions
	textRenderer->RenderText(displayedText, displayedPosition.x, displayedPosition.y, 1, color, glm::vec2(0, position.y - 10));
}

TextObjectManager::TextObjectManager()
{
	focusedObject = 0;
	delay = normalDelay;
	textObjects.resize(4);

	boxStarts[0] = glm::vec4(72, 48, 56, 24);
	boxStarts[1] = glm::vec4(144, 160, 32, 24);

	textObjects[0].position = glm::vec2(62, 48);
	textObjects[0].displayedPosition = textObjects[0].position;
	textObjects[0].portraitPosition = glm::vec2(32, 96);
	textObjects[0].mirrorPortrait = true;
	textObjects[0].fadeIn = true;
	textObjects[0].boxPosition = glm::vec4(8, 8, 240, 64);
	textObjects[0].extraPosition = glm::vec2(88, 64);

	textObjects[1].portraitPosition = glm::vec2(176, 96);
	textObjects[1].position = glm::vec2(62, 455);
	textObjects[1].displayedPosition = textObjects[1].position;
	textObjects[1].mirrorPortrait = false;
	textObjects[1].fadeIn = true;
	textObjects[1].boxPosition = glm::vec4(8, 160, 240, 64);
	textObjects[1].extraPosition = glm::vec2(152, 152);

	textObjects[2].position = glm::vec2(275.0f, 48.0f);
	textObjects[2].portraitPosition = glm::vec2(16, 16);
	textObjects[2].mirrorPortrait = true;
	textObjects[2].showPortrait = true;
	textObjects[2].showBox = false;
	textObjects[2].displayedPosition = textObjects[2].position;
	textObjects[2].boxPosition = glm::vec4(80, 8, 176, 64);
	textObjects[2].boxDisplayPosition = textObjects[2].boxPosition;

	textObjects[3].position = glm::vec2(250, 391);
	textObjects[3].portraitPosition = glm::vec2(8, 136);
	textObjects[3].mirrorPortrait = true;
	textObjects[3].showPortrait = true;
	textObjects[3].showBox = false;
	textObjects[3].fadeIn = false;
	textObjects[3].displayedPosition = textObjects[3].position;
	textObjects[3].boxPosition = glm::vec4(64, 136, 184, 64);
	textObjects[3].boxDisplayPosition = textObjects[3].boxPosition;
}

void TextObjectManager::setUVs()
{
	auto texture = ResourceManager::GetTexture("UIItems");
	extraUVs = texture.GetUVs(0, 96, 16, 16, 2, 1);
	textObjects[0].extraUV = &extraUVs[0];
	textObjects[1].extraUV = &extraUVs[1];
	battleTextUV = texture.GetUVs(0, 192, 184, 64, 1, 1)[0];
	battleBoxIndicator = texture.GetUVs(0, 112, 9, 5, 2, 1);
}

void TextObjectManager::init(int line/* = 0 */)
{
	for (int i = 0; i < textObjects.size(); i++)
	{
		textObjects[i].displayedText = "";
		textObjects[i].index = 0;
	}
	frame = 0;
	frameDirection = 1;
	currentLine = line;
	auto thisLine = textLines[currentLine];
	focusedObject = thisLine.location;
	textObjects[focusedObject].text = thisLine.text;
	textObjects[focusedObject].active = true;
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
			layer1Alpha = 0;
			fadeIn = true;
		}
	}
	else
	{
		if (focusedObject == 3)
		{
			state = BATTLE_BOX_FADE_IN;
			textObjects[focusedObject].active = false;
			battleBoxAlpha = 0.0f;
			textObjects[focusedObject].fadeValue = 0;
			textObjects[focusedObject].showPortrait = true;
		}
		else
		{
			state = READING_TEXT;
			textObjects[focusedObject].fadeValue = 1.0f;
		}
	}
	if (!showBoxAnyway)
	{
		textObjects[0].boxDisplayPosition = boxStarts[0];
		textObjects[0].boxIn = true;
		textObjects[0].showPortrait = false;
		textObjects[0].showBox = false;
		textObjects[0].frame = 0;
		textObjects[1].boxDisplayPosition = boxStarts[1];
		textObjects[1].boxIn = true;
		textObjects[1].showPortrait = false;
		textObjects[1].showBox = false;
		textObjects[1].frame = 0;
		t = 0;
	}
}

void TextObjectManager::Update(float deltaTime, InputManager& inputManager, bool finished)
{
	if (playMusic && !Mix_PlayingMusic())
	{
		playMusic = false;
		ResourceManager::PlayMusic("BossStart", "BossLoop");
	}
	switch (state)
	{
	case WAITING_ON_INPUT:
		if (focusedObject == 3)
		{
			//animate indicator
			battleBoxTimer += deltaTime;
			float delay = 0.32f;
			if (battleBoxFrame == 1)
			{
				delay = 0.2f;
			}
			if (battleBoxTimer >= delay)
			{
				battleBoxFrame++;
				battleBoxTimer = 0;
				if (battleBoxFrame > 1)
				{
					battleBoxFrame = 0;
				}
			}
		}
		else
		{
			MenuManager::menuManager.AnimateArrow(deltaTime);
		}
		if (inputManager.isKeyPressed(SDLK_RETURN))
		{
			if (focusedObject == 3)
			{
				if (nextOption == 1)
				{
					BattleTextClose();
				}
				else
				{
					GoToNextLine();
				}
			}
			//Do something here
			//The idea is that dependent on what the next option is, I can end text display, display images, move units, etc
			//0: End line
			//1: End dialouge
			//2: Vendor dialogue line
			//3: Exit vendor dialogue
			//4: Skip directly to the next line without waiting on player input
			else if (nextOption == 1)
			{
				if (showBoxAnyway)
				{
					state = LAYER_1_FADE_IN;
					finishing = false;

					fadeIn = false;
				}
				else
				{
					state = PORTRAIT_FADE_OUT;
					finishing = true;
					focusedObject = 0; //Just temporary
				}
			}
			else if (nextOption == 3)
			{
				active = false;
				textObjects[focusedObject].showAnyway = true;
				textObjects[focusedObject].index = 0;
				textObjects[focusedObject].active = false;
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
		if (currentObject.boxIn)
		{
			t += 0.1f;
			currentObject.boxDisplayPosition = glm::mix(boxStarts[focusedObject], currentObject.boxPosition, std::min(1.0f, t));
			if (t >= 2)
			{
				currentObject.boxIn = false;
				currentObject.showPortrait = true;
				t = 0;
			}
		}
		else
		{
			currentObject.fadeValue += deltaTime * 3.333333f;
			if (currentObject.fadeValue >= 1)
			{
				currentObject.fadeValue = 1;
				currentObject.fadeIn = false;
				focusedObject++;
				if (focusedObject >= 2)
				{
					focusedObject = textLines[currentLine].location;
					state = READING_TEXT;
				}
				else
				{
					if (textObjects[focusedObject].portraitID >= 0)
					{
						textObjects[focusedObject].showBox = true;
						textObjects[focusedObject].active = true;
					}
					else
					{
						focusedObject = textLines[currentLine].location;
						state = READING_TEXT;
					}
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
		currentObject.fadeValue -= deltaTime * 3.333333f;
		if (currentObject.fadeValue <= 0)
		{
			currentObject.fadeValue = 0;
			if (continueBattle)
			{
				currentObject.fadeIn = false;
				currentObject.showPortrait = false;
				currentObject.displayedText = "";
				state = BATTLE_BOX_FADE_OUT;
			}
			else if (finishing)
			{
				currentObject.fadeIn = false;
				currentObject.showPortrait = false;
				int nextObject = focusedObject + 1;
				if (nextObject >= 2)
				{
					nextObject = 0;
				}
				if (textObjects[nextObject].active)
				{
					focusedObject = nextObject;
				}
				if (!textObjects[nextObject].showPortrait)
				{
					state = FADE_BOX_OUT;
					boxFadeOut = true;
				}
			}
			else if (midTalkPortraitChange)
			{
				state = PORTRAIT_FADE_IN;
				int realfocusedObject = textLines[currentLine].location;
				auto& agdags = textObjects[realfocusedObject];
				currentObject.portraitID = agdags.text[agdags.index + 3] - '0';
				agdags.index += 4;
				currentObject.showPortrait = true;
				midTalkPortraitChange = false;
			}
			else
			{
				if (!finishing)
				{
					state = PORTRAIT_FADE_IN;
					currentObject.portraitID = textLines[currentLine].portraitID;
					currentObject.showPortrait = true;
				}
			}
		}
	}
		break;
	case FADE_BOX_OUT:
	{
		if (boxFadeOut)
		{
			t += 0.1f;
			if (t > 0.7f)
			{
				t = 0;
				boxFadeOut = false;
				textObjects[focusedObject].displayedText = "";
			}
		}
		else
		{
			auto& currentObject = textObjects[focusedObject];
			t += 0.1f;
			currentObject.boxDisplayPosition = glm::mix(currentObject.boxPosition, boxStarts[focusedObject], std::min(1.0f, t));
			if (t > 1)
			{
				currentObject.showBox = false;
			}
			if (t >= 2)
			{
				currentObject.boxIn = false;
				t = 0;
				textObjects[focusedObject].active = false;
				focusedObject++;
				if (focusedObject >= 2)
				{
					focusedObject = 0;
				}
				if (!textObjects[focusedObject].showBox)
				{
					if (showBG)
					{
						state = FADE_BG_OUT;
					}
					else
					{
						state = LAYER_1_FADE_IN;
					}
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
				else
				{
					textObjects[focusedObject].displayedText = "";
				}
			}
		}
		break;
	}
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
			if (showBoxAnyway)
			{
				textObjects[focusedObject].boxIn = false;
				textObjects[focusedObject].showPortrait = true;
			}
			else
			{
				textObjects[focusedObject].showBox = true;
			}
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
				textObjects[focusedObject].showBox = true;
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
		ResourceManager::GetShader("instance").SetFloat("backgroundFade", layer1Alpha, true);
		break;
	case BATTLE_BOX_FADE_IN:
		battleBoxAlpha += deltaTime;
		if (battleBoxAlpha >= 1)
		{
			battleBoxAlpha = 1;
			textObjects[focusedObject].active = true;
			state = PORTRAIT_FADE_IN;
		}
		break;
	case BATTLE_BOX_FADE_OUT:
		battleBoxAlpha -= deltaTime;
		if (battleBoxAlpha <= 0)
		{
			finishing = true;
			active = false;

		//	textObjects[focusedObject].showAnyway = true;
			textObjects[focusedObject].index = 0;
			textObjects[focusedObject].active = false;
			continueBattle = false;
		}
		break;
	}
}

void TextObjectManager::BattleTextClose()
{
	if (continueBattle)
	{
		state = PORTRAIT_FADE_OUT;
	}
	else
	{
		finishing = true;
		active = false;

		textObjects[focusedObject].showAnyway = true;
		textObjects[focusedObject].index = 0;
		textObjects[focusedObject].active = false;
	}
}

void TextObjectManager::ReadText(InputManager& inputManager, float deltaTime)
{
	if (inputManager.isKeyDown(SDLK_RETURN))
	{
		delay = fastDelay;
	}
	else if (inputManager.isKeyUp(SDLK_RETURN))
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
					textObjects[focusedObject].showAnyway = true;
					currentObject->index = 0;
				}
				else if (nextOption == 4)
				{
					GoToNextLine();
				}
				else if (nextOption == 5)
				{
					focusedObject = currentObject->text[currentObject->index + 2] - '0';
					int newPortraitID = currentObject->text[currentObject->index + 3] - '0';
					if (textObjects[focusedObject].portraitID != newPortraitID)
					{
						if (textObjects[focusedObject].displayedText != "")
						{
							state = REMOVING_TEXT;
							textObjects[focusedObject].displayedPosition = textObjects[focusedObject].position;
						}
						//if object ID was greater than 0, we need to fade out the old portrait and fade in the new one
						else if (textObjects[focusedObject].portraitID >= 0)
						{
							//fade old portrait out
							state = PORTRAIT_FADE_OUT;
						}
						midTalkPortraitChange = true;
					}
				}
				else
				{
					state = WAITING_ON_INPUT;
				}
				animTime = 0.0f;
				frame = 0;
				frameDirection = 1;
				ResourceManager::StopSound(-1);
			}
			else
			{
				currentObject->displayedText += currentObject->text[currentObject->index];
				currentObject->index++;
				ResourceManager::PlaySound("speech");
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
				//if object ID was greater than 0, we need to fade out the old portrait and fade in the new one
				if (textObjects[focusedObject].portraitID >= 0)
				{
					//fade old portrait out
					state = PORTRAIT_FADE_OUT;
				}
				else
				{
					state = PORTRAIT_FADE_IN;
					if (!textObjects[focusedObject].showBox)
					{
						textObjects[focusedObject].showBox = true;
						textObjects[focusedObject].active = true;
					}
					else
					{
						textObjects[focusedObject].showPortrait = true;
					}
					textObjects[focusedObject].portraitID = portraitID;
				}
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
	bool showBox = showPortraits || (showBoxAnyway && state == FADE_BG_IN);
	for (int i = 0; i < textObjects.size(); i++)
	{
		if (textObjects[i].active || textObjects[i].showAnyway)
		{
			glm::vec3 color(1);
			if (i == 3 && textObjects[i].showPortrait)
			{
				auto texture = ResourceManager::GetTexture("UIItems");
				glm::vec2 position(textObjects[3].boxPosition.x, textObjects[3].boxPosition.y);
				glm::vec2 size(textObjects[3].boxPosition.z, textObjects[3].boxPosition.w);
				ResourceManager::GetShader("Nsprite").Use();
				ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());
				Renderer->setUVs(battleTextUV);
				Renderer->DrawSprite(texture, position, 0, size, glm::mix(glm::vec4(0, 0, 0, 1), glm::vec4(1), textObjects[i].fadeValue));
				color = glm::mix(glm::vec3(0, 0, 0), glm::vec3(1), textObjects[i].fadeValue);
			}
			else if (showBox)
			{
				DrawBox(i, Renderer, camera);
			}
			textObjects[i].Draw(textRenderer, Renderer, camera, showPortraits, showBox, color);
		}
	}
	if (state == WAITING_ON_INPUT)
	{
		if (focusedObject == 0)
		{
			MenuManager::menuManager.DrawArrow(glm::ivec2(120, 65));
		}
		else if (focusedObject == 1)
		{
			MenuManager::menuManager.DrawArrow(glm::ivec2(120, 217));
		}
		else if (focusedObject == 3)
		{
			if (textObjects[3].active)
			{
				auto texture = ResourceManager::GetTexture("UIItems");
				ResourceManager::GetShader("Nsprite").Use();
				ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());
				Renderer->setUVs(battleBoxIndicator[battleBoxFrame]);
				Renderer->DrawSprite(texture, glm::vec2(219, 192), 0, glm::vec2(9, 5));
			}
		}
	}
}

void TextObjectManager::DrawBox(int i, SpriteRenderer* Renderer, Camera* camera)
{
	auto current = textObjects[i];
	if (current.showBox)
	{
		Renderer->shader = ResourceManager::GetShader("clip");
		float alpha = 1.0f;
		if (state == FADE_BG_IN)
		{
			alpha = BGAlpha;
		}

		Texture2D texture = ResourceManager::GetTexture("TextBackground");
		ResourceManager::GetShader("clip").Use();
		ResourceManager::GetShader("clip").SetMatrix4("projection", camera->getOrthoMatrix());
		ResourceManager::GetShader("clip").SetVector4f("bounds",
			glm::vec4(current.boxDisplayPosition.x + 1, current.boxDisplayPosition.y + 1,
				current.boxDisplayPosition.x + 1 + current.boxDisplayPosition.z - 2, current.boxDisplayPosition.y + 1 + current.boxDisplayPosition.w - 2)); //clip area should be the border's position + 1 and the border's size -2
		Renderer->setUVs();
		Renderer->DrawSprite(texture, glm::vec2(current.boxPosition.x + 2, current.boxPosition.y + 2), 0, glm::vec2(236, 60), glm::vec4(1, 1, 1, alpha));

		Renderer->shader = ResourceManager::GetShader("sliceFull");

		ResourceManager::GetShader("sliceFull").Use();
		ResourceManager::GetShader("sliceFull").SetMatrix4("projection", camera->getOrthoMatrix());

		texture = ResourceManager::GetTexture("TextBorder");
		glm::vec2 size = glm::vec2(current.boxDisplayPosition.z, current.boxDisplayPosition.w);
		float borderSize = 5.0f;
		ResourceManager::GetShader("sliceFull").SetVector2f("u_dimensions", borderSize / size.x, borderSize / size.y);
		ResourceManager::GetShader("sliceFull").SetVector2f("u_border", borderSize / 24.0f, borderSize / 24.0f);

		Renderer->setUVs();
		Renderer->DrawSprite(texture, glm::vec2(current.boxDisplayPosition.x, current.boxDisplayPosition.y), 0.0f, size, glm::vec4(1, 1, 1, alpha));

		Renderer->shader = ResourceManager::GetShader("Nsprite");

		if (i < 2)
		{
			texture = ResourceManager::GetTexture("UIItems");
			ResourceManager::GetShader("Nsprite").Use();
			ResourceManager::GetShader("Nsprite").SetMatrix4("projection", camera->getOrthoMatrix());
			Renderer->setUVs(extraUVs[i]);
			Renderer->DrawSprite(texture, current.extraPosition, 0, glm::vec2(16, 16), glm::vec4(1, 1, 1, alpha));
		}
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

void TextObjectManager::DrawOverBattleBox(Camera* camera, int shapeVAO)
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", battleBoxAlpha);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(0, 127, 0.0f));
	model = glm::scale(model, glm::vec3(256, 97, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.0f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

bool TextObjectManager::ShowText()
{
	return active || textObjects[focusedObject].showAnyway;
}

void TextObjectManager::EndingScene()
{
	showBoxAnyway = true;
	textObjects[0].active = true;
	textObjects[0].showBox = true;
	textObjects[0].boxIn = false;
	textObjects[0].boxDisplayPosition = textObjects[0].boxPosition;
	textObjects[1].active = true;
	textObjects[1].showBox = true;
	textObjects[1].boxIn = false;
	textObjects[1].boxDisplayPosition = textObjects[1].boxPosition;
}
