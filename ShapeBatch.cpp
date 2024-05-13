#include "ShapeBatch.h"
#include "Shader.h"
#include "ResourceManager.h"
#include <algorithm>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <iostream>


ShapeBatch::ShapeBatch() : vao(0)
{
    //ctor
}

ShapeBatch::~ShapeBatch()
{
    //dtor
}
void ShapeBatch::init()
{
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glBindVertexArray(0);
    ResourceManager::GetShader("shapeInstance").Use().SetInteger("model_matrix_tbo", 3);
    ResourceManager::GetShader("shapeInstance").SetInteger("color_tbo", 4);

}

void ShapeBatch::begin()
{
    theShapes.clear();
    renderBatches.clear();
}

void ShapeBatch::end()
{
    createRenderBatches();
}

void ShapeBatch::addToBatch(glm::vec2 position, int width, int height, const glm::vec3& color)
{
    theShapes.emplace_back(position, width, height, color);
}

/*void ShapeBatch::sortSprites()
{
    std::sort(theShapes.begin(), theShapes.end(), compareTexture);
}

bool ShapeBatch::compareTexture(const BatchSprite& a, const BatchSprite& b)
{
    return (a.textureID < b.textureID);
}*/

void ShapeBatch::createRenderBatches()
{

    if (theShapes.empty())
    {
        return;
    }

    GLuint shape_model_matrix_buffer;
    GLuint shape_model_matrix_tbo;
    GLuint uvTBO;
    GLuint uvBuffer;

    std::vector<glm::vec3> colors;
    colors.resize(theShapes.size());
    models.resize(theShapes.size());

    int theOffset = 0;
    int currentSprite = 0;

    renderBatches.emplace_back(theOffset, 1);

    glm::mat4 model;
    model = glm::translate(model, glm::vec3(theShapes[currentSprite].shapePosition, 0.0f));  // First translate (transformations are: scale happens first, then rotation and then finall translation happens; reversed order)

    glm::vec2 size(theShapes[currentSprite].spriteWidth, theShapes[currentSprite].spriteHeight);

    model = glm::translate(model, glm::vec3(0.5f * size.x, 0.5f * size.y, 0.0f)); // Move origin of rotation to center of quad
    model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f)); // Then rotate // fix later
    model = glm::translate(model, glm::vec3(-0.5f * size.x, -0.5f * size.y, 0.0f)); // Move origin back

    model = glm::scale(model, glm::vec3(size, 1.0f)); // Last scale

    models[currentSprite] = model;
    colors[currentSprite] = theShapes[currentSprite].color;
    theOffset++;

    for (currentSprite = 1; currentSprite < theShapes.size(); currentSprite++)
    {

        renderBatches.back().numberOfVerticies++;

        model = glm::mat4();
        model = glm::translate(model, glm::vec3(theShapes[currentSprite].shapePosition, 0.0f));  // First translate (transformations are: scale happens first, then rotation and then finall translation happens; reversed order)

        glm::vec2 size(theShapes[currentSprite].spriteWidth, theShapes[currentSprite].spriteHeight);

        model = glm::translate(model, glm::vec3(0.5f * size.x, 0.5f * size.y, 0.0f)); // Move origin of rotation to center of quad
        model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f)); // Then rotate // fix later
        model = glm::translate(model, glm::vec3(-0.5f * size.x, -0.5f * size.y, 0.0f)); // Move origin back

        model = glm::scale(model, glm::vec3(size, 1.0f)); // Last scale

        models[currentSprite] = model;
        colors[currentSprite] = theShapes[currentSprite].color;

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

     glGenTextures(1, &uvTBO);
     glActiveTexture(GL_TEXTURE4);
     glBindTexture(GL_TEXTURE_BUFFER, uvTBO);
     glGenBuffers(1, &uvBuffer);
     glBindBuffer(GL_TEXTURE_BUFFER, uvBuffer);
     glBufferData(GL_TEXTURE_BUFFER, theShapes.size() * sizeof(glm::vec3), colors.data(), GL_STATIC_DRAW);
     glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, uvBuffer); //Not passing alpha right now, so use GL_RGB32F instead of GL_RGBA32F 

     glActiveTexture(GL_TEXTURE0);
     glBindVertexArray(0);

}

void ShapeBatch::renderBatch()
{
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);
    for (int i = 0; i < renderBatches.size(); i++)
    {
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, renderBatches[i].numberOfVerticies);
    }
    glBindVertexArray(0);
}