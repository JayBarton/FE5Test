#include "SBatch.h"
#include "Shader.h"
#include "ResourceManager.h"
#include <algorithm>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <iostream>

SBatch::SBatch() : vao(0)
{
}

SBatch::~SBatch()
{
}

void SBatch::init()
{
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glBindVertexArray(0);
}

void SBatch::begin()
{
    theSprites.clear();
    renderBatches.clear();
}

void SBatch::end()
{
    sortSprites();
    createRenderBatches();
}

void SBatch::addToBatch(GLuint id, glm::vec2 position, glm::vec2 size, const glm::vec4& color, float hitFactor, bool grey, int team, const glm::vec4& uv)
{
    theSprites.emplace_back(id, position, size.x, size.y, uv, color, hitFactor, grey, team);
}

void SBatch::sortSprites()
{
    std::sort(theSprites.begin(), theSprites.end(), compareTexture);
}

void SBatch::createRenderBatches()
{
    if (theSprites.empty())
    {
        return;
    }

    GLuint model_matrix_buffer;
    GLuint model_matrix_tbo;

    std::vector<glm::vec4> uvCoords;
    std::vector<glm::vec4> colors;
    std::vector<int> paletteRows;
    std::vector<float> greys; //can probably an int or even a bool
    std::vector<float> hitFactors;
    uvCoords.resize(theSprites.size());
    models.resize(theSprites.size());
    colors.resize(theSprites.size());
    paletteRows.resize(theSprites.size());
    greys.resize(theSprites.size());
    hitFactors.resize(theSprites.size());

    int theOffset = 0;
    int currentSprite = 0;

    renderBatches.emplace_back(theOffset, 1, theSprites[0].textureID);

    glm::mat4 model;
    model = glm::translate(model, glm::vec3(theSprites[currentSprite].spritePosition, 0.0f));  // First translate (transformations are: scale happens first, then rotation and then finall translation happens; reversed order)

    glm::vec2 size(theSprites[currentSprite].spriteWidth, theSprites[currentSprite].spriteHeight);

    model = glm::translate(model, glm::vec3(0.5f * size.x, 0.5f * size.y, 0.0f)); // Move origin of rotation to center of quad
    model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f)); // Then rotate // fix later
    model = glm::translate(model, glm::vec3(-0.5f * size.x, -0.5f * size.y, 0.0f)); // Move origin back

    model = glm::scale(model, glm::vec3(size, 1.0f)); // Last scale

    models[currentSprite] = model;
    uvCoords[currentSprite] = theSprites[currentSprite].uv;
    colors[currentSprite] = theSprites[currentSprite].color;
    paletteRows[currentSprite] = theSprites[currentSprite].team;
    greys[currentSprite] = theSprites[currentSprite].grey;
    hitFactors[currentSprite] = theSprites[currentSprite].hitFactor;
    theOffset++;

    for (currentSprite = 1; currentSprite < theSprites.size(); currentSprite++)
    {
        if (theSprites[currentSprite].textureID != theSprites[currentSprite - 1].textureID)
        {
            renderBatches.emplace_back(theOffset, 1, theSprites[currentSprite].textureID);
        }
        else
        {
            renderBatches.back().numberOfVerticies++;
        }

        model = glm::mat4();
        model = glm::translate(model, glm::vec3(theSprites[currentSprite].spritePosition, 0.0f));  // First translate (transformations are: scale happens first, then rotation and then finall translation happens; reversed order)

        glm::vec2 size(theSprites[currentSprite].spriteWidth, theSprites[currentSprite].spriteHeight);

        model = glm::translate(model, glm::vec3(0.5f * size.x, 0.5f * size.y, 0.0f)); // Move origin of rotation to center of quad
        model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f)); // Then rotate // fix later
        model = glm::translate(model, glm::vec3(-0.5f * size.x, -0.5f * size.y, 0.0f)); // Move origin back

        model = glm::scale(model, glm::vec3(size, 1.0f)); // Last scale

        models[currentSprite] = model;

        uvCoords[currentSprite] = theSprites[currentSprite].uv;
        colors[currentSprite] = theSprites[currentSprite].color;
        paletteRows[currentSprite] = theSprites[currentSprite].team;
        greys[currentSprite] = theSprites[currentSprite].grey;
        hitFactors[currentSprite] = theSprites[currentSprite].hitFactor;
        theOffset++;
    }

    ResourceManager::GetShader("sprite").Use().SetMatrix4("model", models[0], GL_TRUE, theSprites.size());
    ResourceManager::GetShader("sprite").Use().SetVector4fv("uvs", theSprites.size(), glm::value_ptr(uvCoords[0]));
    ResourceManager::GetShader("sprite").Use().SetVector4fv("spriteColor", theSprites.size(), glm::value_ptr(colors[0]));

    ResourceManager::GetShader("sprite").Use().SetFloatv("u_colorFactor", &greys[0], GL_TRUE, theSprites.size());
    ResourceManager::GetShader("sprite").Use().SetFloatv("hitFactor", &hitFactors[0], GL_TRUE, theSprites.size());
    ResourceManager::GetShader("sprite").Use().SetInteger2("paletteRow", &paletteRows[0], GL_TRUE, theSprites.size());
}

void SBatch::renderBatch()
{
    glBindVertexArray(vao);
    Texture2D texture2 = ResourceManager::GetTexture("palette");
    glActiveTexture(GL_TEXTURE1);
    texture2.Bind();

    //Would be nice to have a check to see if this is actually necessary, otherwise I'm swapping this texture for nothing
    texture2 = ResourceManager::GetTexture("BattleFadeIn");
    glActiveTexture(GL_TEXTURE2);
    texture2.Bind();
    for (int i = 0; i < renderBatches.size(); i++)
    {
        glActiveTexture(GL_TEXTURE0);
        ResourceManager::GetShader("sprite").Use().SetInteger("instanceOffset", renderBatches[i].offSet);
        glBindTexture(GL_TEXTURE_2D, renderBatches[i].textureID);

        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, renderBatches[i].numberOfVerticies);
    }
    glBindVertexArray(0);
}

bool SBatch::compareTexture(const bSprite& a, const bSprite& b)
{
    if (a.textureID != b.textureID) {
        return a.textureID < b.textureID;
    }
    return a.spritePosition.y < b.spritePosition.y;
}
