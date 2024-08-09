#pragma once
#include <string>
#include <glm.hpp>
#include <vector>
class TextRenderer;

struct TextObject
{
	int index;
	int nextIndex = 55;
	int charsPerLine = 55;

	int portraitID = -1;
	int frame = 0;

	glm::vec2 position;
	glm::vec2 displayedPosition;
	glm::vec2 portraitPosition;
	glm::vec4 boxPosition;
	glm::vec4 boxDisplayPosition;
	//The full text to be displayed
	std::string text;
	//The currently displayed text
	std::string displayedText;

	float fadeValue = 0.0f;

	bool mirrorPortrait = false;
	bool showPortrait = false;
	bool fadeIn = false;
	bool fadeOut = false;
	bool boxIn = true; //idk man

	TextObject();

	void Draw(TextRenderer* textRenderer, class SpriteRenderer* Renderer, class Camera* camera, bool canShow);
};

struct SpeakerText
{
	class Sprite* speaker = nullptr;
	int location;
	std::string text;
	int portraitID;
	int BG = 0;
};

enum TextObjectState
{
	WAITING_ON_INPUT,
	REMOVING_TEXT,
	LAYER_1_FADE_IN,
	PORTRAIT_FADE_IN,
	READING_TEXT,
	PORTRAIT_FADE_OUT,
	FADE_GAME_OUT,
	FADE_BG_IN,
	FADE_BG_OUT
};

struct TextObjectManager
{
	bool active = false;
	bool talkActivated = false;
	bool showAnyway = false;
	bool showBG = false;
	bool fadeIn = false;
	bool finishing = false;

	int currentLine = 0;
	int focusedObject = 0;
	int nextOption = 0;

	float displayTimer = 0.0f;
	float delay;
	float normalDelay = 0.05f;
	float fastDelay = 0.0025f;

	int frame = 0;
	int frameDirection = 1;
	int BG = 0;
	float animTime = 0.0f;
	float BGAlpha = 0.0f;
	float blackAlpha = 0.0f;
	float layer1Alpha = 0.0f;
	float layer1MaxAlpha = 0.35f;

	float t;

	std::vector<SpeakerText> textLines;
	std::vector<TextObject> textObjects;

	TextObjectState state;

	TextObjectManager();
	void init(int line = 0);
	void Update(float deltaTime, class InputManager& inputManager, bool finished = false);
	void ReadText(InputManager& inputManager, float deltaTime);
	void GoToNextLine();
	void Draw(TextRenderer* textRenderer, class SpriteRenderer* Renderer, class Camera* camera);
	void DrawFade(Camera* camera, int shapeVAO);
	void DrawLayer1Fade(Camera* camera, int shapeVAO);
	bool ShowText();
};