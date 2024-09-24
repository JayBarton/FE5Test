#include "MenuManager.h"
#include "TextRenderer.h"
#include "Camera.h"
#include "../InputManager.h"
#include "../csv.h"
#include "../SceneActions.h"
#include "PlacementModes.h"
#include <SDL.h>

#include <sstream>

#include <fstream>  
#include <nlohmann/json.hpp>
using json = nlohmann::json;

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
	}
}

void Menu::CancelOption()
{
	MenuManager::menuManager.PreviousMenu();
	currentOption = 0;
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

void MenuManager::AddEnemyMenu(EnemyMode* mode, Object* obj)
{
	Menu* newMenu = new EnemyMenu(text, camera, shapeVAO, mode, obj);
	MenuManager::menuManager.menus.push_back(newMenu);
}

void MenuManager::AddInventoryMenu(EnemyMode* mode, Object* obj, std::vector<int>& inventory, std::vector<Item>& items)
{
	Menu* newMenu = new InventoryMenu(text, camera, shapeVAO, mode, obj, inventory, items);
	MenuManager::menuManager.menus.push_back(newMenu);
}

void MenuManager::AddStatsMenu(EnemyMode* mode, Object* obj, std::vector<int>& baseStats, bool& editedStats)
{
	Menu* newMenu = new StatsMenu(text, camera, shapeVAO, mode, obj, baseStats, editedStats);
	MenuManager::menuManager.menus.push_back(newMenu);
}

void MenuManager::AddProfsMenu(EnemyMode* mode, Object* obj, std::vector<int>& weaponProfs, bool& editedProfs)
{
	Menu* newMenu = new ProfsMenu(text, camera, shapeVAO, mode, obj, weaponProfs, editedProfs);
	MenuManager::menuManager.menus.push_back(newMenu);
}

void MenuManager::AddVendorMenu(VendorMode* mode, Vendor* vendor)
{
	Menu* newMenu = new VendorMenu(text, camera, shapeVAO, vendor);
	MenuManager::menuManager.menus.push_back(newMenu);
}

void MenuManager::OpenSceneMenu(std::vector<SceneObjects*>& sceneObjects, std::vector<VisitObjects>& visitObjects)
{
	Menu* newMenu = new SceneMenu(text, camera, shapeVAO, sceneObjects, visitObjects);
	MenuManager::menuManager.menus.push_back(newMenu);
}

void MenuManager::OpenActionMenu(SceneObjects& sceneObject)
{
	Menu* newMenu = new SceneActionMenu(text, camera, shapeVAO, sceneObject);
	MenuManager::menuManager.menus.push_back(newMenu);
}

void MenuManager::OpenVisitMenu(VisitObjects& visitObject)
{
	Menu* newMenu = new VisitMenu(text, camera, shapeVAO, visitObject);
	MenuManager::menuManager.menus.push_back(newMenu);
}

void MenuManager::OpenActivationMenu(SceneObjects& sceneObject)
{
	Menu* newMenu = new SceneActivationMenu(text, camera, shapeVAO, sceneObject);
	MenuManager::menuManager.menus.push_back(newMenu);
}

void MenuManager::OpenTalkMenu(SceneObjects& sceneObject)
{
	Menu* newMenu = new TalkActivationMenu(text, camera, shapeVAO, sceneObject);
	MenuManager::menuManager.menus.push_back(newMenu);
}

void MenuManager::SelectOptionMenu(int action, std::vector<SceneAction*>& sceneActions)
{
	Menu* newMenu = nullptr;
	switch (action)
	{
	case CAMERA_ACTION:
		newMenu = new CameraActionMenu(text, camera, shapeVAO, sceneActions);
		break;
	case NEW_UNIT_ACTION:
		newMenu = new NewUnitActionMenu(text, camera, shapeVAO, sceneActions);
		break;
	case MOVE_UNIT_ACTION:
		newMenu = new MoveUnitActionMenu(text, camera, shapeVAO, sceneActions);
		break;
	case DIALOGUE_ACTION:
		newMenu = new DialogueActionMenu(text, camera, shapeVAO, sceneActions);
		break;
	case ITEM_ACTION:
		newMenu = new ItemActionMenu(text, camera, shapeVAO, sceneActions);
		break;
	case NEW_SCENE_UNIT_ACTION:
		newMenu = new NewSceneUnitActionMenu(text, camera, shapeVAO, sceneActions);
		break;
	case SCENE_UNIT_MOVE_ACTION:
		newMenu = new SceneUnitMoveActionMenu(text, camera, shapeVAO, sceneActions);
		break;
	case SCENE_UNIT_REMOVE_ACTION:
		newMenu = new RemoveSceneUnitActionMenu(text, camera, shapeVAO, sceneActions);
		break;
	case START_MUSIC:
		newMenu = new StartMusicActionMenu(text, camera, shapeVAO, sceneActions);
		break;
	case STOP_MUSIC:
		newMenu = new StopMusicActionMenu(text, camera, shapeVAO, sceneActions);
		break;
	case SHOW_MAP_TITLE:
		newMenu = new ShowTitleActionMenu(text, camera, shapeVAO, sceneActions);
		break;
	}
	MenuManager::menuManager.menus.push_back(newMenu);
}

void MenuManager::OpenRequirementsMenu(std::vector<std::pair<int, int>>& requiredUnits)
{
	Menu* newMenu = new RequireUnitsMenu(text, camera, shapeVAO, requiredUnits);
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

bool IDLocation(const json& enemy, int ID)
{
	return enemy["ID"] == ID;
}

EnemyMenu::EnemyMenu(TextRenderer* Text, Camera* camera, int shapeVAO, EnemyMode* mode, Object* object) : Menu(Text, camera, shapeVAO)
{
	pageOptions[0] = 3;
	pageOptions[1] = 2;
	pageOptions[2] = 3;
	currentOption = 0;
	this->mode = mode;
	this->object = object;

	std::ifstream f("../BaseStats.json");
	json data = json::parse(f);
	json bases = data["classes"];
	//int currentUnit = 0;
	weaponNameMap["Sword"] = WeaponData::TYPE_SWORD;
	weaponNameMap["Axe"] = WeaponData::TYPE_AXE;
	weaponNameMap["Lance"] = WeaponData::TYPE_LANCE;
	weaponNameMap["Bow"] = WeaponData::TYPE_BOW;
	weaponNameMap["Thunder"] = WeaponData::TYPE_THUNDER;
	weaponNameMap["Fire"] = WeaponData::TYPE_FIRE;
	weaponNameMap["Wind"] = WeaponData::TYPE_WIND;
	weaponNameMap["Dark"] = WeaponData::TYPE_DARK;
	weaponNameMap["Light"] = WeaponData::TYPE_LIGHT;
	weaponNameMap["Staff"] = WeaponData::TYPE_STAFF;

	weaponNamesArray[WeaponData::TYPE_SWORD] = "Sword";
	weaponNamesArray[WeaponData::TYPE_AXE] = "Axe";
	weaponNamesArray[WeaponData::TYPE_LANCE] = "Lance";
	weaponNamesArray[WeaponData::TYPE_BOW] = "Bow";
	weaponNamesArray[WeaponData::TYPE_THUNDER] = "Thunder";
	weaponNamesArray[WeaponData::TYPE_FIRE] = "Fire";
	weaponNamesArray[WeaponData::TYPE_WIND] = "Wind";
	weaponNamesArray[WeaponData::TYPE_DARK] = "Dark";
	weaponNamesArray[WeaponData::TYPE_LIGHT] = "Light";
	weaponNamesArray[WeaponData::TYPE_STAFF] = "Staff";
	baseStats.resize(9);
	weaponProficiencies.resize(10);

	int desiredID;
	if (object)
	{
		editedStats = object->editedStats;
		editedProfs = object->editedProfs;
		level = object->level;
		growthRateID = object->growthRateID;
		inventory = object->inventory;

		desiredID = object->type;
		activationType = object->activationType;
		stationary = object->stationary;
		bossBonus = object->bossBonus;
		sceneID = object->sceneID;
	}
	else
	{
		desiredID = mode->currentElement;
	}
	auto it = std::find_if(bases.begin(), bases.end(), std::bind(IDLocation, std::placeholders::_1, desiredID));;
	className = (*it)["Name"];
	
	if (editedStats)
	{
		baseStats = object->stats;
	}
	else
	{
		auto stats = (*it)["Stats"];
		baseStats[0] = stats["HP"];
		baseStats[1] = stats["Str"];
		baseStats[2] = stats["Mag"];
		baseStats[3] = stats["Skl"];
		baseStats[4] = stats["Spd"];
		baseStats[5] = stats["Lck"];
		baseStats[6] = stats["Def"];
		baseStats[7] = stats["Bld"];
		baseStats[8] = stats["Mov"];
	}
	if (editedProfs)
	{
		weaponProficiencies = object->profs;
	}
	else
	{
		auto weaponProf = (*it)["WeaponProf"];
		for (auto it = weaponProf.begin(); it != weaponProf.end(); ++it)
		{
			weaponProficiencies[weaponNameMap[it.key()]] = int(it.value());
		}
	}

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

	io::CSVReader<8, io::trim_chars<' '>, io::no_quote_escape<':'>> in("../items.csv");
	in.read_header(io::ignore_extra_column, "ID", "name", "maxUses", "useID", "isWeapon", "canDrop", "description", "value");
	std::string name;
	int maxUses;
	int useID;
	int isWeapon;
	int canDrop;
	int value;
	std::string description;
	while (in.read_row(ID, name, maxUses, useID, isWeapon, canDrop, description, value))
	{
		//csv reader reads in new lines wrong for whatever reason, this fixes it.
		size_t found = description.find("\\n");
		while (found != std::string::npos)
		{
			description.replace(found, 2, "\n");
			found = description.find("\\n", found + 1);
		}
		items.push_back({ ID, name, maxUses, maxUses, useID, value, bool(isWeapon), bool(canDrop), description });
	}
}

void EnemyMenu::Draw()
{
	if (page == 0)
	{
		ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
		ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
		glm::mat4 model = glm::mat4();
		model = glm::translate(model, glm::vec3(32 + 64 * currentOption, 56, 0.0f));

		model = glm::scale(model, glm::vec3(16, 16, 0.0f));

		ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

		ResourceManager::GetShader("shape").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		text->RenderText("Basics", 528, TileManager::TILE_SIZE, 1);

		text->RenderText("Level : " + intToString(level), 100, 200, 1);
		auto growths = unitGrowths[growthRateID];
		text->RenderText("Growth Pattern: " + growthNames[growthRateID], 200, 200, 1);

		text->RenderText("HP: " + intToString(growths.maxHP), 200, 230, 1);
		text->RenderText("Str: " + intToString(growths.strength), 200, 260, 1);
		text->RenderText("Mag: " + intToString(growths.magic), 200, 290, 1);
		text->RenderText("Skl: " + intToString(growths.skill), 200, 320, 1);
		text->RenderText("Spd: " + intToString(growths.speed), 200, 350, 1);
		text->RenderText("Lck: " + intToString(growths.luck), 200, 380, 1);
		text->RenderText("Def: " + intToString(growths.defense), 200, 410, 1);
		text->RenderText("Bld: " + intToString(growths.build), 200, 440, 1);

		text->RenderText("Inventory", 500, 200, 1);
		if (!inInventory)
		{
			for (int i = 0; i < inventory.size(); i++)
			{
				auto item = items[inventory[i]];
				text->RenderText(item.name, 500, 230 + 30 * i + 1, 1);
			}
		}
		text->RenderText("sceneID: " + intToString(sceneID), 200, 550, 1);

	}
	else if (page == 1)
	{
		ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
		ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
		glm::mat4 model = glm::mat4();
		model = glm::translate(model, glm::vec3(64 + 96 * currentOption, 56, 0.0f));

		model = glm::scale(model, glm::vec3(16, 16, 0.0f));

		ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

		ResourceManager::GetShader("shape").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		text->RenderText("Stats and Profciencies", 528, TileManager::TILE_SIZE, 1);

		text->RenderText("Level : " + intToString(level), 100, 200, 1);
		text->RenderText("Stats", 200, 200, 1);

		text->RenderText("HP: " + intToString(baseStats[0]), 200, 230, 1);
		text->RenderText("Str: " + intToString(baseStats[1]), 200, 260, 1);
		text->RenderText("Mag: " + intToString(baseStats[2]), 200, 290, 1);
		text->RenderText("Skl: " + intToString(baseStats[3]), 200, 320, 1);
		text->RenderText("Spd: " + intToString(baseStats[4]), 200, 350, 1);
		text->RenderText("Lck: " + intToString(baseStats[5]), 200, 380, 1);
		text->RenderText("Def: " + intToString(baseStats[6]), 200, 410, 1);
		text->RenderText("Bld: " + intToString(baseStats[7]), 200, 440, 1);
		text->RenderText("Mov: " + intToString(baseStats[8]), 200, 470, 1);

		text->RenderText("Weapon Profciencies", 500, 200, 1);
		int offset = 30;
		for (int i = 0; i < 10; i++)
		{
			text->RenderText(weaponNamesArray[i], 500, 200 + offset, 1);
			text->RenderText(intToString(weaponProficiencies[i]), 580, 200 + offset, 1);
			offset += 30;
		}
	}
	else
	{
		ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
		ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
		glm::mat4 model = glm::mat4();
		model = glm::translate(model, glm::vec3(64 + 64 * currentOption, 56, 0.0f));

		model = glm::scale(model, glm::vec3(16, 16, 0.0f));

		ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

		ResourceManager::GetShader("shape").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		text->RenderText("AI Options", 528, TileManager::TILE_SIZE, 1);

		std::string activationTypeString = "Attack";
		if (activationType == 1)
		{
			activationTypeString = "Range";
		}
		else if (activationType == 2)
		{			
			activationTypeString = "Attack Range";
		}
		std::string stationaryYesNo = "No";
		std::string bossYesNo = "No";
		if (stationary)
		{
			stationaryYesNo = "Yes";
		}
		if (bossBonus)
		{
			bossYesNo = "Yes";
		}
		text->RenderText("Activation type : " + activationTypeString, 100, 200, 1);
		text->RenderText("Stationary? " + stationaryYesNo, 400, 200, 1);
		text->RenderText("Boss? " + bossYesNo, 600, 200, 1);
	}
	text->RenderText(className, 100, 110, 1);
}

void EnemyMenu::SelectOption()
{
	if (object)
	{
		mode->updateEnemy(level, growthRateID, inventory, object->type, baseStats, editedStats, weaponProficiencies, editedProfs, activationType, stationary, bossBonus, sceneID);
	}
	else
	{
		mode->placeEnemy(level, growthRateID, inventory, baseStats, editedStats, weaponProficiencies, editedProfs, activationType, stationary, bossBonus, sceneID);
	}
	CancelOption();
}

void EnemyMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (inputManager.isKeyPressed(SDLK_UP))
	{
		if (currentOption == CHANGE_LEVEL)
		{
			level++;
			if (level > 20)
			{
				level = 1;
			}
		}
		else if(currentOption == CHANGE_GROWTH)
		{
			growthRateID++;
			if (growthRateID > 5)
			{
				growthRateID = 0;
			}
		}

	}
	else if (inputManager.isKeyPressed(SDLK_DOWN))
	{
		if (page == 0)
		{
			if (currentOption == CHANGE_LEVEL)
			{
				level--;
				if (level < 1)
				{
					level = 20;
				}
			}
			else if (currentOption == CHANGE_GROWTH)
			{
				growthRateID--;
				if (growthRateID < 0)
				{
					growthRateID = 5;
				}
			}
			else if (currentOption == OPEN_INVENTORY)
			{
				MenuManager::menuManager.AddInventoryMenu(mode, object, inventory, items);
				inInventory = true;
			}
		}
		else if (page == 1)
		{
			if (currentOption == CHANGE_STATS)
			{
				MenuManager::menuManager.AddStatsMenu(mode, object, baseStats, editedStats);
			}
			else if (currentOption == CHANGE_PROFS)
			{
				MenuManager::menuManager.AddProfsMenu(mode, object, weaponProficiencies, editedProfs);
			}
		}
		else
		{
			if (currentOption == ACTIVATION_OPTION)
			{
				activationType++;
				if (activationType > 2)
				{
					activationType = 0;
				}
			}
			else if (currentOption == STATIONARY_OPTION)
			{
				stationary = !stationary;
			}
			else if (currentOption == BOSS_OPTION)
			{
				bossBonus = !bossBonus;
			}
		}
	}
	if (inputManager.isKeyPressed(SDLK_RIGHT))
	{
		currentOption++;
		if (currentOption >= pageOptions[page])
		{
			currentOption = 0;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_LEFT))
	{
		currentOption--;
		if (currentOption < 0)
		{
			currentOption = numberOfOptions -1;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
	else if (inputManager.isKeyPressed(SDLK_z))
	{
		CancelOption();
		currentOption = 0;
	}
	else if (inputManager.isKeyPressed(SDLK_TAB))
	{
		page++;
		if (page > 2)
		{
			page = 0;
		}
		currentOption = 0;
	}
	//Just putting this here for now because I'm lazy, need a better way of handling this ultimately
	else if (inputManager.isKeyPressed(SDLK_w))
	{
		sceneID++;
	}
	else if (inputManager.isKeyPressed(SDLK_s))
	{
		sceneID--;
	}
}

InventoryMenu::InventoryMenu(TextRenderer* Text, Camera* camera, int shapeVAO,
	class EnemyMode* mode, class Object* object, std::vector<int>& inventory, std::vector<Item>& items)
	: Menu(Text, camera, shapeVAO), inventory(inventory), items(items)
{
	currentSlot = 0;
	if (currentSlot < inventory.size())
	{
		currentItem = inventory[currentSlot];
	}
}

void InventoryMenu::SelectOption()
{
	if (inventory.size() > currentSlot)
	{
		//update inventory slot
		inventory[currentSlot] = currentItem;
	}
	else
	{
		inventory.push_back(currentItem);
		currentSlot++;
	}
}

void InventoryMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (inputManager.isKeyPressed(SDLK_UP))
	{
		if (inventory.size() > currentSlot)
		{
			//update inventory slot
			inventory[currentSlot] = currentItem;
		}
		currentSlot--;
		if (currentSlot < 0)
		{
			currentSlot = inventory.size();
		}
		if (currentSlot < inventory.size())
		{
			currentItem = inventory[currentSlot];
		}
		else
		{
			currentItem = 0;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_DOWN))
	{
		if (inventory.size() > currentSlot)
		{
			//update inventory slot
			inventory[currentSlot] = currentItem;
		}
		currentSlot++;
		if (currentSlot > inventory.size())
		{
			currentSlot = 0;
		}
		if (currentSlot < inventory.size())
		{
			currentItem = inventory[currentSlot];
		}
		else
		{
			currentItem = 0;
		}
	}

	else if (inputManager.isKeyPressed(SDLK_RIGHT))
	{
		currentItem++;
		if (currentItem > items.size() - 1)
		{
			currentItem = 0;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_LEFT))
	{
		currentItem--;
		if (currentItem < 0)
		{
			currentItem = items.size() - 1;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
	else if (inputManager.isKeyPressed(SDLK_BACKSPACE))
	{
		if (inventory.size() > currentSlot)
		{
			inventory.erase(inventory.begin() + currentSlot);
			if (currentSlot < inventory.size())
			{
				currentItem = inventory[currentSlot];
			}
		}
	}
	else if (inputManager.isKeyPressed(SDLK_z))
	{
		CancelOption();
		currentOption = 0;
	}
}

void InventoryMenu::CancelOption()
{
	Menu::CancelOption();
	static_cast<EnemyMenu*>(MenuManager::menuManager.menus.back())->inInventory = false;
}

void InventoryMenu::Draw()
{
	MenuManager::menuManager.menus[MenuManager::menuManager.menus.size() - 2]->Draw();

	if (currentSlot >= 0)
	{
		ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
		ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
		glm::mat4 model = glm::mat4();
		model = glm::translate(model, glm::vec3(144, 80 + 12 * currentSlot, 0.0f));

		model = glm::scale(model, glm::vec3(16, 16, 0.0f));

		ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

		ResourceManager::GetShader("shape").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
	}

	for (int i = 0; i < inventory.size(); i++)
	{
		if (i != currentSlot)
		{
			auto item = items[inventory[i]];
			text->RenderText(item.name, 500, 230 + 30 * i + 1, 1);
			text->RenderTextRight(intToString(item.maxUses), 628, 230 + 30 * i + 1, 1, 14);
		}
	}
	if (currentItem >= 0)
	{
		auto item = items[currentItem];
		text->RenderText(item.name, 500, 230 + 30 * currentSlot + 1, 1, glm::vec3(1.0f, 1.0f, 0.0f));
		text->RenderTextRight(intToString(item.maxUses), 628, 230 + 30 * currentSlot + 1, 1, 14, glm::vec3(1.0f, 1.0f, 0.0f));
	}
}

StatsMenu::StatsMenu(TextRenderer* Text, Camera* camera, int shapeVAO, EnemyMode* mode, Object* object, std::vector<int>& baseStats, bool& editedStats) :
	Menu(Text, camera, shapeVAO), stats(baseStats), edited(editedStats)
{
	numberOfOptions = 9;
}

void StatsMenu::Draw()
{
	MenuManager::menuManager.menus[MenuManager::menuManager.menus.size() - 2]->Draw();
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(48, 80 + 12 * currentOption, 0.0f));

	model = glm::scale(model, glm::vec3(16, 16, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

void StatsMenu::SelectOption()
{
}

void StatsMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (inputManager.isKeyPressed(SDLK_DOWN))
	{
		currentOption++;
		if (currentOption >= numberOfOptions)
		{
			currentOption = 0;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_UP))
	{
		currentOption--;
		if (currentOption < 0)
		{
			currentOption = numberOfOptions - 1;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_LEFT))
	{
		stats[currentOption]--;
		if (stats[currentOption] < 0)
		{
			stats[currentOption] = 0;
		}
		edited = true;
	}
	else if (inputManager.isKeyPressed(SDLK_RIGHT))
	{
		stats[currentOption]++;
		edited = true;
	}
	else if (inputManager.isKeyPressed(SDLK_z))
	{
		CancelOption();
	}
}


ProfsMenu::ProfsMenu(TextRenderer* Text, Camera* camera, int shapeVAO, EnemyMode* mode, Object* object, std::vector<int>& weaponProfs, bool& editedProfs) :
	Menu(Text, camera, shapeVAO), profs(weaponProfs), edited(editedProfs)
{
	numberOfOptions = 10;
}

void ProfsMenu::Draw()
{
	MenuManager::menuManager.menus[MenuManager::menuManager.menus.size() - 2]->Draw();
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(144, 80 + 12 * currentOption, 0.0f));

	model = glm::scale(model, glm::vec3(16, 16, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

void ProfsMenu::SelectOption()
{
}

void ProfsMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (inputManager.isKeyPressed(SDLK_DOWN))
	{
		currentOption++;
		if (currentOption >= numberOfOptions)
		{
			currentOption = 0;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_UP))
	{
		currentOption--;
		if (currentOption < 0)
		{
			currentOption = numberOfOptions - 1;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_LEFT))
	{
		profs[currentOption]--;
		if (profs[currentOption] < 0)
		{
			profs[currentOption] = 0;
		}
		edited = true;
	}
	else if (inputManager.isKeyPressed(SDLK_RIGHT))
	{
		profs[currentOption]++;
		edited = true;
	}
	else if (inputManager.isKeyPressed(SDLK_z))
	{
		CancelOption();
	}
}

SceneMenu::SceneMenu(TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<SceneObjects*>& sceneObjects, std::vector<VisitObjects>& visitObjects) :
	Menu(Text, camera, shapeVAO), sceneObjects(sceneObjects), visitObjects(visitObjects)
{
	numberOfOptions = sceneObjects.size() + 1;
}

void SceneMenu::Draw()
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(16 + 64 *!addingScene, 32 + 12 * currentOption, 0.0f));

	model = glm::scale(model, glm::vec3(16, 16, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
	for (int i = 0; i < sceneObjects.size(); i++)
	{
		text->RenderText("Scene: " + intToString(i + 1), 100, 100 + (i * 32), 1);
		if (!sceneObjects[i]->activation)
		{
			text->RenderText("No Activation!", 200, 100 + (i * 32), 1, glm::vec3(1, 0, 0));
		}
	}
	text->RenderText("New Scene", 100, 100 + (sceneObjects.size() * 32), 1);

	for (int i = 0; i < visitObjects.size(); i++)
	{
		text->RenderText("Visit Location: " + intToString(visitObjects[i].position.x) + ", " + intToString(visitObjects[i].position.y),
			300, 100 + (i * 32), 1);
		int xOffset = 500;
		for (const auto& pair : visitObjects[i].sceneMap)
		{
			text->RenderText(intToString(pair.first), xOffset, 100 + (i * 32), 1);
			xOffset += 32;
		}
	}
	text->RenderText("New Visit", 300, 100 + (visitObjects.size() * 32), 1);
}

void SceneMenu::CheckInput(class InputManager& inputManager, float deltaTime)
{
	Menu::CheckInput(inputManager, deltaTime);
	if (inputManager.isKeyPressed(SDLK_LEFT) || inputManager.isKeyPressed(SDLK_RIGHT))
	{
		addingScene = !addingScene;
		currentOption = 0;
	}
}

void SceneMenu::SelectOption()
{
	if (addingScene)
	{
		if (currentOption == sceneObjects.size())
		{
			SceneObjects* newObject = new SceneObjects;
			sceneObjects.push_back(newObject);
			numberOfOptions++;
		}
		else
		{
			MenuManager::menuManager.OpenActionMenu(*sceneObjects[currentOption]);
		}
	}
	else
	{
		if (currentOption == visitObjects.size())
		{
			VisitObjects visitObject = VisitObjects();
			visitObjects.push_back(visitObject);
			numberOfOptions++;
		}
		else
		{
			//Open visit menu
			MenuManager::menuManager.OpenVisitMenu(visitObjects[currentOption]);
		}
	}
}

VisitMenu::VisitMenu(TextRenderer* Text, Camera* camera, int shapeVAO, VisitObjects& visitObject) :
	Menu(Text, camera, shapeVAO), visitObject(visitObject)
{
	if (visitObject.position.x > 0 && visitObject.position.y > 0)
	{
		cameraPosition = visitObject.position;
	}
	else
	{
		cameraPosition = camera->getPosition();
	}
}

void VisitMenu::Draw()
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getCameraMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(cameraPosition.x, cameraPosition.y, 0.0f));

	model = glm::scale(model, glm::vec3(16, 16, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	if (settingPosition)
	{
		text->RenderText("Click where you want the visit location", 100, 50, 1);
	}
	else
	{
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(visitObject.position.x, visitObject.position.y, 0.0f));

		model = glm::scale(model, glm::vec3(16, 16, 0.0f));

		ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.5f, 0.5f));

		ResourceManager::GetShader("shape").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		glm::vec2 drawPosition = glm::vec2(visitObject.position) + glm::vec2(2, 4);
		drawPosition = camera->worldToRealScreen(drawPosition, SCREEN_WIDTH, SCREEN_HEIGHT);
		text->RenderText("start", drawPosition.x, drawPosition.y, 1);
		text->RenderText("Current: " + intToString(unitID) + " " + intToString(sceneID + 1), 100, 100, 1);
		text->RenderText("Use the enter key to confirm new scene pairs. Space bar to confirm.\n First is the unitID(-1 for all), second is the scene number", 100, 50, 1);
	}
	int yOffset = 150;
	for (const auto& pair : visitObject.sceneMap) 
	{
		text->RenderText(intToString(pair.first) + " " + intToString(pair.second + 1), 100, yOffset, 1);
		yOffset += 30;
	}
}

void VisitMenu::SelectOption()
{
	if (settingPosition)
	{
		visitObject.position = cameraPosition;
		settingPosition = false;
	}
	else
	{
		visitObject.sceneMap[unitID] = sceneID;
	}
}

void VisitMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	glm::ivec2 move(0);
	if (inputManager.isKeyPressed(SDLK_UP))
	{
		move.y = -1;
	}
	else if (inputManager.isKeyPressed(SDLK_DOWN))
	{
		move.y = 1;
	}
	if (inputManager.isKeyPressed(SDLK_RIGHT))
	{
		move.x = 1;
	}
	else if (inputManager.isKeyPressed(SDLK_LEFT))
	{
		move.x = -1;
	}
	//Want to be able to display the unit name in the future
	if (inputManager.isKeyPressed(SDLK_w))
	{
		unitID++;
	}
	else if (inputManager.isKeyPressed(SDLK_s))
	{
		unitID--;
		if (unitID < -1)
		{
			unitID = -1;
		}
	}
	if (inputManager.isKeyPressed(SDLK_e))
	{
		sceneID++;

	}
	else if (inputManager.isKeyPressed(SDLK_d))
	{
		sceneID--;
		if (sceneID < 0)
		{
			sceneID = 0;
		}
	}
	if (!settingPosition)
	{
		if (inputManager.isKeyPressed(SDLK_DELETE))
		{
			visitObject.sceneMap.erase(unitID);
		}
	}
	if (inputManager.isKeyPressed(SDLK_SPACE))
	{
		Menu::CancelOption();
	}
	else if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
	else if (inputManager.isKeyPressed(SDLK_z))
	{
		CancelOption();
	}
	else
	{
		cameraPosition += move * TileManager::TILE_SIZE;
		camera->setPosition(cameraPosition);
	}
}

void VisitMenu::CancelOption()
{
	if (settingPosition)
	{
		Menu::CancelOption();
	}
	else
	{
		settingPosition = true;
	}
}

SceneActionMenu::SceneActionMenu(TextRenderer* Text, Camera* camera, int shapeVAO, SceneObjects& sceneObject) : 
	Menu(Text, camera, shapeVAO), sceneObject(sceneObject), sceneActions(sceneObject.actions)
{
	numberOfOptions = sceneActions.size() + 1;
	actionNames.resize(11);
	actionNames[CAMERA_ACTION] = "Camera Action";
	actionNames[NEW_UNIT_ACTION] = "New Unit Action";
	actionNames[MOVE_UNIT_ACTION] = "Move Unit Action";
	actionNames[DIALOGUE_ACTION] = "Dialogue Action";
	actionNames[ITEM_ACTION] = "Item Action";
	actionNames[NEW_SCENE_UNIT_ACTION] = "New Scene Unit Action";
	actionNames[SCENE_UNIT_MOVE_ACTION] = "Scene Unit Move Action";
	actionNames[SCENE_UNIT_REMOVE_ACTION] = "Scene Unit Remove Action";
	actionNames[START_MUSIC] = "Start Music";
	actionNames[STOP_MUSIC] = "Stop Music";
	actionNames[SHOW_MAP_TITLE] = "Show Title";
}

void SceneActionMenu::Draw()
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	if (activationMode)
	{
		model = glm::translate(model, glm::vec3(176, 32, 0.0f));
	}
	else
	{
		model = glm::translate(model, glm::vec3(16, 32 + yIndicator, 0.0f));
	}

	model = glm::scale(model, glm::vec3(16, 16, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	if (swapAction >= 0)
	{
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(16, 32 + 12 * swapAction, 0.0f));

		model = glm::scale(model, glm::vec3(16, 16, 0.0f));

		ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.5f, 5.0f));

		ResourceManager::GetShader("shape").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
	}

	for (int i = 0; i < sceneActions.size(); i++)
	{
		auto currentAction = sceneActions[i];
		if (currentAction->type == CAMERA_ACTION)
		{
			auto action = static_cast<CameraMove*>(currentAction);
			text->RenderText("Camera move: " + intToString(action->position.x) + " " + 
				intToString(action->position.y), 100, 100 + (i * 32) - yOffset, 1);
		}
		else if (currentAction->type == NEW_UNIT_ACTION)
		{
			auto action = static_cast<AddUnit*>(currentAction);
			text->RenderText("New Unit, ID: " + intToString(action->unitID) + ", Start: " + intToString(action->start.x) + 
				" " + intToString(action->start.y) + ", End: " + intToString(action->end.x) + " " + 
				intToString(action->end.y), 100, 100 + (i * 32) - yOffset, 1);
		}
		else if (currentAction->type == MOVE_UNIT_ACTION)
		{
			auto action = static_cast<UnitMove*>(currentAction);

			text->RenderText("Move Unit, ID: " + intToString(action->unitID) + ", End: " + intToString(action->end.x) + " " +
				intToString(action->end.y), 100, 100 + (i * 32) - yOffset, 1);
		}
		else if (currentAction->type == DIALOGUE_ACTION)
		{
			auto action = static_cast<DialogueAction*>(currentAction);
			text->RenderText("Dialogue, ID: " + intToString(action->ID), 100, 100 + (i * 32) - yOffset, 1);
		}
		else if (currentAction->type == ITEM_ACTION)
		{
			auto action = static_cast<ItemAction*>(currentAction);
			text->RenderText("Item, ID: " + intToString(action->ID), 100, 100 + (i * 32) - yOffset, 1);
		}
		else if (currentAction->type == NEW_SCENE_UNIT_ACTION)
		{
			auto action = static_cast<AddSceneUnit*>(currentAction);
			text->RenderText("New scene Unit :" + intToString(action->unitID) + " Delay: " + floatToString(action->nextActionDelay) + " Move Delay: " + floatToString(action->nextMoveDelay), 100, 100 + (i * 32) - yOffset, 1);
		}
		else if (currentAction->type == SCENE_UNIT_MOVE_ACTION)
		{
			auto action = static_cast<SceneUnitMove*>(currentAction);
			text->RenderText("Scene Unit move :" + intToString(action->unitID) + " Delay: " + floatToString(action->nextActionDelay) + " Move Speed: " + floatToString(action->moveSpeed), 100, 100 + (i * 32) - yOffset, 1);
		}
		else if (currentAction->type == SCENE_UNIT_REMOVE_ACTION)
		{
			auto action = static_cast<SceneUnitRemove*>(currentAction);
			text->RenderText("Unit to remove: " + intToString(action->unitID) + " Delay: " + floatToString(action->nextActionDelay), 100, 100 + (i * 32) - yOffset, 1);
		}
		else if (currentAction->type == START_MUSIC)
		{
			auto action = static_cast<StartMusic*>(currentAction);

			text->RenderText("Start Music: " + intToString(action->ID), 100, 100 + (i * 32) - yOffset, 1);
		}
		else if (currentAction->type == STOP_MUSIC)
		{
			auto action = static_cast<StopMusic*>(currentAction);
			text->RenderText("Stop Music, delay " + floatToString(action->nextActionDelay), 100, 100 + (i * 32) - yOffset, 1);
		}
		else if (currentAction->type == SHOW_MAP_TITLE)
		{
			text->RenderText("Show Title, delay " + floatToString(currentAction->nextActionDelay), 100, 100 + (i * 32) - yOffset, 1);
		}
	}

	text->RenderText("New Action", 100, 100 + (sceneActions.size() * 32) - yOffset, 1);
	std::string activationModeString = "Activation Mode: ";
	if (sceneObject.activation)
	{
		if (sceneObject.activation->type == 1)
		{
			auto activation = static_cast<EnemyTurnEnd*>(sceneObject.activation);
			activationModeString += "Enemy turn end: " + intToString(activation->round);
		}
		else if (sceneObject.activation->type == 0)
		{
			auto activation = static_cast<TalkActivation*>(sceneObject.activation);
			activationModeString += "Talk: Talker: " + intToString(activation->talker) + " Listener: " + intToString(activation->listener);
		}
		else if (sceneObject.activation->type == 2)
		{
			activationModeString += "Visit";
		}
		else if (sceneObject.activation->type == 3)
		{
			activationModeString += "Intro";
		}
		else if (sceneObject.activation->type == 4)
		{
			activationModeString += "Ending";
		}
	}

	model = glm::mat4();
	model = glm::translate(model, glm::vec3(0, 0, 0.0f));
	model = glm::scale(model, glm::vec3(256, 32, 0.0f));

	ResourceManager::GetShader("shape").Use().SetVector3f("shapeColor", glm::vec3(0.0f, 0.0f, 1.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	model = glm::mat4();
	model = glm::translate(model, glm::vec3(0, 200, 0.0f));
	model = glm::scale(model, glm::vec3(256, 32, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.0f, 1.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	text->RenderText("Action Menu", 100, 50, 1);
	text->RenderText(actionNames[selectedAction], 300, 50, 1);

	text->RenderText(activationModeString, 500, 50, 1);
	std::string repeatText = "Repeat? : ";
	if (sceneObject.repeat)
	{
		repeatText += "Yes";
	}
	else
	{
		repeatText += "No";
	}
	text->RenderText(repeatText, 100, 550, 1);

}

void SceneActionMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	int yDirection = 0;
	if (inputManager.isKeyPressed(SDLK_UP))
	{
		yDirection = -1;
		firstMove = true;
	}
	else if (inputManager.isKeyPressed(SDLK_DOWN))
	{
		yDirection = 1;
		firstMove = true;
	}
	if (yDirection > 0)
	{
		currentOption++;
		if (currentOption > numberOfOptions)
		{
			currentOption = numberOfOptions;
		}
		else
		{
			yIndicator += indicatorIncrement;
			if (yIndicator > 84)
			{
				yIndicator = 84;
				down = true;
				goal = yOffset + 64;
				yOffset += 32;
			}
		}
	}
	else if(yDirection < 0)
	{
		currentOption--;
		if (currentOption < 0)
		{
			currentOption = 0;
		}
		else
		{
			yIndicator -= indicatorIncrement;
			if (yIndicator < 0)
			{
				yIndicator = 0;
				up = true;
				goal = yOffset - 64;
				yOffset -= 32;
			}
		}
	}
	if (inputManager.isKeyDown(SDLK_UP))
	{
		yDirection = -1;
	}
	if (inputManager.isKeyDown(SDLK_DOWN))
	{
		yDirection = 1;
	}
	if (yDirection != 0)
	{
		movementDelay += deltaTime;
		float delayTime = normalDelay;
		if (firstMove)
		{
			delayTime = firstDelay;
		}
		if (movementDelay >= delayTime)
		{
			if (yDirection > 0)
			{
				currentOption++;
				if (currentOption > numberOfOptions)
				{
					currentOption = numberOfOptions;
				}
				else
				{
					yIndicator += indicatorIncrement;
					if (yIndicator > 84)
					{
						yIndicator = 84;
						down = true;
						goal = yOffset + 64;
						yOffset += 32;
					}
				}
			}
			else
			{
				currentOption--;
				if (currentOption < 0)
				{
					currentOption = 0;
				}
				else
				{
					yIndicator -= indicatorIncrement;
					if (yIndicator < 0)
					{
						yIndicator = 0;
						up = true;
						goal = yOffset - 64;
						yOffset -= 32;
					}
				}
			}
			movementDelay = 0.0f;
			firstMove = false;
		}
	}
	else
	{
		movementDelay = 0.0f;
		firstMove = true;
	}

	if (inputManager.isKeyPressed(SDLK_RIGHT))
	{
		selectedAction++;
		
		if (selectedAction > SHOW_MAP_TITLE)
		{
			selectedAction = 0;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_LEFT))
	{
		selectedAction--;
		if (selectedAction < 0)
		{
			selectedAction = SHOW_MAP_TITLE;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_DELETE))
	{
		if (activationMode)
		{
			//delete activation mode?
			delete sceneObject.activation;
			sceneObject.activation = nullptr;
		}
		else if (currentOption < sceneActions.size())
		{
			auto* it = sceneActions[currentOption];
			sceneActions.erase(sceneActions.begin() + currentOption);
			delete it;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_TAB))
	{
		if (swapAction < 0)
		{
			activationMode = !activationMode;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_r))
	{
		sceneObject.repeat = !sceneObject.repeat;
	}

	if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
	else if (inputManager.isKeyPressed(SDLK_i))
	{
		if (swapAction >= 0)
		{
			int direction = 1;
			if (swapAction < currentOption)
			{
				direction = 0;
			}
			sceneActions.insert(sceneActions.begin() + currentOption, sceneActions[swapAction]);
			sceneActions.erase(sceneActions.begin() + swapAction + direction);
			swapAction = -1;
		}
	}
	if (inputManager.isKeyPressed(SDLK_z))
	{
		CancelOption();
	}
	if (inputManager.isKeyPressed(SDLK_SPACE))
	{
		//Edit action
		auto currentAction = sceneActions[currentOption];
		if (currentAction->type == NEW_SCENE_UNIT_ACTION)
		{
			auto newMenu = new NewSceneUnitActionMenu(text, camera, shapeVAO, sceneActions, static_cast<AddSceneUnit*>(currentAction));
			MenuManager::menuManager.menus.push_back(newMenu);
		}
		else if (currentAction->type == SCENE_UNIT_MOVE_ACTION)
		{

		}
		else if (currentAction->type == SCENE_UNIT_REMOVE_ACTION)
		{

		}
	}
}

void SceneActionMenu::SelectOption()
{
	if (activationMode)
	{
		if (sceneObject.activation == nullptr)
		{
			MenuManager::menuManager.OpenActivationMenu(sceneObject);
		}
	}
	else if (currentOption == sceneActions.size())
	{
		MenuManager::menuManager.SelectOptionMenu(selectedAction, sceneActions);
	}
	else
	{
		if (swapAction >= 0)
		{
			auto* temp = sceneActions[currentOption];
			sceneActions[currentOption] = sceneActions[swapAction];
			sceneActions[swapAction] = temp;
			swapAction = -1;
		}
		else
		{
			swapAction = currentOption;
		}
	}
}

void SceneActionMenu::CancelOption()
{
	if (swapAction >= 0)
	{
		swapAction = -1;
	}
	else
	{
		Menu::CancelOption();
	}
}

SceneActivationMenu::SceneActivationMenu(TextRenderer* Text, Camera* camera, int shapeVAO, SceneObjects& sceneObject) : Menu(Text, camera, shapeVAO), sceneObject(sceneObject)
{
	numberOfOptions = 5;

}

void SceneActivationMenu::Draw()
{
	int xPosition = 100;
	int yPosition = 100;
	text->RenderText("Activation Menu", 100, 50, 1);
	if (currentOption == TALK)
	{
		text->RenderText("Talk", xPosition, yPosition, 1);
	}
	else if (currentOption == ENEMY_TURN_END)
	{
		text->RenderText("Enemy Turn End", xPosition, yPosition, 1);
	}
	else if (currentOption == VISIT)
	{
		text->RenderText("Visit", xPosition, yPosition, 1);
	}
	else if(currentOption == INTRO)
	{
		text->RenderText("Intro", xPosition, yPosition, 1);
	}
	else
	{
		text->RenderText("Ending", xPosition, yPosition, 1);
	}
}

void SceneActivationMenu::SelectOption()
{
	if (currentOption == TALK)
	{
		MenuManager::menuManager.OpenTalkMenu(sceneObject);
	}
	else if (currentOption == ENEMY_TURN_END)
	{
		sceneObject.activation = new EnemyTurnEnd(ENEMY_TURN_END, 2);
		CancelOption();
	}
	else if (currentOption == VISIT)
	{
		sceneObject.activation = new Activation(VISIT);
		CancelOption();
	}
	else if (currentOption == INTRO)
	{
		sceneObject.activation = new Activation(INTRO);
		CancelOption();
	}
	else
	{
		sceneObject.activation = new Activation(ENDING);
		CancelOption();
	}
}

TalkActivationMenu::TalkActivationMenu(TextRenderer* Text, Camera* camera, int shapeVAO, SceneObjects& sceneObject) : Menu(Text, camera, shapeVAO), sceneObject(sceneObject)
{
	numberOfOptions = 2;

}

void TalkActivationMenu::Draw()
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(32 + 32 * !setTalker, 56, 0.0f));

	model = glm::scale(model, glm::vec3(16, 16, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	text->RenderText("Talk Activation Menu", 100, 50, 1);

	text->RenderText("Talker: " + intToString(talker), 100, 200, 1);
	text->RenderText("Listener: " + intToString(listener), 200, 200, 1);
}

void TalkActivationMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (inputManager.isKeyPressed(SDLK_RIGHT) || inputManager.isKeyPressed(SDLK_LEFT))
	{
		setTalker = !setTalker;
	}
	if (inputManager.isKeyPressed(SDLK_UP))
	{
		if (setTalker)
		{
			talker++;
		}
		else
		{
			listener++;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_DOWN))
	{
		if (setTalker)
		{
			talker--;
		}
		else
		{
			listener--;
		}
	}
	if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
	else if (inputManager.isKeyPressed(SDLK_z))
	{
		CancelOption();
	}
}


void TalkActivationMenu::SelectOption()
{
	sceneObject.activation = new TalkActivation(0, talker, listener);

	Menu::CancelOption();
	Menu::CancelOption();
}


CameraActionMenu::CameraActionMenu(TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<SceneAction*>& sceneActions) : Menu(Text, camera, shapeVAO), sceneActions(sceneActions)
{
	cameraPosition = camera->getPosition();
}

void CameraActionMenu::Draw()
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getCameraMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(cameraPosition.x, cameraPosition.y, 0.0f));

	model = glm::scale(model, glm::vec3(16, 16, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	text->RenderText("Press enter to save camera position", 100, 50, 1);
	text->RenderText("Position: " + intToString(cameraPosition.x) + " " + intToString(cameraPosition.y), 600, 50, 1);
}

void CameraActionMenu::SelectOption()
{
	CameraMove* move = new CameraMove(0, cameraPosition);
	sceneActions.push_back(move);
	MenuManager::menuManager.menus[MenuManager::menuManager.menus.size() - 2]->numberOfOptions++;
	CancelOption();
}

void CameraActionMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	glm::ivec2 move(0);
	if (inputManager.isKeyPressed(SDLK_UP))
	{
		move.y = -1;
	}
	else if (inputManager.isKeyPressed(SDLK_DOWN))
	{
		move.y = 1;
	}
	if (inputManager.isKeyPressed(SDLK_RIGHT))
	{
		move.x = 1;
	}
	else if (inputManager.isKeyPressed(SDLK_LEFT))
	{
		move.x = -1;
	}
	if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
	else if (inputManager.isKeyPressed(SDLK_z))
	{
		CancelOption();
	}
	else
	{
		cameraPosition += move * TileManager::TILE_SIZE;
		camera->setPosition(cameraPosition);
	}
}

NewUnitActionMenu::NewUnitActionMenu(TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<SceneAction*>& sceneActions) : Menu(Text, camera, shapeVAO), sceneActions(sceneActions)
{
	unitID = 0;
}

void NewUnitActionMenu::Draw()
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getCameraMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(cameraPosition.x, cameraPosition.y, 0.0f));

	model = glm::scale(model, glm::vec3(16, 16, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	if (setStart)
	{
		text->RenderText("Click where you want the unit to spawn", 100, 50, 1);
	}
	else
	{
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(startPosition.x, startPosition.y, 0.0f));

		model = glm::scale(model, glm::vec3(16, 16, 0.0f));

		ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.0f, 0.5f, 0.5f));

		ResourceManager::GetShader("shape").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		glm::vec2 drawPosition = glm::vec2(startPosition) + glm::vec2(2, 4);
		drawPosition = camera->worldToRealScreen(drawPosition, SCREEN_WIDTH, SCREEN_HEIGHT);
		text->RenderText("start", drawPosition.x, drawPosition.y, 1);

		text->RenderText("Click where you want the unit to move to", 100, 50, 1);
	}
	text->RenderText("Position: " + intToString(cameraPosition.x) + " " + intToString(cameraPosition.y), 100, 100, 1);
	text->RenderText("Start position: " + intToString(startPosition.x) + " " + intToString(startPosition.y), 600, 100, 1);
	text->RenderText("End position: " + intToString(endPosition.x) + " " + intToString(endPosition.y), 600, 150, 1);
	text->RenderText("Unit: " + intToString(unitID), 600, 200, 1);
}

