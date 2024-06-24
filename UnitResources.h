#pragma once
#include <vector>
#include <glm.hpp>

struct AnimData
{
	int facing;
	glm::ivec2 size;
	glm::ivec2 offset;
};

//Don't know if this needs to exist, I could potentially just do all this in main
//Leave it here for now
struct UnitResources
{
	static std::vector<std::vector<glm::vec4>> unitUVs;
	static std::vector<AnimData> animData;

	static void LoadUVs();
	static void LoadAnimData();
};