#pragma once
#include <string>
#include <glm.hpp>
#include <vector>
class TextRenderer;

struct TextObject
{
	int index;
	int nextIndex;
	int charsPerLine;

	glm::vec2 position;
	glm::vec2 displayedPosition;
	//The full text to be displayed
	std::string text;
	//The currently displayed text
	std::string displayedText;

	TextObject();

	void Draw(TextRenderer* textRenderer);
};

struct SpeakerText
{
	class Unit* speaker = nullptr;
	int location;
	std::string text;
};

struct TextObjectManager
{
	bool waitingOnInput = false;
	bool removingText = false;
	bool active = false;
	bool talkActivated = false;
	bool showAnyway = false;

	int currentLine = 0;
	int focusedObject = 0;
	int nextOption = 0;

	float displayTimer = 0.0f;
	float delay;
	float normalDelay = 0.05f;
	float fastDelay = 0.0025f;
	std::vector<SpeakerText> textLines;
	std::vector<TextObject> textObjects;

	TextObjectManager();
	void init(int line = 0);
	void Update(float deltaTime, class InputManager& inputManager, class Cursor& cursor);
	void Draw(TextRenderer* textRenderer);
	bool ShowText();
};