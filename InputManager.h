#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include <unordered_map>
#include <glm.hpp>

/*
Code modified from https://github.com/Barnold1953/GraphicsTutorials/tree/master/Bengine
*/

class InputManager
{
public:
    InputManager();
    ~InputManager();

    void update(float delta);

    void pressKey(unsigned int keyID);
    void releaseKey(unsigned int keyID);

    //True if key is held down
    bool isKeyDown(unsigned int keyID);

    //True on the frame the key is pressed
    bool isKeyPressed(unsigned int keyID);

    //True on the frame the key is released
	bool isKeyReleased(unsigned int keyID);

	//True if key is not down
	bool isKeyUp(unsigned int keyID);

	bool keyDoublePress(unsigned int keyID);

    bool KeyDownDelay(unsigned int keyID, float normalDelay = 0.05f, float initialDelay = 0.15f);

protected:
private:
    std::unordered_map<unsigned int, bool> keyMap;
    std::unordered_map<unsigned int, bool> previousKeyMap;

	int lastKey = -1;
	int releasedKey = -1;

    bool heldDelay = false;

	float doubleTimer = 0.0f;
	float doubleTime = 0.15f;
    float holdTimer = 0.0f;
    float holdTime = 0.05f;

    bool wasKeyDown(unsigned int keyID);
};

#endif // INPUTMANAGER_H
