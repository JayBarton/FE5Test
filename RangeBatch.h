#pragma once
#include <glm.hpp>
#include <vector>
#include "Shader.h"

class BatchRangeShape
{
public:
    BatchRangeShape(glm::vec2 position, glm::vec3 mainColor, glm::vec3 outerColor) : shapePosition(position), mainColor(mainColor), outerColor(outerColor)
    {
    }
    glm::vec2 shapePosition;
    glm::vec3 mainColor;
    glm::vec3 outerColor;
};

class RangeBatch
{
public:
    RangeBatch();
    ~RangeBatch();
    void init();
    void begin();
    void end();
    void addToBatch(glm::vec2 position, const glm::vec3& mainColor = glm::vec3(0), const glm::vec3& innerColor = glm::vec3(0));
    void createRenderBatches();
    void renderBatch();

    int numberOfShapes;

protected:
private:
    GLuint vao;

    std::vector<BatchRangeShape> theShapes;
    std::vector<glm::mat4> models;
};
