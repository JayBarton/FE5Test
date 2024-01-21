/*
Code modified from https://github.com/Barnold1953/GraphicsTutorials/tree/master/Bengine
*/

#ifndef CAMERA_H
#define CAMERA_H

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

class Camera
{
public:
	Camera() {}

    Camera(int screenW, int screenH, int levelW, int levelH);

    ~Camera();

    void update();

	bool outSideBox(glm::vec2 p);
	bool onScreen(glm::vec2 p, glm::vec2 size, glm::vec2 offset = glm::vec2(0));

	glm::vec2 screenToWorld(glm::vec2 screenCoords);
	glm::vec2 worldToScreen(glm::vec2 screenCoords);
  //  bool isBoxInView(const glm::vec2& boxPosition, const glm::vec2 dimensions);

    void setPosition(const glm::vec2& newPosition)
    {
        position = newPosition;
        needsMatrixUpdate = true;
    }
    glm::vec2 getPosition()
    {
        return position;
    }

	//Reset the level height
	void setLevelHeight(int h)
	{
		levelHeight = h;
	}

    void setScale(float newScale)
    {
        cameraScale = newScale;
        needsMatrixUpdate = true;
    }
    float getScale()
    {
        return cameraScale;
    }

    glm::mat4 getCameraMatrix()
    {
        return cameraMatrix;
    }
	glm::mat4 getOrthoMatrix()
	{
		return orthographicMatrix;
	}

	void Follow(glm::vec2 p, float speed, float delta);
	void Follow(glm::vec2 p);

protected:
private:
    int screenWidth;
    int screenHeight;

    int halfWidth;
    int halfHeight;

    int levelWidth;
    int levelHeight;

	int boxX;
	int boxY;
	int boxW;
	int boxH;

	int boxOffset = 60;

	glm::ivec4 border;

    float cameraScale;

    glm::vec2 position;
    glm::mat4 cameraMatrix;
    glm::mat4 orthographicMatrix;

    bool needsMatrixUpdate;
};

#endif // CAMERA_H
