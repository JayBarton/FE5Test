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

#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <map>
#include <string>

#include "Texture.h"
#include "Shader.h"
#include <SDL_mixer.h>

// A static singleton ResourceManager class that hosts several
// functions to load Textures and Shaders. Each loaded texture
// and/or shader is also stored for future reference by string
// handles. All functions and resources are static and no
// public constructor is defined.
class ResourceManager
{
public:
    // Resource storage
    static std::map<std::string, Shader>    Shaders;
    static std::map<std::string, Texture2D> Textures;
    static std::map<std::string, Mix_Chunk*> Sounds;
    static std::map<std::string, Mix_Music*> Music;
    // Loads (and generates) a shader program from file loading vertex, fragment (and geometry) shader's source code. If gShaderFile is not nullptr, it also loads a geometry shader
    static Shader   LoadShader(const GLchar *vShaderFile, const GLchar *fShaderFile, const GLchar *gShaderFile, std::string name);
    // Retrieves a stored sader
    static Shader   GetShader(std::string name);
    // Loads (and generates) a texture from file
    static Texture2D LoadTexture(const GLchar *file, std::string name);
    static Texture2D LoadTexture2(const GLchar *file, std::string name);
    // Retrieves a stored texture
    static Texture2D GetTexture(std::string name);

    static Mix_Chunk* LoadSound(const GLchar* file, std::string name);
    static int PlaySound(std::string name, int channel = -1, bool delay = false);
    static void StopSound(int channel);
    static bool IsPlayingChannel(int channel);

    static Mix_Music* LoadMusic(const GLchar* file, std::string name);
    static void PlayMusic(std::string name, int loop = -1);
    static void PlayMusic(std::string name, std::string next);
    static void PlayNextSong();

    static void FadeOutPause(int ms);
    static void PauseMusic();
    static void ResumeMusic(int);

    static bool pausedMusic;

    // Properly de-allocates all loaded resources
    static void      Clear();
private:
    // Private constructor, that is we do not want any actual resource manager objects. Its members and functions should be publicly available (static).
    ResourceManager() { }
    // Loads and generates a shader from file
    static Shader    loadShaderFromFile(const GLchar *vShaderFile, const GLchar *fShaderFile, const GLchar *gShaderFile = nullptr);
    // Loads a single texture from file
    static Texture2D loadTextureFromFile(const GLchar *file);
    static Texture2D loadTextureFromFile2(const GLchar *file);

    static std::string nextSong;
    static std::string currentSong;
    static std::string pausedSong;
    static std::string pausedNextSong;
    static double pausedTime;
};

#endif // RESOURCEMANAGER_H
