#pragma once
#include <string>
#include <cstdio>
#include <iostream>
#include <sstream>

// The Width of the screen
const GLuint SCREEN_WIDTH = 800;
// The height of the screen
const GLuint SCREEN_HEIGHT = 600;

static std::string intToString(int i)
{
	std::stringstream s;
	s << i;
	return s.str();
}