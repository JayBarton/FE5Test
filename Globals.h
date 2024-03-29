#pragma once
#include <string>
#include <cstdio>
#include <iostream>
#include <sstream>

// The Width of the screen
const GLuint SCREEN_WIDTH = 800;
// The height of the screen
const GLuint SCREEN_HEIGHT = 600;

//I don't like this I'm not keeping it
#ifndef GLOBALS_H
#define GLOBALS_H
extern float unitSpeed;
#endif
static float heldSpeed = 7.5f;

static std::string intToString(int i)
{
	std::stringstream s;
	s << i;
	return s.str();
}