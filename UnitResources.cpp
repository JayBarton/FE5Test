#include "UnitResources.h"
#include "TileManager.h"
#include "ResourceManager.h"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::vector<std::vector<glm::vec4>> UnitResources::unitUVs;
std::vector<AnimData> UnitResources::animData;

void UnitResources::LoadUVs()
{
	unitUVs.resize(14);

	unitUVs[0] = ResourceManager::GetTexture("sprites").GetUVs(0, 16, TileManager::TILE_SIZE, TileManager::TILE_SIZE, 3, 1);
	auto extras = ResourceManager::GetTexture("movesprites").GetUVs(640, 128, 32, 32, 4, 4);
	unitUVs[0].insert(unitUVs[0].end(), extras.begin(), extras.end());
	unitUVs[1] = ResourceManager::GetTexture("sprites").GetUVs(48, 48, TileManager::TILE_SIZE, TileManager::TILE_SIZE, 3, 1);
	extras = ResourceManager::GetTexture("movesprites").GetUVs(768, 128, 32, 32, 4, 4);
	unitUVs[1].insert(unitUVs[1].end(), extras.begin(), extras.end());
	unitUVs[2] = ResourceManager::GetTexture("sprites").GetUVs(96, 0, TileManager::TILE_SIZE, TileManager::TILE_SIZE, 3, 1);
	extras = ResourceManager::GetTexture("movesprites").GetUVs(256, 0, 32, 32, 4, 4);
	unitUVs[2].insert(unitUVs[2].end(), extras.begin(), extras.end());

	//Leif
	unitUVs[3] = ResourceManager::GetTexture("sprites").GetUVs(0, 0, TileManager::TILE_SIZE, TileManager::TILE_SIZE, 3, 1);
	extras = ResourceManager::GetTexture("movesprites").GetUVs(0, 0, 32, 32, 4, 4);
	unitUVs[3].insert(unitUVs[3].end(), extras.begin(), extras.end());
	//Finn
	unitUVs[4] = ResourceManager::GetTexture("sprites").GetUVs(0, 80, TileManager::TILE_SIZE, 20, 3, 1);
	extras = ResourceManager::GetTexture("movesprites").GetUVs(768, 0, 32, 32, 4, 4);
	unitUVs[4].insert(unitUVs[4].end(), extras.begin(), extras.end());
	//Eyvale
	unitUVs[5] = ResourceManager::GetTexture("sprites").GetUVs(45, 64, 15, TileManager::TILE_SIZE, 3, 1);
	extras = ResourceManager::GetTexture("movesprites").GetUVs(384, 128, 32, 32, 4, 4);
	unitUVs[5].insert(unitUVs[5].end(), extras.begin(), extras.end());
	//Halvan/Othin
	unitUVs[6] = ResourceManager::GetTexture("sprites").GetUVs(0, 64, 15, TileManager::TILE_SIZE, 3, 1);
	extras = ResourceManager::GetTexture("movesprites").GetUVs(640, 0, 32, 32, 4, 4);
	unitUVs[6].insert(unitUVs[6].end(), extras.begin(), extras.end());
	//Dagda
	unitUVs[7] = ResourceManager::GetTexture("sprites").GetUVs(96, 16, TileManager::TILE_SIZE, TileManager::TILE_SIZE, 3, 1);
	extras = ResourceManager::GetTexture("movesprites").GetUVs(512, 0, 32, 32, 4, 4);
	unitUVs[7].insert(unitUVs[7].end(), extras.begin(), extras.end());
	//Tanya
	unitUVs[8] = ResourceManager::GetTexture("sprites").GetUVs(0, 48, TileManager::TILE_SIZE, TileManager::TILE_SIZE, 3, 1);
	extras = ResourceManager::GetTexture("movesprites").GetUVs(512, 128, 32, 32, 4, 4);
	unitUVs[8].insert(unitUVs[8].end(), extras.begin(), extras.end());
	//Marty
	unitUVs[9] = ResourceManager::GetTexture("sprites").GetUVs(48, 16, TileManager::TILE_SIZE, TileManager::TILE_SIZE, 3, 1);
	extras = ResourceManager::GetTexture("movesprites").GetUVs(384, 0, 32, 32, 4, 4);
	unitUVs[9].insert(unitUVs[9].end(), extras.begin(), extras.end());

	unitUVs[10] = ResourceManager::GetTexture("sprites").GetUVs(48, 0, TileManager::TILE_SIZE, TileManager::TILE_SIZE, 3, 1);
	extras = ResourceManager::GetTexture("movesprites").GetUVs(128, 0, 32, 32, 4, 4);
	unitUVs[10].insert(unitUVs[10].end(), extras.begin(), extras.end());

	extras = ResourceManager::GetTexture("movesprites").GetUVs(384, 128, 32, 32, 4, 4);
	unitUVs[11] = ResourceManager::GetTexture("sprites").GetUVs(96, 32, TileManager::TILE_SIZE, TileManager::TILE_SIZE, 3, 1);
	extras = ResourceManager::GetTexture("movesprites").GetUVs(256, 128, 32, 32, 4, 4);
	unitUVs[11].insert(unitUVs[11].end(), extras.begin(), extras.end());

	unitUVs[12] = ResourceManager::GetTexture("sprites").GetUVs(48, 32, TileManager::TILE_SIZE, TileManager::TILE_SIZE, 3, 1);
	extras = ResourceManager::GetTexture("movesprites").GetUVs(128, 128, 32, 32, 4, 4);
	unitUVs[12].insert(unitUVs[12].end(), extras.begin(), extras.end());

	unitUVs[13] = ResourceManager::GetTexture("sprites").GetUVs(0, 32, TileManager::TILE_SIZE, TileManager::TILE_SIZE, 3, 1);
	extras = ResourceManager::GetTexture("movesprites").GetUVs(0, 128, 32, 32, 4, 4);
	unitUVs[13].insert(unitUVs[13].end(), extras.begin(), extras.end());
}

void UnitResources::LoadAnimData()
{
	std::ifstream f("AnimData.json");
	json data = json::parse(f);
	json anim = data["AnimData"];
	animData.resize(14);
	int current = 0;
	for (const auto& data : anim)
	{
		int face = data["FocusFace"];
		glm::ivec2 size = glm::ivec2(data["Size"][0], data["Size"][1]);
		glm::ivec2 offset = glm::ivec2(data["Offset"][0], data["Offset"][1]);
		animData[current] = AnimData{ face, size, offset };
		current++;
	}
}
