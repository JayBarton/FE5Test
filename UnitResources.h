#pragma once
#include <vector>
#include <glm.hpp>

//Don't know if this needs to exist, I could potentially just do all this in main
//Leave it here for now
struct UnitResources
{
	static std::vector<std::vector<glm::vec4>> unitUVs;

	static void LoadUVs();
};