void NewUnitActionMenu::SelectOption()
{
	if (setStart)
	{
		startPosition = cameraPosition;
		setStart = false;
	}
	else
	{
		endPosition = cameraPosition;
		finished = true;
		AddUnit* move = new AddUnit(1, unitID, startPosition, endPosition);
		sceneActions.push_back(move);
		MenuManager::menuManager.menus[MenuManager::menuManager.menus.size() - 2]->numberOfOptions++;
		CancelOption();
	}
}

//I want to ultimately handle this input with the mouse, just reusing the code from the camera action for now.
void NewUnitActionMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	glm::ivec2 move(0);
	if (inputManager.isKeyPressed(SDLK_UP))
	{
		move.y = -1;
	}
	else if (inputManager.isKeyPressed(SDLK_DOWN))
	{
		move.y = 1;
	}
	if (inputManager.isKeyPressed(SDLK_RIGHT))
	{
		move.x = 1;
	}
	else if (inputManager.isKeyPressed(SDLK_LEFT))
	{
		move.x = -1;
	}
	//Want to be able to display the unit name in the future
	if (inputManager.isKeyPressed(SDLK_w))
	{
		unitID++;
		if (unitID > 7)
		{
			unitID = 0;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_s))
	{
		unitID--;
		if (unitID < 0)
		{
			unitID = 7;
		}
	}
	if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
	else if (inputManager.isKeyPressed(SDLK_z))
	{
		CancelOption();
	}
	else
	{
		cameraPosition += move * TileManager::TILE_SIZE;
		camera->setPosition(cameraPosition);
	}
}

