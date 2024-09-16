#include "RangeBatch.h"
#include "ResourceManager.h"
#include <gtc/matrix_transform.hpp>

RangeBatch::RangeBatch() : vao(0)
{
}

RangeBatch::~RangeBatch()
{
}

void RangeBatch::init()
{
    if (vao == 0)
    {
        glGenVertexArrays(1, &vao);
    }
    glBindVertexArray(vao);

    glBindVertexArray(0);
    ResourceManager::GetShader("range").Use().SetInteger("model_matrix_tbo", 3);
    ResourceManager::GetShader("range").SetInteger("main_color_tbo", 4);
    ResourceManager::GetShader("range").SetInteger("outer_color_tbo", 5);
}

void RangeBatch::begin()
{
	theShapes.clear();
	numberOfShapes = 0;
}

void RangeBatch::end()
{
    createRenderBatches();
}

void RangeBatch::addToBatch(glm::vec2 position, const glm::vec3& mainColor, const glm::vec3& outerColor)
{
    theShapes.emplace_back(position, mainColor, outerColor);
}

void RangeBatch::createRenderBatches()
{
    if (theShapes.empty())
    {
        return;
    }

    GLuint shape_model_matrix_buffer;
    GLuint shape_model_matrix_tbo;
    GLuint mainColorTBO;
    GLuint mainColorBuffer;
    GLuint outerColorTBO;
    GLuint outerColorBuffer;

    std::vector<glm::vec3> mainColors;
    std::vector<glm::vec3> outerColors;
    mainColors.resize(theShapes.size());
    outerColors.resize(theShapes.size());
    models.resize(theShapes.size());

    int theOffset = 0;
    int currentSprite = 0;

    glm::mat4 model;
    model = glm::translate(model, glm::vec3(theShapes[currentSprite].shapePosition, 0.0f));  // First translate (transformations are: scale happens first, then rotation and then finall translation happens; reversed order)

    glm::vec2 size(16, 16);

    model = glm::translate(model, glm::vec3(0.5f * size.x, 0.5f * size.y, 0.0f)); // Move origin of rotation to center of quad
    model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f)); // Then rotate // fix later
    model = glm::translate(model, glm::vec3(-0.5f * size.x, -0.5f * size.y, 0.0f)); // Move origin back

    model = glm::scale(model, glm::vec3(size, 1.0f)); // Last scale

    models[currentSprite] = model;
    mainColors[currentSprite] = theShapes[currentSprite].mainColor;
    outerColors[currentSprite] = theShapes[currentSprite].outerColor;
    theOffset++;
    numberOfShapes++;
    for (currentSprite = 1; currentSprite < theShapes.size(); currentSprite++)
    {
        numberOfShapes++;

        model = glm::mat4();
        model = glm::translate(model, glm::vec3(theShapes[currentSprite].shapePosition, 0.0f));  // First translate (transformations are: scale happens first, then rotation and then finall translation happens; reversed order)

        model = glm::translate(model, glm::vec3(0.5f * size.x, 0.5f * size.y, 0.0f)); // Move origin of rotation to center of quad
        model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f)); // Then rotate // fix later
        model = glm::translate(model, glm::vec3(-0.5f * size.x, -0.5f * size.y, 0.0f)); // Move origin back

        model = glm::scale(model, glm::vec3(size, 1.0f)); // Last scale

        models[currentSprite] = model;
        mainColors[currentSprite] = theShapes[currentSprite].mainColor;
        outerColors[currentSprite] = theShapes[currentSprite].outerColor;

        theOffset++;
    }

    glBindVertexArray(vao);

    glGenTextures(1, &shape_model_matrix_tbo);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_BUFFER, shape_model_matrix_tbo);
    glGenBuffers(1, &shape_model_matrix_buffer);
    glBindBuffer(GL_TEXTURE_BUFFER, shape_model_matrix_buffer);
    glBufferData(GL_TEXTURE_BUFFER, theShapes.size() * sizeof(glm::mat4), nullptr, GL_STATIC_DRAW);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, shape_model_matrix_buffer);
    glBufferSubData(GL_TEXTURE_BUFFER, 0, models.size() * sizeof(glm::mat4), models.data());

    glBindVertexArray(0);

    glGenTextures(1, &mainColorTBO);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_BUFFER, mainColorTBO);
    glGenBuffers(1, &mainColorBuffer);
    glBindBuffer(GL_TEXTURE_BUFFER, mainColorBuffer);
    glBufferData(GL_TEXTURE_BUFFER, theShapes.size() * sizeof(glm::vec3), mainColors.data(), GL_STATIC_DRAW);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, mainColorBuffer); //Not passing alpha right now, so use GL_RGB32F instead of GL_RGBA32F 

    glActiveTexture(GL_TEXTURE0);

    glGenTextures(1, &outerColorTBO);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_BUFFER, outerColorTBO);
    glGenBuffers(1, &outerColorBuffer);
    glBindBuffer(GL_TEXTURE_BUFFER, outerColorBuffer);
    glBufferData(GL_TEXTURE_BUFFER, theShapes.size() * sizeof(glm::vec3), outerColors.data(), GL_STATIC_DRAW);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, outerColorBuffer);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(0);
}

void RangeBatch::renderBatch()
{
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, numberOfShapes);

    glBindVertexArray(0);
}
