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
* 
** Additional optimization changes from https://github.com/johnWRS/LearnOpenGLTextRenderingImprovement/tree/main
******************************************************************/

#ifndef TEXTRENDERER_H
#define TEXTRENDERER_H

#include <map>

#include <GL/glew.h>
#include <glm.hpp>

#include "Texture.h"
#include "..\Shader.h"


/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
    int TextureID;   // ID handle of the glyph texture
    glm::ivec2 Size;    // Size of glyph
    glm::ivec2 Bearing; // Offset from baseline to left/top of glyph
    GLuint Advance;     // Horizontal offset to advance to next glyph
};

const unsigned int ARRAY_LIMIT = 200;
// A renderer class for rendering text displayed by a font loaded using the
// FreeType library. A single font is loaded, processed into a list of Character
// items for later rendering.
class TextRenderer
{
public:
    // Holds a list of pre-compiled Characters
    std::map<GLchar, Character> Characters;
    // Shader used for text rendering
    Shader TextShader;
    // Constructor
    TextRenderer(GLuint width, GLuint height);
    // Pre-compiles a list of characters from the given font
    void Load(std::string font, GLuint fontSize);
    // Renders a string of text using the precompiled list of characters
    void RenderText(std::string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color = glm::vec3(1.0f));

    void TextRenderCall(int length);

    //containerWidth is the width I want to justify to. If I have a bunch of text, and the widest is say 40, 
    //I should set containerWidth to 40 for anything I want to line up with that text
    void RenderTextRight(std::string text, GLfloat x, GLfloat y, GLfloat scale, int containerWidth, glm::vec3 color = glm::vec3(1.0f));
    int GetTextWidth(std::string text, GLfloat scale);
private:
    // Render state
    GLuint VAO, VBO;
    GLuint textureArray = 0;
    GLuint drawSize = 30;
    std::vector<glm::mat4> transforms;
    std::vector<int> letterMap;
};


#endif // TEXTRENDERER_H