void NewUnitActionMenu::CancelOption()
{
	if (setStart || finished)
	{
		Menu::CancelOption();
	}
	else
	{
		setStart = true;
	}
}

MoveUnitActionMenu::MoveUnitActionMenu(TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<SceneAction*>& sceneActions) : Menu(Text, camera, shapeVAO), sceneActions(sceneActions)
{
	unitID = 0;
}

void MoveUnitActionMenu::Draw()
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getCameraMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(cameraPosition.x, cameraPosition.y, 0.0f));

	model = glm::scale(model, glm::vec3(16, 16, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	text->RenderText("Position: " + intToString(cameraPosition.x) + " " + intToString(cameraPosition.y), 100, 100, 1);
	text->RenderText("End position: " + intToString(endPosition.x) + " " + intToString(endPosition.y), 600, 150, 1);
	text->RenderText("Unit: " + intToString(unitID), 600, 200, 1);
}

void MoveUnitActionMenu::SelectOption()
{
	endPosition = cameraPosition;
	UnitMove* move = new UnitMove(2, unitID, endPosition);
	sceneActions.push_back(move);
	MenuManager::menuManager.menus[MenuManager::menuManager.menus.size() - 2]->numberOfOptions++;
	CancelOption();
}

//I want to ultimately handle this input with the mouse, just reusing the code from the camera action for now.
void MoveUnitActionMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	glm::ivec2 move(0);
	if (inputManager.isKeyPressed(SDLK_UP))
	{
		move.y = -1;
	}
	else if (inputManager.isKeyPressed(SDLK_DOWN))
	{
		move.y = 1;
	}
	if (inputManager.isKeyPressed(SDLK_RIGHT))
	{
		move.x = 1;
	}
	else if (inputManager.isKeyPressed(SDLK_LEFT))
	{
		move.x = -1;
	}
	//Want to be able to display the unit name in the future
	//Not really sure how to handle this, since I need to account for any potential unit on any team
	//A lot of this is rough manually setting, which is annoying
	if (inputManager.isKeyPressed(SDLK_w))
	{
		unitID++;
		if (unitID > 99)
		{
			unitID = 0;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_s))
	{
		unitID--;
		if (unitID < 0)
		{
			unitID = 99;
		}
	}
	if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
	else if (inputManager.isKeyPressed(SDLK_z))
	{
		CancelOption();
	}
	else
	{
		cameraPosition += move * TileManager::TILE_SIZE;
		camera->setPosition(cameraPosition);
	}
}

