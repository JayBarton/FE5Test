#include "MenuManager.h"
#include "Cursor.h"
#include "TextRenderer.h"
#include "Camera.h"
#include "InputManager.h"

#include <SDL.h>

Menu::Menu(Cursor* Cursor, TextRenderer* Text, Camera* Camera, int shapeVAO) : cursor(Cursor), text(Text), camera(Camera), shapeVAO(shapeVAO)
{
}
void Menu::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (inputManager.isKeyPressed(SDLK_UP))
	{
		currentOption--;
		if (currentOption < 0)
		{
			currentOption = optionsVector.size() - 1;
		}
		std::cout << currentOption << std::endl;
	}
	if (inputManager.isKeyPressed(SDLK_DOWN))
	{
		currentOption++;
		if (currentOption >= optionsVector.size())
		{
			currentOption = 0;
		}
		std::cout << currentOption << std::endl;
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

UnitOptionsMenu::UnitOptionsMenu(Cursor* Cursor, TextRenderer* Text, Camera* Camera, int shapeVAO) : Menu(Cursor, Text, Camera, shapeVAO)
{
	GetUnitOptions();
}

void UnitOptionsMenu::SelectOption()
{
	switch (optionsVector[currentOption])
	{
	case ATTACK:
		std::cout << "Attack here eventually\n";
		break;
	case ITEMS:
		MenuManager::menuManager.AddMenu(1);
		std::cout << "Items here eventually\n";
		break;
	case DISMOUNT:
		std::cout << "Dismount here eventually\n";
		break;
		//Wait
	default:
		cursor->Wait();
		ClearMenu();
		break;
	}
}

void UnitOptionsMenu::CancelOption()
{
	cursor->UndoMove();
	Menu::CancelOption();
}

void UnitOptionsMenu::Draw()
{
//	int xStart = SCREEN_WIDTH;
	int xStart = 800;
	int yOffset = 100;
	glm::vec2 fixedPosition = camera->worldToScreen(cursor->position);
	if (fixedPosition.x >= camera->screenWidth * 0.5f)
	{
		//	xStart = 178;
	}
	//ResourceManager::GetShader("shape").Use().SetMatrix4("projection", glm::ortho(0.0f, 800.0f, 600.0f, 0.0f));
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(176, 32 + (12 * currentOption), 0.0f));

	model = glm::scale(model, glm::vec3(16, 16, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
	//Just a little test with new line
	std::string commands = "";
	if (canAttack)
	{
		commands += "Attack\n";
		text->RenderText("Attack", xStart - 200, yOffset, 1);
		yOffset += 30;
	}
	commands += "Items\n";
	text->RenderText("Items", xStart - 200, yOffset, 1);
	yOffset += 30;
	if (canDismount)
	{
		commands += "Dismount\n";
		text->RenderText("Dismount", xStart - 200, yOffset, 1);
		yOffset += 30;
	}
	commands += "Wait";
	text->RenderText("Wait", xStart - 200, yOffset, 1);
	//Text->RenderText(commands, xStart - 200, 100, 1);
}

void UnitOptionsMenu::GetUnitOptions()
{
	currentOption = 0;
	canAttack = false;
	canDismount = false;
	optionsVector.clear();
	optionsVector.reserve(5);
	auto units = cursor->inRangeUnits();
	if (units.size() > 0)
	{
		canAttack = true;
		optionsVector.push_back(ATTACK);
	}
	optionsVector.push_back(ITEMS);
	//if can dismount
	canDismount = true;
	optionsVector.push_back(DISMOUNT);
	optionsVector.push_back(WAIT);
}

ItemOptionsMenu::ItemOptionsMenu(Cursor* Cursor, TextRenderer* Text, Camera* Camera, int shapeVAO) : Menu(Cursor, Text, Camera, shapeVAO)
{
}

void ItemOptionsMenu::Draw()
{
	text->RenderText("Item menu coming soon", 0, 0, 1);
}

void ItemOptionsMenu::SelectOption()
{
	std::cout << "Coming soon\n";
}

MenuManager MenuManager::menuManager;
void MenuManager::SetUp(Cursor* Cursor, TextRenderer* Text, Camera* Camera, int shapeVAO)
{
	cursor = Cursor;
	text = Text;
	camera = Camera;
	this->shapeVAO = shapeVAO;
}

void MenuManager::AddMenu(int ID)
{
	//I don't know if this ID system will stick around
	if (ID == 0)
	{
		Menu* newMenu = new UnitOptionsMenu(cursor, text, camera, shapeVAO);
		menus.push_back(newMenu);
	}
	else if (ID == 1)
	{
		Menu* newMenu = new ItemOptionsMenu(cursor, text, camera, shapeVAO);
		menus.push_back(newMenu);
	}
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