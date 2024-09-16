#pragma once
#include <glm.hpp>
#include <vector>
#include "Shader.h"

class BatchShape
{
public:
    BatchShape(glm::vec2 position, int width, int height, glm::vec3 color) : shapePosition(position), spriteWidth(width), spriteHeight(height), color(color)
    {
    }
    glm::vec2 shapePosition;
    int spriteWidth;
    int spriteHeight;
    glm::vec3 color;
};

class ShapeBatch
{
public:
    ShapeBatch();
    ~ShapeBatch();
    void init();
    void begin();
    void end();
    void addToBatch(glm::vec2 position, int width, int height, const glm::vec3& color = glm::vec3(0));
    void createRenderBatches();
    void renderBatch();

    int numberOfShapes;

protected:
private:
    GLuint vao;

    std::vector<BatchShape> theShapes;
    std::vector<glm::mat4> models;
};
