/*******************************************************************
** Modified from code written by Joey de Vries
** https://learnopengl.com/
** https://twitter.com/JoeyDeVriez
** This code is part of Breakout.
**
** Breakout is free software: you can redistribute it and/or modify
** it under the terms of the CC BY 4.0 license as published by
** Creative Commons, either version 4 of the License, or (at your
** option) any later version.
******************************************************************/

#ifndef SPRITERENDERER_H
#define SPRITERENDERER_H

#include <GL/glew.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtx/rotate_vector.hpp>

#include "Texture.h"
#include "..\Shader.h"

class SpriteRenderer
{
public:
    // Constructor (inits shaders/shapes)
    SpriteRenderer(Shader &shader);
    // Destructor
    ~SpriteRenderer();
    // Renders a defined quad textured with given sprite
    void DrawSprite(Texture2D &texture, glm::vec2 position, float rotate, glm::vec2 size, const glm::vec4 &color = glm::vec4(1.0f));

	void DrawSprite();
    void setUVs(const glm::vec4 &uv = glm::vec4(0, 1, 0, 1));
private:
    GLuint VBO;
    // Render state
    Shader shader;
    GLuint quadVAO;
    // Initializes and configures the quad's buffer and vertex attributes
    void initRenderData();
};

#endif // SPRITERENDERER_H
