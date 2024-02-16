#include "MenuManager.h"
#include "TextRenderer.h"
#include "Camera.h"
#include "../InputManager.h"
#include "../csv.h"
#include "PlacementModes.h"
#include <SDL.h>

#include <sstream>
Menu::Menu(TextRenderer* Text, Camera* Camera, int shapeVAO) : text(Text), camera(Camera), shapeVAO(shapeVAO)
{
}

void Menu::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (inputManager.isKeyPressed(SDLK_UP))
	{
		currentOption--;
		if (currentOption < 0)
		{
			currentOption = numberOfOptions - 1;
		}
	}
	if (inputManager.isKeyPressed(SDLK_DOWN))
	{
		currentOption++;
		if (currentOption >= numberOfOptions)
		{
			currentOption = 0;
		}
	}
	if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
	if (inputManager.isKeyPressed(SDLK_z))
	{
		CancelOption();
		currentOption = 0;
	}
}

void Menu::CancelOption()
{
	MenuManager::menuManager.PreviousMenu();
}

void Menu::ClearMenu()
{
	MenuManager::menuManager.ClearMenu();
}

MenuManager MenuManager::menuManager;
void MenuManager::SetUp(TextRenderer* Text, Camera* Camera, int shapeVAO, SpriteRenderer* Renderer)
{
	renderer = Renderer;
	text = Text;
	camera = Camera;
	this->shapeVAO = shapeVAO;
}

void MenuManager::AddMenu(int ID)
{
}

void MenuManager::AddEnemyMenu(EnemyMode* mode)
{
	Menu* newMenu = new EnemyMenu(text, camera, shapeVAO, mode);
	MenuManager::menuManager.menus.push_back(newMenu);
}

void MenuManager::PreviousMenu()
{
	Menu* p = menus.back();
	menus.pop_back();
	delete p;
}

void MenuManager::ClearMenu()
{
	while (menus.size() > 0)
	{
		PreviousMenu();
	}
}

EnemyMenu::EnemyMenu(TextRenderer* Text, Camera* camera, int shapeVAO, EnemyMode* mode) : Menu(Text, camera, shapeVAO)
{
	this->mode = mode;

	int ID;
	int HP;
	int str;
	int mag;
	int skl;
	int spd;
	int lck;
	int def;
	int bld;
	int currentUnit = 0;

	io::CSVReader<9, io::trim_chars<' '>, io::no_quote_escape<':'>> in2("../EnemyGrowths.csv");
	in2.read_header(io::ignore_extra_column, "ID", "HP", "Str", "Mag", "Skl", "Spd", "Lck", "Def", "Bld");
	unitGrowths.resize(6);
	while (in2.read_row(ID, HP, str, mag, skl, spd, lck, def, bld)) {
		unitGrowths[currentUnit] = StatGrowths{ HP, str, mag, skl, spd, lck, def, bld, 0 };
		currentUnit++;
	}

	growthNames[0] = "Physical 1";
	growthNames[1] = "Magical 1";
	growthNames[2] = "Fighter 1";
	growthNames[3] = "Physical 2";
	growthNames[4] = "Magical 2";
	growthNames[5] = "Fighter 2";
}
std::string intToString2(int i)
{
	std::stringstream s;
	s << i;
	return s.str();
}
void EnemyMenu::Draw()
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(64 + 80 * !updatingLevel, 56, 0.0f));

	model = glm::scale(model, glm::vec3(16, 16, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	text->RenderText("Level : " + intToString2(level), 200, 200, 1);
	auto growths = unitGrowths[growthRateID];
	text->RenderText("Growth Pattern: " + growthNames[growthRateID], 400, 200, 1);

	text->RenderText("HP: " + intToString2(growths.maxHP), 400, 230, 1);
	text->RenderText("Str: " + intToString2(growths.strength), 400, 260, 1);
	text->RenderText("Mag: " + intToString2(growths.magic), 400, 290, 1);
	text->RenderText("Skl: " + intToString2(growths.skill), 400, 320, 1);
	text->RenderText("Spd: " + intToString2(growths.speed), 400, 350, 1);
	text->RenderText("Lck: " + intToString2(growths.luck), 400, 380, 1);
	text->RenderText("Def: " + intToString2(growths.defense), 400, 410, 1);
	text->RenderText("Bld: " + intToString2(growths.build), 400, 440, 1);

}

void EnemyMenu::SelectOption()
{
	mode->placeEnemy(level, growthRateID);
	CancelOption();
}

void EnemyMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (inputManager.isKeyPressed(SDLK_UP))
	{
		if (updatingLevel)
		{
			level++;
			if (level > 20)
			{
				level = 1;
			}
		}
		else
		{
			growthRateID++;
			if (growthRateID > 5)
			{
				growthRateID = 0;
			}
		}

	}
	if (inputManager.isKeyPressed(SDLK_DOWN))
	{
		if (updatingLevel)
		{
			level--;
			if (level < 1)
			{
				level = 20;
			}
		}
		else
		{
			growthRateID--;
			if (growthRateID < 0)
			{
				growthRateID = 5;
			}
		}
	}
	if (inputManager.isKeyPressed(SDLK_TAB))
	{
		updatingLevel = !updatingLevel;
	}
	if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
	if (inputManager.isKeyPressed(SDLK_z))
	{
		CancelOption();
		currentOption = 0;
	}
}