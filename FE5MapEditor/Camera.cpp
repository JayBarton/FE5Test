/*
Code modified from https://github.com/Barnold1953/GraphicsTutorials/tree/master/Bengine
*/

#include "Camera.h"
#include <gtc/epsilon.hpp>
#include <gtx/norm.hpp>
#include <math.h>
#include <iostream>
Camera::Camera(int screenW, int screenH, int levelW, int levelH) : screenWidth(screenW),
    screenHeight(screenH), levelWidth(levelW), levelHeight(levelH), cameraScale(1.0f), position(0.0f, 0.0f),
    cameraMatrix(1.0f), needsMatrixUpdate(true)
{
    orthographicMatrix = glm::ortho(0.0f , float(screenWidth) ,
                                    float(screenHeight) , 0.0f , -1.0f, 1.0f);
    halfWidth = screenWidth/2;
    halfHeight = screenHeight/2;

	boxX = halfWidth - boxOffset;
	boxY = halfHeight;
	boxW = halfWidth + boxOffset;
	boxH = halfHeight + halfHeight;
	//border = glm::ivec4(halfWidth - 32, halfHeight - 100, halfWidth + 32, halfHeight);
	border = glm::ivec4(48, 48, screenWidth - 64, screenHeight - 64);
}

Camera::~Camera()
{
    //dtor
}

void Camera::update()
{
    if(needsMatrixUpdate)
    {
        if (position.x - (halfWidth / cameraScale) < 0)
        {
            position.x = (halfWidth / cameraScale) ;
        }
        if(position.x  + (halfWidth / cameraScale) > levelWidth)
        {
            position.x = (levelWidth - halfWidth / cameraScale) ;
        }

        if(position.y  - (halfHeight / cameraScale) < 0)
        {
            position.y = (halfHeight / cameraScale) ;
        }
        if(position.y  + (halfHeight / cameraScale) > levelHeight)
        {
            position.y = (levelHeight - halfHeight / cameraScale) ;
        }

       //Round the position to prevent oddities when rendering at decimal positions
        glm::vec3 translate(round(-position.x) + halfWidth, round(-position.y) + halfHeight, 0.0f);

        glm::vec3 scale(cameraScale, cameraScale, 0.0f);

        cameraMatrix = glm::translate(orthographicMatrix, translate);
        cameraMatrix = glm::scale(glm::mat4(1.0f), scale) * cameraMatrix;
        needsMatrixUpdate = false;
    }
}

bool Camera::outSideBox(glm::vec2 p)
{
	bool outSide = false;

	glm::vec2 pScreen = worldToScreen(p);
	if (pScreen.x > boxW || pScreen.x < boxX || pScreen.y >boxH || pScreen.y < boxY)
	{
outSide = true;
	}

	return outSide;
}

bool Camera::onScreen(glm::vec2 p, glm::vec2 size, glm::vec2 offset /*= glm::vec2(0)*/)
{
	bool on = true;

	glm::vec2 pScreen = worldToScreen(p);

	glm::vec4 screenBounds(0 - offset.x, 0 - offset.y, screenWidth + offset.x, screenHeight + offset.y);
	size *= cameraScale;
	if (pScreen.x + size.x < screenBounds.x || pScreen.x > screenBounds.z
		|| pScreen.y > screenBounds.w || pScreen.y + size.y < screenBounds.y)
	{
		on = false;
	}
	return on;
}


glm::vec2 Camera::screenToWorld(glm::vec2 screenCoords)
{
	int width = 800;
	int height = 600;
	screenCoords -= glm::vec2(800 * 0.5f, 600 * 0.5f);
	//Scale the coordinates
	screenCoords /= cameraScale;
	screenCoords.x /= cameraScale * (width / float(256));
	screenCoords.y /= cameraScale * (height / float(224));
	//Translate with the camera position
	screenCoords += position;

	return screenCoords;
}

