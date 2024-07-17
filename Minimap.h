#pragma once
#include <glm.hpp>
#include <vector>

#include "ShapeBatch.h"
#include "Unit.h"

class InputManager;
class Camera;

struct Minimap
{
	void Update(InputManager& inputManager, float deltaTime, Camera& camera);
	void CheckInput(InputManager& inputManager, float deltaTime, Camera& camera);
	void Draw(const std::vector<Unit*>& playerUnits, const std::vector<Unit*>& enemyUnits, Camera& camera, int shapeVAO, class SpriteRenderer* Renderer);

	bool show = false;
	bool held = false;

	float flashEffect;

	float fadeAlpha = 0.0f;

	std::vector<glm::vec4> cursorUvs;

	ShapeBatch shapeBatch;
};