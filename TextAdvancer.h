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
	glm::vec2 extraPosition;
	glm::vec4 boxDisplayPosition;
	glm::vec4* extraUV;
	//The full text to be displayed
	std::string text;
	//The currently displayed text
	std::string displayedText;

	float fadeValue = 0.0f;

	bool mirrorPortrait = false;
	bool showPortrait = false;
	bool showBox = false;
	bool fadeIn = false;
	bool fadeOut = false;
	bool boxIn = true; //idk man
	bool active = false;
	bool showAnyway = false;

	TextObject();

	void Draw(TextRenderer* textRenderer, class SpriteRenderer* Renderer, class Camera* camera, bool canShow, bool canShowBox, glm::vec3 color);
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
	FADE_BG_OUT,
	FADE_BOX_OUT,
	BATTLE_BOX_FADE_IN,
	BATTLE_BOX_FADE_OUT
};

struct TextObjectManager
{
	bool active = false;
	bool talkActivated = false;
	bool showBG = false;
	bool fadeIn = false;
	bool finishing = false;
	bool midTalkPortraitChange = false;

	bool boxFadeOut = false;
	bool showBoxAnyway = false;
	//Hate this, need this to manage the enemy battle messages
	bool continueBattle = false;
	//Same as above, this sucks but need it to handle playing music for the map battles
	bool playMusic = false;

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
	int battleBoxFrame = 0;
	float animTime = 0.0f;
	float BGAlpha = 0.0f;
	float blackAlpha = 0.0f;
	float layer1Alpha = 0.0f;
	float layer1MaxAlpha = 0.525f;
	float battleBoxAlpha = 0.0f;

	float battleBoxTimer = 0.0f;

	float t = 0.0f;

	std::vector<SpeakerText> textLines;
	std::vector<TextObject> textObjects;

	glm::vec4 boxStarts[2];
	std::vector<glm::vec4> extraUVs;
	std::vector<glm::vec4> battleBoxIndicator;
	glm::vec4 battleTextUV;

	TextObjectState state = WAITING_ON_INPUT;

	TextObjectManager();
	void setUVs();
	void init(int line = 0);
	void Update(float deltaTime, class InputManager& inputManager, bool finished = false);
	void BattleTextClose();
	void ReadText(InputManager& inputManager, float deltaTime);
	void GoToNextLine();
	void Draw(TextRenderer* textRenderer, class SpriteRenderer* Renderer, class Camera* camera);
	void DrawBox(int i, SpriteRenderer* Renderer, Camera* camera);
	void DrawFade(Camera* camera, int shapeVAO);
	void DrawOverBattleBox(Camera* camera, int shapeVAO);
	bool ShowText();

	void EndingScene();
};