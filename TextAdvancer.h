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

	int portraitID = -1;
	int frame = 0;

	glm::vec2 position;
	glm::vec2 displayedPosition;
	glm::vec2 portraitPosition;
	//The full text to be displayed
	std::string text;
	//The currently displayed text
	std::string displayedText;

	bool mirrorPortrait = false;

	TextObject();

	void Draw(TextRenderer* textRenderer, class SpriteRenderer* Renderer, class Camera* camera);
};

struct SpeakerText
{
	class Sprite* speaker = nullptr;
	int location;
	std::string text;
	int portraitID;
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

	int frame = 0;
	int frameDirection = 1;
	float animTime = 0.0f;

	std::vector<SpeakerText> textLines;
	std::vector<TextObject> textObjects;

	TextObjectManager();
	void init(int line = 0);
	void Update(float deltaTime, class InputManager& inputManager);
	void Draw(TextRenderer* textRenderer, class SpriteRenderer* Renderer, class Camera* camera);
	bool ShowText();
};