DialogueActionMenu::DialogueActionMenu(TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<SceneAction*>& sceneActions) : Menu(Text, camera, shapeVAO), sceneActions(sceneActions)
{
	dialogueID = 0;
}

void DialogueActionMenu::Draw()
{
	//Would like this to display the dialogue in question
	text->RenderText("Dialogue: " + intToString(dialogueID), 600, 200, 1);
}

void DialogueActionMenu::SelectOption()
{
	DialogueAction* move = new DialogueAction(3, dialogueID);
	sceneActions.push_back(move);
	MenuManager::menuManager.menus[MenuManager::menuManager.menus.size() - 2]->numberOfOptions++;
	CancelOption();
}

//I want to ultimately handle this input with the mouse, just reusing the code from the camera action for now.
void DialogueActionMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (inputManager.isKeyPressed(SDLK_UP))
	{
		dialogueID++;
	}
	else if (inputManager.isKeyPressed(SDLK_DOWN))
	{
		dialogueID--;
	}
	if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
	else if (inputManager.isKeyPressed(SDLK_z))
	{
		CancelOption();
	}
}

ItemActionMenu::ItemActionMenu(TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<SceneAction*>& sceneActions) : Menu(Text, camera, shapeVAO), sceneActions(sceneActions)
{
	itemID;
	io::CSVReader<8, io::trim_chars<' '>, io::no_quote_escape<':'>> in("../items.csv");
	in.read_header(io::ignore_extra_column, "ID", "name", "maxUses", "useID", "isWeapon", "canDrop", "description", "value");
	std::string name;
	int ID;
	int maxUses;
	int useID;
	int isWeapon;
	int canDrop;
	int value;
	std::string description;
	while (in.read_row(ID, name, maxUses, useID, isWeapon, canDrop, description, value))
	{
		//csv reader reads in new lines wrong for whatever reason, this fixes it.
		size_t found = description.find("\\n");
		while (found != std::string::npos)
		{
			description.replace(found, 2, "\n");
			found = description.find("\\n", found + 1);
		}
		items.push_back({ ID, name, maxUses, maxUses, useID, value, bool(isWeapon), bool(canDrop), description });
	}
	numberOfOptions = items.size();
}

