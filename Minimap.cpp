#include "Minimap.h"
#include "ResourceManager.h"
#include "Camera.h"
#include "TileManager.h"
#include "InputManager.h"
#include "SDL.h"
#include "Settings.h"

void Minimap::Update(InputManager& inputManager, float deltaTime, Camera& camera)
{
	if (opening)
	{
		openDelay += deltaTime ;
		subtractValue += deltaTime * 264;
		volumeModifier -= deltaTime * 2;
		if (openDelay >= 0.25f)
		{
			openDelay = 0.0f;
			opening = false;
			subtractValue = subtractValueMax;
			volumeModifier = 0.5f;
		}
		Mix_VolumeMusic(128 * volumeModifier);
	}
	else if (closing)
	{
		openDelay += deltaTime;
		subtractValue -= deltaTime * 264;
		volumeModifier += deltaTime * 2;
		if (openDelay >= 0.25f)
		{
			openDelay = 0.0f;
			show = false;
			closing = false;
			ResourceManager::GetShader("instance").Use().SetFloat("subtractValue", 0);
			ResourceManager::GetShader("Nsprite").Use().SetFloat("subtractValue", 0);
			ResourceManager::GetShader("sprite").Use().SetFloat("subtractValue", 0);
			volumeModifier = 1;
		}
		Mix_VolumeMusic(128 * volumeModifier);
	}
	else
	{
		CheckInput(inputManager, deltaTime, camera);

		flashEffect += 1.5f * deltaTime;
		if (flashEffect >= 0.7f)
		{
			flashEffect = 0.0f;
		}
		fadeAlpha += 0.5f * deltaTime;
		if (fadeAlpha >= 0.5f)
		{
			fadeAlpha = 0.5f;
		}
	}
}

void Minimap::CheckInput(InputManager& inputManager, float deltaTime, Camera& camera)
{
	if (inputManager.isKeyDown(SDLK_RETURN))
	{
		held = true;
	}
	else if (inputManager.isKeyReleased(SDLK_RETURN))
	{
		held = false;
	}
	int xDirection = 0;
	int yDirection = 0;
	if (inputManager.isKeyDown(SDLK_RIGHT))
	{
		xDirection = 1;
	}
	if (inputManager.isKeyDown(SDLK_LEFT))
	{
		xDirection = -1;
	}
	if (inputManager.isKeyDown(SDLK_UP))
	{
		yDirection = -1;
	}
	if (inputManager.isKeyDown(SDLK_DOWN))
	{
		yDirection = 1;
	}
	glm::vec2 position = camera.getPosition();
	position = glm::ivec2(position) + glm::ivec2(xDirection, yDirection) * TileManager::TILE_SIZE;
	camera.setPosition(position);
}

void Minimap::Draw(const std::vector<Unit*>& playerUnits, const std::vector<Unit*>& enemyUnits, Camera& camera, int shapeVAO, SpriteRenderer* Renderer)
{
	if (show)
	{
		float transparent = 1.0f;
		if (held)
		{
			transparent = 0.4f;
		}

		ResourceManager::GetShader("shapeInstance").Use().SetMatrix4("projection", camera.getOrthoMatrix());
		ResourceManager::GetShader("shapeInstance").SetFloat("alpha", transparent);
		int x = 256 * 0.5f - (TileManager::tileManager.rowTiles * 2);
		int y = 224 * 0.5f - (TileManager::tileManager.columnTiles * 2);
		int startY = y;
		int startX = x;
		ShapeBatch shapeBatch;
		shapeBatch.init();
		shapeBatch.begin();
		//Going to want to batch this, since this is quite a few draw calls
		int width = TileManager::tileManager.levelWidth / 4;
		for (int i = 0; i < TileManager::tileManager.totalTiles; i++)
		{
			shapeBatch.addToBatch(glm::vec2(x, y), 4, 4, glm::vec3(TileManager::tileManager.tiles[i].properties.miniMapColor));
			x += 4;
			if (x >= width + startX)
			{
				//Move back
				x = startX;

				//Move to the next row
				y += 4;
			}
		}

		for (int i = 0; i < enemyUnits.size(); i++)
		{
			auto unitPosition = enemyUnits[i]->sprite.getPosition();
			unitPosition /= 16;
			unitPosition *= 4;
			unitPosition += glm::vec2(startX, startY);
			shapeBatch.addToBatch(glm::vec2(unitPosition.x + 1, unitPosition.y), 2, 1, glm::vec3(1, flashEffect, flashEffect));
			shapeBatch.addToBatch(glm::vec2(unitPosition.x, unitPosition.y + 1), 4, 2, glm::vec3(1, flashEffect, flashEffect));
			shapeBatch.addToBatch(glm::vec2(unitPosition.x + 1, unitPosition.y + 3), 2, 1, glm::vec3(1, flashEffect, flashEffect));
		}

		for (int i = 0; i < playerUnits.size(); i++)
		{
			auto unitPosition = playerUnits[i]->sprite.getPosition();
			unitPosition /= 16;
			unitPosition *= 4;
			unitPosition += glm::vec2(startX, startY);
			shapeBatch.addToBatch(glm::vec2(unitPosition.x + 1, unitPosition.y), 2, 1, glm::vec3(flashEffect, flashEffect, 1));
			shapeBatch.addToBatch(glm::vec2(unitPosition.x, unitPosition.y + 1), 4, 2, glm::vec3(flashEffect, flashEffect, 1));
			shapeBatch.addToBatch(glm::vec2(unitPosition.x + 1, unitPosition.y + 3), 2, 1, glm::vec3(flashEffect, flashEffect, 1));
		}

		shapeBatch.end();
		shapeBatch.renderBatch();

		//ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera.getOrthoMatrix());
		ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera.getOrthoMatrix());
		ResourceManager::GetShader("Nsprite").SetFloat("subtractValue", 0);

		Renderer->setUVs(cursorUvs[0]);
		Texture2D displayTexture = ResourceManager::GetTexture("cursor");
		auto cameraPosition = camera.getPosition();
		cameraPosition -= glm::vec2(128, 112);
		cameraPosition /= 4;
		cameraPosition += glm::vec2(startX - 3, startY - 3) ;
		Renderer->DrawSprite(displayTexture, cameraPosition, 0.0f, glm::vec2(70, 62));

		//duplicate on the check, we're just doing it, don't worry about it
		if (held)
		{
			ResourceManager::GetShader("instance").SetFloat("subtractValue", 0, true);
			ResourceManager::GetShader("Nsprite").SetFloat("subtractValue", 0, true);
			ResourceManager::GetShader("sprite").SetFloat("subtractValue", 0, true);
		}
		else
		{
			ResourceManager::GetShader("instance").SetFloat("subtractValue", subtractValue, true);
			ResourceManager::GetShader("Nsprite").SetFloat("subtractValue", subtractValue, true);
			ResourceManager::GetShader("sprite").SetFloat("subtractValue", subtractValue, true);
		}
	}
}

void Minimap::Open()
{
	show = true;
	subtractValue = 0.0f;
	opening = true;
	ResourceManager::PlaySound("minimapOpen");
}

void Minimap::Close()
{
	closing = true;
	ResourceManager::PlaySound("minimapClose");
}