glm::vec2 Camera::worldToScreen(glm::vec2 screenCoords)
{
	//Translate with the camera position
	screenCoords -= position;
	//Scale the coordinates
	screenCoords *= cameraScale;
	//Make it so the zero is the center
	screenCoords += glm::vec2(halfWidth, halfHeight);

	return screenCoords;
}

glm::vec2 Camera::worldToRealScreen(glm::vec2 screenCoords, int width, int height)
{
	//Translate with the camera position
	screenCoords -= position;
	//Scale the coordinates
	screenCoords *= cameraScale;
	screenCoords.x *= cameraScale * (width / static_cast<float>(256));
	screenCoords.y *= cameraScale * (height / static_cast<float>(224));
	//Make it so the zero is the center
	screenCoords += glm::vec2(width * 0.5f, height * 0.5f);

	return screenCoords;
}

void Camera::Follow(glm::vec2 p, float speed, float delta)
{
	glm::vec2 pScreen = worldToScreen(p);
	if (pScreen.x > boxW)
	{
		position.x += (speed * delta) / cameraScale;
		needsMatrixUpdate = true;
	}
	else if (pScreen.x < boxX)
	{
		position.x -= (speed * delta) / cameraScale;
		needsMatrixUpdate = true;
	}
	if (pScreen.y > boxH)
	{
		position.y += (speed * delta) / cameraScale;
		needsMatrixUpdate = true;
	}
	else if (pScreen.y < boxY)
	{
		position.y -= (speed * delta) / cameraScale;
		needsMatrixUpdate = true;
	}
}

void Camera::Follow(glm::vec2 p)
{
	glm::vec2 pScreen = worldToScreen(p);
	if (pScreen.x > border.z)
	{
		position.x += (pScreen.x - border.z) / cameraScale;
		needsMatrixUpdate = true;
	}
	else if (pScreen.x < border.x)
	{
		position.x -= (border.x - pScreen.x) / cameraScale;
		needsMatrixUpdate = true;
	}
	if (pScreen.y > border.w)
	{
		position.y += (pScreen.y - border.w) / cameraScale;
		needsMatrixUpdate = true;
	}
	else if (pScreen.y < border.y)
	{
		position.y -= (border.y - pScreen.y) / cameraScale;
		needsMatrixUpdate = true;
	}
}

void Camera::SetMove(glm::vec2 p)
{
	glm::vec2 pScreen = worldToScreen(p);
	startPosition = position;
	movePosition = position;
	if (pScreen.x > border.z)
	{
		movePosition.x += (pScreen.x - border.z) / cameraScale;
	}
	else if (pScreen.x < border.x)
	{
		movePosition.x -= (border.x - pScreen.x) / cameraScale;
	}
	if (pScreen.y > border.w)
	{
		movePosition.y += (pScreen.y - border.w) / cameraScale;
	}
	else if (pScreen.y < border.y)
	{
		movePosition.y -= (border.y - pScreen.y) / cameraScale;
	}

	if (movePosition.x - (halfWidth / cameraScale) < 0)
	{
		movePosition.x = (halfWidth / cameraScale);
	}
	if (movePosition.x + (halfWidth / cameraScale) > levelWidth)
	{
		movePosition.x = (levelWidth - halfWidth / cameraScale);
	}

	if (movePosition.y - (halfHeight / cameraScale) < 0)
	{
		movePosition.y = (halfHeight / cameraScale);
	}
	if (movePosition.y + (halfHeight / cameraScale) > levelHeight)
	{
		movePosition.y = (levelHeight - halfHeight / cameraScale);
	}
	if (startPosition == movePosition)
	{
		moving = false;
	}
}

void Camera::MoveTo(float delta, float speed)
{
	auto direction = glm::normalize(movePosition - startPosition);
	auto L1 = glm::length(position);
	auto L2 = glm::length(movePosition);
	auto moveScale = L1 / L2;
	if (moveScale < 1)
	{
		moveScale = 1;
	}
	position += direction * speed * moveScale;

	if (glm::length(position - movePosition) <= speed)
	{
		moving = false;
		position = movePosition;
	}
	needsMatrixUpdate = true;
}
