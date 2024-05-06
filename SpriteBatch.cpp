#include "SpriteBatch.h"
#include "Shader.h"
#include "ResourceManager.h"
#include <algorithm>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <iostream>


SpriteBatch::SpriteBatch() : vao(0)
{
    //ctor
}

SpriteBatch::~SpriteBatch()
{
    //dtor
}
void SpriteBatch::init()
{
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glBindVertexArray(0);
    ResourceManager::GetShader("instance").Use().SetInteger("model_matrix_tbo", 1);
    ResourceManager::GetShader("instance").SetInteger("uv_tbo", 2);

}

void SpriteBatch::begin()
{
    theSprites.clear();
    renderBatches.clear();
}

void SpriteBatch::end()
{
    sortSprites();
    createRenderBatches();
}

void SpriteBatch::addToBatch(GLuint id, glm::vec2 position, int width, int height, const glm::vec4 &uv)
{
    theSprites.emplace_back(id, position, width, height, uv);
}

void SpriteBatch::sortSprites()
{
    std::sort(theSprites.begin(), theSprites.end(), compareTexture);
}

bool SpriteBatch::compareTexture(const BatchSprite&a, const BatchSprite&b)
{
    return (a.textureID < b.textureID);
}

void SpriteBatch::createRenderBatches()
{

    if(theSprites.empty())
    {
        return;
    }

    GLuint model_matrix_buffer;
    GLuint model_matrix_tbo;
    GLuint uvTBO;
    GLuint uvBuffer;

    std::vector<glm::vec4> uvCoords;
    uvCoords.resize(theSprites.size());
    models.resize(theSprites.size());

    int theOffset = 0;
    int currentSprite = 0;

    renderBatches.emplace_back(theOffset, 1, theSprites[0].textureID);

    glm::mat4 model;
    model = glm::translate(model, glm::vec3(theSprites[currentSprite].spritePosition, 0.0f));  // First translate (transformations are: scale happens first, then rotation and then finall translation happens; reversed order)

    glm::vec2 size (theSprites[currentSprite].spriteWidth, theSprites[currentSprite].spriteHeight);

    model = glm::translate(model, glm::vec3(0.5f * size.x, 0.5f * size.y, 0.0f)); // Move origin of rotation to center of quad
    model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f)); // Then rotate // fix later
    model = glm::translate(model, glm::vec3(-0.5f * size.x, -0.5f * size.y, 0.0f)); // Move origin back

    model = glm::scale(model, glm::vec3(size, 1.0f)); // Last scale

    models[currentSprite] = model;
    uvCoords[currentSprite] = theSprites[currentSprite].uv;

    theOffset ++;

    for(currentSprite = 1; currentSprite < theSprites.size(); currentSprite ++)
    {
        if(theSprites[currentSprite].textureID !=theSprites[currentSprite - 1].textureID)
        {
            renderBatches.emplace_back(theOffset, 1, theSprites[currentSprite].textureID);
        }
        else
        {
            renderBatches.back().numberOfVerticies ++;
        }

        model = glm::mat4();
        model = glm::translate(model, glm::vec3(theSprites[currentSprite].spritePosition, 0.0f));  // First translate (transformations are: scale happens first, then rotation and then finall translation happens; reversed order)

        glm::vec2 size (theSprites[currentSprite].spriteWidth, theSprites[currentSprite].spriteHeight);

        model = glm::translate(model, glm::vec3(0.5f * size.x, 0.5f * size.y, 0.0f)); // Move origin of rotation to center of quad
        model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f)); // Then rotate // fix later
        model = glm::translate(model, glm::vec3(-0.5f * size.x, -0.5f * size.y, 0.0f)); // Move origin back

        model = glm::scale(model, glm::vec3(size, 1.0f)); // Last scale

        models[currentSprite] = model;

        uvCoords[currentSprite] = theSprites[currentSprite].uv;

        theOffset++;
    }

    glBindVertexArray(vao);

    glGenTextures(1, &model_matrix_tbo);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, model_matrix_tbo);
    glGenBuffers(1, &model_matrix_buffer);
    glBindBuffer(GL_TEXTURE_BUFFER, model_matrix_buffer);
    glBufferData(GL_TEXTURE_BUFFER, theSprites.size()* sizeof(glm::mat4), nullptr, GL_STATIC_DRAW);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, model_matrix_buffer);
    glBufferSubData(GL_TEXTURE_BUFFER, 0, models.size() * sizeof(glm::mat4), models.data());

    glGenTextures(1, &uvTBO);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_BUFFER, uvTBO);
    glGenBuffers(1, &uvBuffer);
    glBindBuffer(GL_TEXTURE_BUFFER, uvBuffer);
    glBufferData(GL_TEXTURE_BUFFER, theSprites.size() * sizeof(glm::vec4), uvCoords.data(), GL_STATIC_DRAW);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, uvBuffer);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(0);
}

void SpriteBatch::renderBatch()
{
	glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);
    for(int i = 0; i < renderBatches.size(); i ++)
    {
        glBindTexture(GL_TEXTURE_2D, renderBatches[i].textureID);
		glDrawArraysInstanced(GL_TRIANGLES, 0, 6, renderBatches[i].numberOfVerticies);
    }
	glBindVertexArray(0);

}


