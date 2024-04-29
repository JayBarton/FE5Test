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

#include "TextRenderer.h"

#include <iostream>

#include <gtc/matrix_transform.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "..\ResourceManager.h"


TextRenderer::TextRenderer(GLuint width, GLuint height)
{
    // Load and configure shader
    this->TextShader = ResourceManager::LoadShader("Shaders/textVertexShader.txt", "Shaders/textFragmentShader.txt", nullptr, "text");
    this->TextShader.SetMatrix4("projection", glm::ortho(0.0f, static_cast<GLfloat>(width), static_cast<GLfloat>(height), 0.0f), GL_TRUE);

    GLfloat vertexData[] = {
        0.0f,1.0f,
        1.0f,1.0f,
        0.0f,0.0f,
        1.0f,0.0f,
    };

    letterMap.resize(ARRAY_LIMIT);
    transforms.resize(ARRAY_LIMIT);

    // Configure VAO/VBO for texture quads
    glGenVertexArrays(1, &this->VAO);
    glGenBuffers(1, &this->VBO);
    glBindVertexArray(this->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void TextRenderer::Load(std::string font, GLuint fontSize)
{
    // First clear the previously loaded Characters
    this->Characters.clear();
    // Then initialize and load the FreeType library
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) // All functions return a value different than 0 whenever an error occurred
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
    // Load font as face
    FT_Face face;
    if (FT_New_Face(ft, font.c_str(), 0, &face))
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
    // Set size to load glyphs as
    // Code I got this from tried to size everything as 256 and then scale it to the proper size, but that looked bad, so I'm just using the normal fontsize
    // Keeping this note just in case
  //  FT_Set_Pixel_Sizes(face, 256, 256);
    drawSize = fontSize;
    FT_Set_Pixel_Sizes(face, 0, fontSize);
    // Disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glGenTextures(1, &textureArray);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R8, fontSize, fontSize, 128, 0, GL_RED, GL_UNSIGNED_BYTE, 0);

    // Then for the first 128 ASCII characters, pre-load/compile their characters and store them
    for (GLubyte c = 0; c < 128; c++) // lol see what I did there
    {
        // Load character glyph
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }
        glTexSubImage3D(
            GL_TEXTURE_2D_ARRAY,
            0, 0, 0, int(c),
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows, 1,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );
        // Set texture options
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Now store character for later use
        Character character = {
            int(c),
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            face->glyph->advance.x
        };
        Characters.insert(std::pair<GLchar, Character>(c, character));
    }
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    // Destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

void TextRenderer::RenderText(std::string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color)
{
    // Activate corresponding render state
    //Adjust scale to font size(need to replace with variable
  //  scale = scale * 30.0f / 256.0f;
    float copyX = x;
    this->TextShader.Use();
    this->TextShader.SetVector3f("textColor", color);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);

    glBindVertexArray(this->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);

    int workingIndex = 0;
    // Iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
        Character ch = Characters[*c];
        //New line
        if (*c == '\n')
        {
            y += ((ch.Size.y)) * 1.3 * scale;
            x = copyX;
        }
        //Don't need to do anything if the character is a space
        else if(*c == ' ')
        {
            x += (ch.Advance >> 6) * scale;
        }
        else
        {
            GLfloat xpos = x + ch.Bearing.x * scale;
            GLfloat ypos = y + (this->Characters['H'].Bearing.y - ch.Bearing.y) * scale;
            transforms[workingIndex] = glm::translate(glm::mat4(1.0f), glm::vec3(xpos, ypos, 0)) * glm::scale(glm::mat4(1.0f), glm::vec3(drawSize * scale, drawSize * scale, 0));
            letterMap[workingIndex] = ch.TextureID;

            // Now advance cursors for next glyph
            x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (1/64th times 2^6 = 64)
            workingIndex++;
            //If we run out of array space, draw what we have and start over
            if (workingIndex == ARRAY_LIMIT - 1)
            {
                TextRenderCall(workingIndex);
                workingIndex = 0;
            }
        }
    }
    TextRenderCall(workingIndex);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

void TextRenderer::TextRenderCall(int length)
{
    if (length > 0)
    {
        this->TextShader.SetMatrix4("transform", transforms[0], GL_TRUE, length);
        glUniform1iv(glGetUniformLocation(TextShader.ID, "letterMap"), length, &letterMap[0]);
        //I don't know how the above is working or why the below isn't!
        // this->TextShader.SetInteger2("letterMap", &letterMap[0], GL_TRUE, length);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, length);
    }
}

void TextRenderer::RenderTextRight(std::string text, GLfloat x, GLfloat y, GLfloat scale, int containerWidth, glm::vec3 color)
{
    GLfloat startX = x  + (containerWidth - GetTextWidth(text, scale));
    RenderText(text, startX, y, scale, color);
}
int TextRenderer::GetTextWidth(std::string text, GLfloat scale)
{
  //  scale *= 30.0f / 256.0f;
    int width = 0;
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
        Character ch = Characters[*c];

        GLfloat w = ch.Size.x * scale;
        GLfloat h = ch.Size.y * scale;
        width += w;
    }
    return width;
}