void ItemActionMenu::Draw()
{
	text->RenderText("Item: " + items[currentOption].name, 600, 200, 1);
}

void ItemActionMenu::SelectOption()
{
	ItemAction* move = new ItemAction(4, currentOption);
	sceneActions.push_back(move);
	MenuManager::menuManager.menus[MenuManager::menuManager.menus.size() - 2]->numberOfOptions++;
	CancelOption();
}

VendorMenu::VendorMenu(TextRenderer* Text, Camera* camera, int shapeVAO, Vendor* vendor) : Menu(Text, camera, shapeVAO), vendor(vendor)
{
	vendor->items.reserve(8);

	io::CSVReader<8, io::trim_chars<' '>, io::no_quote_escape<':'>> in("../items.csv");
	in.read_header(io::ignore_extra_column, "ID", "name", "maxUses", "useID", "isWeapon", "canDrop", "description", "value");
	std::string name;
	int ID;
	int maxUses;
	int useID;
	int isWeapon;
	int canDrop;
	int value;
	std::string description;
	while (in.read_row(ID, name, maxUses, useID, isWeapon, canDrop, description, value))
	{
		//csv reader reads in new lines wrong for whatever reason, this fixes it.
		size_t found = description.find("\\n");
		while (found != std::string::npos)
		{
			description.replace(found, 2, "\n");
			found = description.find("\\n", found + 1);
		}
		items.push_back({ ID, name, maxUses, maxUses, useID, value, bool(isWeapon), bool(canDrop), description });
	}

	currentSlot = 0;
	if (currentSlot < vendor->items.size())
	{
		currentItem = vendor->items[currentSlot];
	}
}

