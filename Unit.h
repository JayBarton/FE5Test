#pragma once
#include <string>
#include "Sprite.h"
class SpriteRenderer;
struct Unit
{
	Unit();

	void init();

	//Unit properties
	std::string name;
	int unitClass; //This will determine the growths I think
	int maxHP;
	int strength;
	int magic;
	int skill;
	int speed;
	int luck;
	int defense;
	int build;
	int move;
	int level;
	//Also need weapon ranks. I'm thinking this might be some sort of array
	//Also need skills, no idea how to implement yet.

	int currentHP;

	Sprite sprite;

	void placeUnit(int x, int y);
	void Update(float deltaTime);
	void Draw(SpriteRenderer* Renderer);
};