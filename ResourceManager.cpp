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

#include "ResourceManager.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <unordered_map>

#include <SDL.h>
#include <SDL_Image.h>

// Instantiate static variables
std::map<std::string, Texture2D>    ResourceManager::Textures;
std::map<std::string, Shader>       ResourceManager::Shaders;
std::map<std::string, Mix_Chunk*>   ResourceManager::Sounds;

Shader ResourceManager::LoadShader(const GLchar *vShaderFile, const GLchar *fShaderFile, const GLchar *gShaderFile, std::string name)
{
    Shaders[name] = loadShaderFromFile(vShaderFile, fShaderFile, gShaderFile);
    return Shaders[name];
}

Shader ResourceManager::GetShader(std::string name)
{
    return Shaders[name];
}

Texture2D ResourceManager::LoadTexture(const GLchar *file, std::string name)
{
    Textures[name] = loadTextureFromFile(file);
    return Textures[name];
}

Texture2D ResourceManager::LoadTexture2(const GLchar* file, std::string name)
{
    Textures[name] = loadTextureFromFile2(file);
    return Textures[name];
}

Texture2D ResourceManager::GetTexture(std::string name)
{
    return Textures[name];
}

Mix_Chunk* ResourceManager::LoadSound(const GLchar* file, std::string name)
{
    Mix_Chunk* sound = Mix_LoadWAV(file);
	if (sound == nullptr)
	{
		printf("Mix_LoadWAV error: %s\n", Mix_GetError());
	}
	else
	{
        Sounds[name] = sound;
        return Sounds[name];
	}
}

void ResourceManager::PlaySound(std::string name)
{
    Mix_PlayChannel(-1, Sounds[name], 0);
}


void ResourceManager::Clear()
{
    // (Properly) delete all shaders
    for (auto iter : Shaders)
    {
        glDeleteProgram(iter.second.ID);
    }
    // (Properly) delete all textures
    for (auto iter : Textures)
    {
        glDeleteTextures(1, &iter.second.ID);
    }
    for(auto iter : Sounds)
    {
        Mix_FreeChunk(iter.second);
        iter.second = nullptr;
    }
}

Shader ResourceManager::loadShaderFromFile(const GLchar *vShaderFile, const GLchar *fShaderFile, const GLchar *gShaderFile)
{
    // 1. Retrieve the vertex/fragment source code from filePath
    std::string vertexCode;
    std::string fragmentCode;
    std::string geometryCode;
    try
    {
        // Open files
        std::ifstream vertexShaderFile(vShaderFile);
        std::ifstream fragmentShaderFile(fShaderFile);
        std::stringstream vShaderStream, fShaderStream;
        // Read file's buffer contents into streams
        vShaderStream << vertexShaderFile.rdbuf();
        fShaderStream << fragmentShaderFile.rdbuf();
        // close file handlers
        vertexShaderFile.close();
        fragmentShaderFile.close();
        // Convert stream into string
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
        // If geometry shader path is present, also load a geometry shader
        if (gShaderFile != nullptr)
        {
            std::ifstream geometryShaderFile(gShaderFile);
            std::stringstream gShaderStream;
            gShaderStream << geometryShaderFile.rdbuf();
            geometryShaderFile.close();
            geometryCode = gShaderStream.str();
        }
    }
    catch (std::exception e)
    {
        std::cout << "ERROR::SHADER: Failed to read shader files" << std::endl;
    }
    const GLchar *vShaderCode = vertexCode.c_str();
    const GLchar *fShaderCode = fragmentCode.c_str();
    const GLchar *gShaderCode = geometryCode.c_str();
    // 2. Now create shader object from source code
    Shader shader;
    shader.Compile(vShaderCode, fShaderCode, gShaderFile != nullptr ? gShaderCode : nullptr);
    return shader;
}

Texture2D ResourceManager::loadTextureFromFile(const GLchar *file)
{
    // Create Texture object
    Texture2D texture;
    // Load image
    SDL_Surface* surface = IMG_Load(file);
    if( surface == NULL )
    {
        printf( "Unable to load image %s! SDL_image Error: %s\n", file, IMG_GetError() );
    }
    if(surface->format->BytesPerPixel == 4)
    {
        texture.Internal_Format = GL_RGBA;
        texture.Image_Format = GL_RGBA;
    }


    // Now generate texture
    texture.Generate(surface->w, surface->h, (unsigned char*)surface->pixels);
    // And finally free image data
    SDL_FreeSurface(surface);
    return texture;
}

//Okay, the plan here is for palette swapping. The idea is I go through my sprite sheet, find the pixels that match my palette,
//and then set the r component of this texture to the index on the palette. I can then use that index to draw the proper palette.
//This was a massive pain in the ass which is a shame because I don't know if I'll keep it. We'll see how long it takes to run this once the
//Sprite sheet is larger, since I imagine it will be quite large, it might be faster to just have a duplicate sprite sheet.
Texture2D ResourceManager::loadTextureFromFile2(const GLchar* file)
{
    // Create Texture object
    Texture2D texture;
    // Load image
    SDL_Surface* surface = IMG_Load(file);
    SDL_Surface* paletteSurface = IMG_Load("E:/Damon/dev stuff/FE5Test/TestSprites/palette.png");
    if (surface == NULL)
    {
        printf("Unable to load image %s! SDL_image Error: %s\n", file, IMG_GetError());
    }
    if (surface->format->BytesPerPixel == 4)
    {
        texture.Internal_Format = GL_RGBA;
        texture.Image_Format = GL_RGBA;
    }
    //What I'm doing here is using the pixel data of the palette colors as a key into this map
    //When I later go to set the index to the sprite's r channel, I can read it from this map
    Uint32* palettePixels = (Uint32*)paletteSurface->pixels;
    std::map<Uint32, int> paletteMap;
    for (int i = 0; i < 16; ++i)
    {
        Uint32 palettePixel = palettePixels[i]; 
        paletteMap[palettePixel] = i;
    }
    Uint32* pixels = (Uint32*)surface->pixels;
    for (int y = 0; y < surface->h; ++y) 
    {
        for (int x = 0; x < surface->w; ++x)
        {
            Uint32 pixel = pixels[y * surface->w + x];

            //This gets me the alpha channel of the pixel
            Uint8 a = (pixel >> 24) & 0xFF;
            //Don't need to check transparent pixels
            if (a > 0)
            {
                int index = paletteMap[pixel];

                //I barely understand this. 0xFFFFFF00 means keep everything except for the r channel, set r to 0. 
                //The FFs correspond to ABG and the 00 to R. No idea why it's backwards. And then I think the | i is setting anything not FF'd to i.
                //I also do not know why multiplying by 16 is needed or even works at all, but I discovered it in the shader initially and it
                pixels[y * surface->w + x] = (pixel & 0xFFFFFF00) | index * 16;
            }
        }
    }
    // Now generate texture
    texture.Generate(surface->w, surface->h, (unsigned char*)surface->pixels);
    // And finally free image data
    SDL_FreeSurface(surface);
    SDL_FreeSurface(paletteSurface);
    return texture;
}