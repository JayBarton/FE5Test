#ifndef SPRITEBATCH_H
#define SPRITEBATCH_H

#include <GL/glew.h>
#include <glm.hpp>
#include <vector>
#include "Shader.h"

class BatchSprite
{
public:
    BatchSprite(GLuint id, glm::vec2 position, int width, int height, const glm::vec4 &theUV) : textureID(id), spritePosition(position), spriteWidth(width), spriteHeight(height), uv(theUV)
    {
    }
    GLuint textureID;
    glm::vec2 spritePosition;
    int spriteWidth;
    int spriteHeight;
    glm::vec4 uv;
};

class RenderBatch
{
public:

    RenderBatch(GLuint theOffset, GLuint verticies, GLuint theTexture): offSet(theOffset), numberOfVerticies(verticies), textureID(theTexture)
    {
    }
    GLuint offSet;
    GLuint numberOfVerticies;
    GLuint textureID;
};

class SpriteBatch
{
public:
    SpriteBatch();
    ~SpriteBatch();
    void init();
    void begin();
    void end();
    void addToBatch(GLuint id, glm::vec2 position, int width, int height, const glm::vec4 &uv = glm::vec4(0));
    void sortSprites();
    void createRenderBatches();
    void renderBatch();

protected:
private:
    static bool compareTexture(const BatchSprite&a, const BatchSprite&b);

    GLuint vao;

    std::vector<BatchSprite> theSprites;
    std::vector<RenderBatch> renderBatches;
    std::vector<glm::mat4> models;
};

#endif // SPRITEBATCH_H
