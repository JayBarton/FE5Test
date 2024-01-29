#pragma once
#include <string>
#include <random>
#include <iostream>
#include <chrono>
#include <random>
#include "Sprite.h"
class SpriteRenderer;

struct StatGrowths
{
	int maxHP;
	int strength;
	int magic;
	int skill;
	int speed;
	int luck;
	int defense;
	int build;
	int move;
};

struct Unit
{
	Unit();

	//Not really sure where I'm passing this in, but the units should have a reference to the generator I think
	void init(std::mt19937* gen, std::uniform_int_distribution<int>* distribution);

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
	int level = 1;
	//Also need weapon ranks. I'm thinking this might be some sort of array
	//Also need skills, no idea how to implement yet.

	int currentHP;
	//Not a huge deal, but I wonder if this is needed here, since enemies don't use it.
	int experience = 0;

	Sprite sprite;
	StatGrowths growths;

	std::mt19937 *gen = nullptr;
	std::uniform_int_distribution<int> *distribution = nullptr;

	void placeUnit(int x, int y);
	void Update(float deltaTime);
	void Draw(SpriteRenderer* Renderer);

	void LevelUp();
	void AddExperience(int exp);
};