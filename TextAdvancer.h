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

	float fadeValue = 0.0f;

	bool mirrorPortrait = false;
	bool showPortrait = false;
	bool fadeIn = false;
	bool fadeOut = false;

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

enum TextObjectState
{
	WAITING_ON_INPUT,
	REMOVING_TEXT,
	PORTRAIT_FADE_IN,
	READING_TEXT,
	PORTRAIT_FADE_OUT
};

struct TextObjectManager
{
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

	TextObjectState state;

	TextObjectManager();
	void init(int line = 0);
	void Update(float deltaTime, class InputManager& inputManager);
	void ReadText(InputManager& inputManager, float deltaTime);
	void GoToNextLine();
	void Draw(TextRenderer* textRenderer, class SpriteRenderer* Renderer, class Camera* camera);
	bool ShowText();
};