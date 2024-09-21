#pragma once
#include <vector>

struct Settings
{
	float unitSpeed = 2.5f;
	std::vector<std::vector<int>> defaultColors = {
		{96, 0, 0, 0, 0, 192},
		{0, 0, 96, 192, 192, 192}
	};

	std::vector<std::vector<int>> backgroundColors;

	int textSpeed = 1;
	int volume = 4;
	int mapAnimations = 1;

	int backgroundPattern = 1;

	bool autoCursor = true;
	bool showTerrain = true;
	bool sterero = true;
	bool music = true;

	std::vector<bool> editedColor;

	static Settings settings;
};