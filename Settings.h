#pragma once

struct Settings
{
	float unitSpeed = 2.5f;
	int textSpeed = 1;
	int volume = 4;
	int mapAnimations = 1;
	bool autoCursor = true;
	bool showTerrain = true;
	bool sterero = true;
	bool music = true;

	static Settings settings;
};