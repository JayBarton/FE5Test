#pragma once
#include <glm.hpp>
#include <vector>
#include "Shader.h"
#include "SpriteBatch.h"

struct bSprite
{
public:
    bSprite(GLuint id, glm::vec2 position, int width, int height, const glm::vec4& theUV, const glm::vec4& color, float hitFactor, bool grey, int team) :
        textureID(id), spritePosition(position), spriteWidth(width), spriteHeight(height), uv(theUV), color(color),  hitFactor(hitFactor), grey(grey), team(team)
    {
    }
    GLuint textureID;
    glm::vec2 spritePosition;
    int spriteWidth;
    int spriteHeight;
    glm::vec4 uv;
    glm::vec4 color;
    float hitFactor;
    bool grey;
    int team;
};

struct SBatch
{
    SBatch();
    ~SBatch();
    void init();
    void begin();
    void end();
    void addToBatch(GLuint id, glm::vec2 position, glm::vec2 size, const glm::vec4& color = glm::vec4(1.0f), float hitFactor = 0, bool grey = false, int team = 0, const glm::vec4& uv = glm::vec4(0));
    void sortSprites();
    void createRenderBatches();
    void renderBatch();

    static bool compareTexture(const bSprite& a, const bSprite& b);

    GLuint vao;

    std::vector<bSprite> theSprites;
    std::vector<RenderBatch> renderBatches;
    std::vector<glm::mat4> models;
};