void VendorMenu::Draw()
{
	if (currentSlot >= 0)
	{
		ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
		ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
		glm::mat4 model = glm::mat4();
		model = glm::translate(model, glm::vec3(144, 80 + 12 * currentSlot, 0.0f));

		model = glm::scale(model, glm::vec3(16, 16, 0.0f));

		ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

		ResourceManager::GetShader("shape").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
	}

	for (int i = 0; i < vendor->items.size(); i++)
	{
		if (i != currentSlot)
		{
			auto item = items[vendor->items[i]];
			text->RenderText(item.name, 500, 230 + 30 * i + 1, 1);
			text->RenderTextRight(intToString(item.maxUses), 628, 230 + 30 * i + 1, 1, 14);
			text->RenderTextRight(intToString(item.value), 700, 230 + 30 * i + 1, 1, 14);
		}
	}
	if (currentItem >= 0)
	{
		auto item = items[currentItem];
		text->RenderText(item.name, 500, 230 + 30 * currentSlot + 1, 1, glm::vec3(1.0f, 1.0f, 0.0f));
		text->RenderTextRight(intToString(item.maxUses), 628, 230 + 30 * currentSlot + 1, 1, 14, glm::vec3(1.0f, 1.0f, 0.0f));
		text->RenderTextRight(intToString(item.value), 700, 230 + 30 * currentSlot + 1, 1, 14, glm::vec3(1.0f, 1.0f, 0.0f));
	}

	text->RenderText("Vendor inventory", 600, 100, 1);

}

void VendorMenu::SelectOption()
{
	if (vendor->items.size() > currentSlot)
	{
		//update inventory slot
		vendor->items[currentSlot] = currentItem;
	}
	else
	{
		vendor->items.push_back(currentItem);
		currentSlot++;
	}
}

void VendorMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (inputManager.isKeyPressed(SDLK_UP))
	{
		if (vendor->items.size() > currentSlot)
		{
			//update inventory slot
			vendor->items[currentSlot] = currentItem;
		}
		currentSlot--;
		if (currentSlot < 0)
		{
			currentSlot = vendor->items.size();
		}
		if (currentSlot < vendor->items.size())
		{
			currentItem = vendor->items[currentSlot];
		}
		else
		{
			currentItem = 0;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_DOWN))
	{
		if (vendor->items.size() > currentSlot)
		{
			//update inventory slot
			vendor->items[currentSlot] = currentItem;
		}
		currentSlot++;
		if (currentSlot > vendor->items.size())
		{
			currentSlot = 0;
		}
		if (currentSlot < vendor->items.size())
		{
			currentItem = vendor->items[currentSlot];
		}
		else
		{
			currentItem = 0;
		}
	}

	else if (inputManager.isKeyPressed(SDLK_RIGHT))
	{
		currentItem++;
		if (currentItem > items.size() - 1)
		{
			currentItem = 0;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_LEFT))
	{
		currentItem--;
		if (currentItem < 0)
		{
			currentItem = items.size() - 1;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
	else if (inputManager.isKeyPressed(SDLK_BACKSPACE))
	{
		if (vendor->items.size() > currentSlot)
		{
			vendor->items.erase(vendor->items.begin() + currentSlot);
			if (currentSlot < vendor->items.size())
			{
				currentItem = vendor->items[currentSlot];
			}
		}
	}
	else if (inputManager.isKeyPressed(SDLK_z))
	{
		CancelOption();
	}
}

RemoveSceneUnitActionMenu::RemoveSceneUnitActionMenu(TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<SceneAction*>& sceneActions)
	: Menu(Text, camera, shapeVAO), sceneActions(sceneActions)
{
	unitID = 0;
}

void RemoveSceneUnitActionMenu::Draw()
{
	text->RenderText("Unit to remove: " + intToString(unitID), 100, 200, 1);
	text->RenderText("Delay: " + floatToString(delay), 300, 200, 1);
	text->RenderText("Increment: " + floatToString(delayIncrement), 300, 250, 1);
	text->RenderText("Note for the time being, this unit ID is based on the order they were added to the scene.", 100, 100, 1);
}

void RemoveSceneUnitActionMenu::SelectOption()
{
	SceneUnitRemove* remove = new SceneUnitRemove(SCENE_UNIT_REMOVE_ACTION, unitID, delay);
	sceneActions.push_back(remove);
	MenuManager::menuManager.menus[MenuManager::menuManager.menus.size() - 2]->numberOfOptions++;
	CancelOption();
}

void RemoveSceneUnitActionMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (inputManager.isKeyPressed(SDLK_UP))
	{
		unitID++;
	}
	else if (inputManager.isKeyPressed(SDLK_DOWN))
	{
		unitID--;
		if (unitID < 0)
		{
			unitID = 0;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_LEFT))
	{
		delay -= delayIncrement;
	}
	else if (inputManager.isKeyPressed(SDLK_RIGHT))
	{
		delay += delayIncrement;
	}
	else if (inputManager.isKeyPressed(SDLK_d))
	{
		if (delayIncrement == 0.1f)
		{
			delayIncrement = 1;
		}
		else if (delayIncrement == 1)
		{
			delayIncrement = 0.01f;
		}
		else
		{
			delayIncrement = 0.1f;
		}
	}
	if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
	else if (inputManager.isKeyPressed(SDLK_z))
	{
		CancelOption();
	}
}

NewSceneUnitActionMenu::NewSceneUnitActionMenu(TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<SceneAction*>& sceneActions, AddSceneUnit* existingAction)
	: Menu(Text, camera, shapeVAO), sceneActions(sceneActions), existingAction(existingAction)
{
	if (existingAction)
	{
		unitID = existingAction->unitID;
		team = existingAction->team;
		nextDelay = existingAction->nextActionDelay;
		moveDelay = existingAction->nextMoveDelay;
		path = existingAction->path;
	}
	else
	{
		unitID = 0;
	}
	cameraPosition = camera->getPosition();

}

void NewSceneUnitActionMenu::Draw()
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getCameraMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(cameraPosition.x, cameraPosition.y, 0.0f));

	model = glm::scale(model, glm::vec3(16, 16, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	for (int i = 0; i < path.size(); i++)
	{
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(path[i].x, path[i].y, 0.0f));

		model = glm::scale(model, glm::vec3(16, 16, 0.0f));

		ResourceManager::GetShader("shape").Use().SetVector3f("shapeColor", glm::vec3(0.0f, 0.5f, 0.5f));

		ResourceManager::GetShader("shape").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		glm::vec2 drawPosition = glm::vec2(path[i]) + glm::vec2(2, 4);
		drawPosition = camera->worldToRealScreen(drawPosition, SCREEN_WIDTH, SCREEN_HEIGHT);
		text->RenderText(intToString(i), drawPosition.x, drawPosition.y, 1);
	}
	if (!delayMode)
	{
		text->RenderText("Position: " + intToString(cameraPosition.x) + " " + intToString(cameraPosition.y), 100, 100, 1);
	}
	else
	{
		text->RenderText("Delay mode, set the delay", 100, 100, 1);
	}
	text->RenderText("Unit: " + intToString(unitID), 600, 200, 1);
	text->RenderText("Team: " + intToString(team), 600, 250, 1);
	text->RenderText("Path Increment: " + intToString(pathIncrement), 600, 300, 1);
	text->RenderText("Delay: " + floatToString(nextDelay), 600, 350, 1);
	text->RenderText("Move Delay: " + floatToString(moveDelay), 600, 400, 1);
	text->RenderText("Increment: " + floatToString(delayIncrement), 600, 450, 1);
}

void NewSceneUnitActionMenu::SelectOption()
{
	path.push_back(cameraPosition);
}

void NewSceneUnitActionMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	glm::ivec2 move(0);
	if (inputManager.isKeyPressed(SDLK_TAB))
	{
		delayMode = !delayMode;
	}
	if (!delayMode)
	{
		if (inputManager.isKeyPressed(SDLK_UP))
		{
			move.y = -1;
		}
		else if (inputManager.isKeyPressed(SDLK_DOWN))
		{
			move.y = 1;
		}
		if (inputManager.isKeyPressed(SDLK_RIGHT))
		{
			move.x = 1;
		}
		else if (inputManager.isKeyPressed(SDLK_LEFT))
		{
			move.x = -1;
		}
		if (inputManager.isKeyPressed(SDLK_p))
		{
			if (pathIncrement == 16)
			{
				pathIncrement = 8;
			}
			else
			{
				pathIncrement = 16;
			}
		}
	}
	else
	{
		if (inputManager.isKeyPressed(SDLK_LEFT))
		{
			nextDelay -= delayIncrement;
		}
		else if (inputManager.isKeyPressed(SDLK_RIGHT))
		{
			nextDelay += delayIncrement;
		}
		else if (inputManager.isKeyPressed(SDLK_UP))
		{
			moveDelay += delayIncrement;
			if (moveDelay < 0)
			{
				moveDelay = 0;
			}
		}
		else if (inputManager.isKeyPressed(SDLK_DOWN))
		{
			moveDelay -= delayIncrement;
			if (moveDelay < 0)
			{
				moveDelay = -1;
			}
		}
		else if (inputManager.isKeyPressed(SDLK_d))
		{
			if (delayIncrement == 0.1f)
			{
				delayIncrement = 1;
			}
			else if (delayIncrement == 1)
			{
				delayIncrement = 0.01f;
			}
			else
			{
				delayIncrement = 0.1f;
			}
		}
	}
	//Want to be able to display the unit name in the future
	if (inputManager.isKeyPressed(SDLK_w))
	{
		unitID++;
		if (unitID > 12)
		{
			unitID = 0;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_s))
	{
		unitID--;
		if (unitID < 0)
		{
			unitID = 12;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_t))
	{
		team++;
		if (team > 1)
		{
			team = 0;
		}
	}

	if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
	else if (inputManager.isKeyPressed(SDLK_z))
	{
		CancelOption();
	}
	else if (inputManager.isKeyPressed(SDLK_SPACE))
	{
		if (existingAction)
		{
			existingAction->unitID = unitID;
			existingAction->team = team;
			existingAction->nextActionDelay = nextDelay;
			existingAction->nextMoveDelay = moveDelay;
			existingAction->path = path;
		}
		else
		{
			AddSceneUnit* action = new AddSceneUnit(NEW_SCENE_UNIT_ACTION, unitID, team, path, nextDelay, moveDelay);
			sceneActions.push_back(action);
			MenuManager::menuManager.menus[MenuManager::menuManager.menus.size() - 2]->numberOfOptions++;
		}
		CancelOption();
	}
	else
	{
		cameraPosition += move * pathIncrement;
		camera->setPosition(cameraPosition);
	}
}

SceneUnitMoveActionMenu::SceneUnitMoveActionMenu(TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<SceneAction*>& sceneActions, SceneUnitMove* existingAction)
	: Menu(Text, camera, shapeVAO), sceneActions(sceneActions), existingAction(existingAction)
{
	if (existingAction)
	{
		unitID = existingAction->unitID;
		nextDelay = existingAction->nextActionDelay;
		moveSpeed = existingAction->moveSpeed;
		path = existingAction->path;
	}
	else
	{
		unitID = 0;
	}
	cameraPosition = camera->getPosition();

}

void SceneUnitMoveActionMenu::Draw()
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getCameraMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(cameraPosition.x, cameraPosition.y, 0.0f));

	model = glm::scale(model, glm::vec3(16, 16, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	for (int i = 0; i < path.size(); i++)
	{
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(path[i].x, path[i].y, 0.0f));

		model = glm::scale(model, glm::vec3(16, 16, 0.0f));

		ResourceManager::GetShader("shape").Use().SetVector3f("shapeColor", glm::vec3(0.0f, 0.5f, 0.5f));

		ResourceManager::GetShader("shape").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		glm::vec2 drawPosition = glm::vec2(path[i]) + glm::vec2(2, 4);
		drawPosition = camera->worldToRealScreen(drawPosition, SCREEN_WIDTH, SCREEN_HEIGHT);
		text->RenderText(intToString(i), drawPosition.x, drawPosition.y, 1);
	}
	if (!delayMode)
	{
		text->RenderText("Position: " + intToString(cameraPosition.x) + " " + intToString(cameraPosition.y), 100, 100, 1);
	}
	else
	{
		text->RenderText("Delay mode, set the delay", 100, 100, 1);
	}
	text->RenderText("Unit: " + intToString(unitID), 600, 200, 1);
	text->RenderText("facing: " + intToString(facing), 600, 250, 1);
	text->RenderText("Path Increment: " + intToString(pathIncrement), 600, 300, 1);
	text->RenderText("Delay: " + floatToString(nextDelay), 600, 350, 1);
	text->RenderText("Move Speed: " + floatToString(moveSpeed), 600, 400, 1);
	text->RenderText("Increment: " + floatToString(delayIncrement), 600, 450, 1);
}

void SceneUnitMoveActionMenu::SelectOption()
{
	path.push_back(cameraPosition);
}

void SceneUnitMoveActionMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	glm::ivec2 move(0);
	if (inputManager.isKeyPressed(SDLK_TAB))
	{
		delayMode = !delayMode;
	}
	if (!delayMode)
	{
		if (inputManager.isKeyPressed(SDLK_UP))
		{
			move.y = -1;
		}
		else if (inputManager.isKeyPressed(SDLK_DOWN))
		{
			move.y = 1;
		}
		if (inputManager.isKeyPressed(SDLK_RIGHT))
		{
			move.x = 1;
		}
		else if (inputManager.isKeyPressed(SDLK_LEFT))
		{
			move.x = -1;
		}
		if (inputManager.isKeyPressed(SDLK_p))
		{
			if (pathIncrement == 16)
			{
				pathIncrement = 8;
			}
			else
			{
				pathIncrement = 16;
			}
		}
	}
	else
	{
		if (inputManager.isKeyPressed(SDLK_LEFT))
		{
			nextDelay -= delayIncrement;
		}
		else if (inputManager.isKeyPressed(SDLK_RIGHT))
		{
			nextDelay += delayIncrement;
		}
		else if (inputManager.isKeyPressed(SDLK_UP))
		{
			moveSpeed += delayIncrement;
			if (moveSpeed < 0)
			{
				moveSpeed = 0;
			}
		}
		else if (inputManager.isKeyPressed(SDLK_DOWN))
		{
			moveSpeed -= delayIncrement;
			if (moveSpeed < 0)
			{
				moveSpeed = -1;
			}
		}
		else if (inputManager.isKeyPressed(SDLK_d))
		{
			if (delayIncrement == 0.1f)
			{
				delayIncrement = 1;
			}
			else if (delayIncrement == 1)
			{
				delayIncrement = 0.01f;
			}
			else
			{
				delayIncrement = 0.1f;
			}
		}
	}
	//Want to be able to display the unit name in the future
	if (inputManager.isKeyPressed(SDLK_w))
	{
		unitID++;
		if (unitID > 22)
		{
			unitID = 0;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_s))
	{
		unitID--;
		if (unitID < 0)
		{
			unitID = 22;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_f))
	{
		facing++;
		if (facing > 3)
		{
			facing = -1;
		}
	}

	if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
	else if (inputManager.isKeyPressed(SDLK_z))
	{
		CancelOption();
	}
	else if (inputManager.isKeyPressed(SDLK_SPACE))
	{
		if (existingAction)
		{
			existingAction->unitID = unitID;
			existingAction->nextActionDelay = nextDelay;
			existingAction->moveSpeed = moveSpeed;
			existingAction->path = path;
		}
		else
		{
			SceneUnitMove* action = new SceneUnitMove(SCENE_UNIT_MOVE_ACTION, unitID, path, nextDelay, moveSpeed, facing);
			sceneActions.push_back(action);
			MenuManager::menuManager.menus[MenuManager::menuManager.menus.size() - 2]->numberOfOptions++;
		}
		CancelOption();
	}
	else
	{
		cameraPosition += move * pathIncrement;
		camera->setPosition(cameraPosition);
	}
}

StartMusicActionMenu::StartMusicActionMenu(TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<SceneAction*>& sceneActions)
	: Menu(Text, camera, shapeVAO), sceneActions(sceneActions)
{
}

void StartMusicActionMenu::Draw()
{
	text->RenderText("Music: " + intToString(musicID), 100, 100, 1);
}

void StartMusicActionMenu::SelectOption()
{
	StartMusic* move = new StartMusic(START_MUSIC, musicID);
	sceneActions.push_back(move);
	MenuManager::menuManager.menus[MenuManager::menuManager.menus.size() - 2]->numberOfOptions++;
	CancelOption();
}

void StartMusicActionMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (inputManager.isKeyPressed(SDLK_UP))
	{
		musicID++;
	}
	else if (inputManager.isKeyPressed(SDLK_DOWN))
	{
		musicID--;
		if (musicID < 0)
		{
			musicID = 0;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_z))
	{
		CancelOption();
	}
	else if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
}

RequireUnitsMenu::RequireUnitsMenu(TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<std::pair<int, int>>& requiredUnits) 
	: Menu(Text, camera, shapeVAO), requiredUnits(requiredUnits)
{
	numberOfOptions = requiredUnits.size() + 1;
	currentPair = std::pair<int, int>(0, 0);
}

void RequireUnitsMenu::Draw()
{
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera->getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(64, 37 + 16 * currentOption, 0.0f));

	model = glm::scale(model, glm::vec3(16, 16, 0.0f));

	ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.5f, 0.5f, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	for (int i = 0; i < requiredUnits.size(); i++)
	{
		if (i != currentOption)
		{
			text->RenderText("Required Unit: " + intToString(requiredUnits[i].first) + " GameOverID: " + intToString(requiredUnits[i].second), 200, 100 + 42 * i, 1);
		}
	}
	text->RenderText("Required Unit: " + intToString(currentPair.first) + " GameOverID: " + intToString(currentPair.second), 200, 100 + 42 * currentOption, 1, glm::vec3(1.0f, 1.0f, 0.0f));

}

void RequireUnitsMenu::SelectOption()
{
	if (currentOption < requiredUnits.size())
	{
		requiredUnits[currentOption] = currentPair;
		currentOption++;
	}
	else
	{
		requiredUnits.push_back(currentPair);
		numberOfOptions++;
		currentOption++;
	}
}

void RequireUnitsMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	Menu::CheckInput(inputManager, deltaTime);

	if (inputManager.isKeyPressed(SDLK_w))
	{
		currentPair.first++;
	}
	else if (inputManager.isKeyPressed(SDLK_s))
	{
		currentPair.first--;
		if (currentPair.first < 0)
		{
			currentPair.first = 0;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_e))
	{
		currentPair.second++;
	}
	else if (inputManager.isKeyPressed(SDLK_d))
	{
		currentPair.second--;
		if (currentPair.second < 0)
		{
			currentPair.second = 0;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_DELETE))
	{
		if (currentOption < requiredUnits.size())
		{
			requiredUnits.erase(requiredUnits.begin() + currentOption);
			numberOfOptions--;
			if (currentOption == requiredUnits.size())
			{
				currentOption--;
			}
		}
		else
		{
			currentPair = std::pair<int, int>(0, 0);
		}
	}
}

StopMusicActionMenu::StopMusicActionMenu(TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<SceneAction*>& sceneActions)
	: Menu(Text, camera, shapeVAO), sceneActions(sceneActions)
{
}

void StopMusicActionMenu::Draw()
{
	text->RenderText("Delay: " + floatToString(nextDelay), 600, 350, 1);
	text->RenderText("Increment: " + floatToString(delayIncrement), 600, 450, 1);
}

void StopMusicActionMenu::SelectOption()
{
	StopMusic* action = new StopMusic(STOP_MUSIC, nextDelay);
	sceneActions.push_back(action);
	MenuManager::menuManager.menus[MenuManager::menuManager.menus.size() - 2]->numberOfOptions++;
	CancelOption();
}

void StopMusicActionMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (inputManager.isKeyPressed(SDLK_LEFT))
	{
		nextDelay -= delayIncrement;
	}
	else if (inputManager.isKeyPressed(SDLK_RIGHT))
	{
		nextDelay += delayIncrement;
	}
	else if (inputManager.isKeyPressed(SDLK_d))
	{
		if (delayIncrement == 0.1f)
		{
			delayIncrement = 1;
		}
		else if (delayIncrement == 1)
		{
			delayIncrement = 0.01f;
		}
		else
		{
			delayIncrement = 0.1f;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_z))
	{
		CancelOption();
	}
	else if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
}

ShowTitleActionMenu::ShowTitleActionMenu(TextRenderer* Text, Camera* camera, int shapeVAO, std::vector<SceneAction*>& sceneActions)
	: Menu(Text, camera, shapeVAO), sceneActions(sceneActions)
{
}

void ShowTitleActionMenu::Draw()
{
	text->RenderText("Delay: " + floatToString(nextDelay), 600, 350, 1);
	text->RenderText("Increment: " + floatToString(delayIncrement), 600, 450, 1);
}

void ShowTitleActionMenu::SelectOption()
{
	ShowTitle* action = new ShowTitle(SHOW_MAP_TITLE, nextDelay);
	sceneActions.push_back(action);
	MenuManager::menuManager.menus[MenuManager::menuManager.menus.size() - 2]->numberOfOptions++;
	CancelOption();
}

void ShowTitleActionMenu::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (inputManager.isKeyPressed(SDLK_LEFT))
	{
		nextDelay -= delayIncrement;
	}
	else if (inputManager.isKeyPressed(SDLK_RIGHT))
	{
		nextDelay += delayIncrement;
	}
	else if (inputManager.isKeyPressed(SDLK_d))
	{
		if (delayIncrement == 0.1f)
		{
			delayIncrement = 1;
		}
		else if (delayIncrement == 1)
		{
			delayIncrement = 0.01f;
		}
		else
		{
			delayIncrement = 0.1f;
		}
	}
	else if (inputManager.isKeyPressed(SDLK_z))
	{
		CancelOption();
	}
	else if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		SelectOption();
	}
}
