/*
Code modified from https://github.com/Barnold1953/GraphicsTutorials/tree/master/Bengine
*/

#include "InputManager.h"
#include <iostream>

InputManager::InputManager()
{
    //ctor
}

InputManager::~InputManager()
{
    //dtor
}

void InputManager::update(float delta)
{
	if (releasedKey >= 0)
	{
		doubleTimer += delta;
		if (doubleTimer >= doubleTime)
		{
			doubleTimer = 0.0f;
		//	lastKey = -1;
			releasedKey = -1;
		}
	}
	heldDelay = false;
	if (lastKey >= 0)
	{
		if (wasKeyDown(lastKey))
		{
			holdTimer += delta;
			if (holdTimer >= holdTime)
			{
				holdTimer = 0.0f;
				heldDelay = true;
			}
		}
	}

    //loop through keymap using a foreach loop and copy it to previous keymap
    for(auto& it: keyMap)
    {
        previousKeyMap[it.first] = it.second;
    }
}

void InputManager::pressKey(unsigned int keyID)
{
	keyMap[keyID] = true;

	if (lastKey < 0)
	{
		lastKey = keyID;
		doubleTimer = 0.0f;
		if (releasedKey != lastKey)
		{
			releasedKey = -1;
		}
	}
}

void InputManager::releaseKey(unsigned int keyID)
{
	keyMap[keyID] = false;

	if (lastKey >= 0 && !isKeyDown(lastKey))
	{
		lastKey = -1;
		holdTimer = 0.0f;
	}

	if (releasedKey < 0)
	{
		releasedKey = keyID;
		doubleTimer = 0.0f;
	}
	else if (lastKey >= 0)
	{
		releasedKey = -1;
	}
}

bool InputManager::isKeyDown(unsigned int keyID)
{
    auto it = keyMap.find(keyID);
    if(it != keyMap.end())
    {
        return it->second;
    }
    return false;
}

bool InputManager::isKeyPressed(unsigned int keyID)
{
    if(isKeyDown(keyID) && !wasKeyDown(keyID))
    {
        return true;
    }
    return false;
}

bool InputManager::isKeyReleased(unsigned int keyID)
{
	if (wasKeyDown(keyID) && !isKeyDown(keyID))
	{
		return true;
	}
	return false;
}

bool InputManager::isKeyUp(unsigned int keyID)
{
    return !isKeyDown(keyID);
}


bool InputManager::KeyDownDelay(unsigned int keyID, float normalDelay, float initialDelay)
{
	if(isKeyPressed(keyID))
	{
		holdTime = initialDelay;
		return true;
	}
	else if(isKeyDown(keyID) && heldDelay)
	{
		holdTime = normalDelay;
		return true;
	}
	return false;
}


//This function a little special
//Only returns unique double presses, which is to say, if a key is held down, it cannot register a double press of a second key
//Some edits to this class will allow the above behaviour, meaning anytime any passed key is pressed twice, this will return true
//For this specific implemntation however, I only want unique ones.
bool InputManager::keyDoublePress(unsigned int keyID)
{
	if (isKeyPressed(keyID))
	{
		if (lastKey == keyID && releasedKey == keyID)
		{
			doubleTimer = 0.0f;
			return true;
		}
		/*else
		{
			lastKey = keyID;
		}*/
	}
	return false;
}

bool InputManager::wasKeyDown(unsigned int keyID)
{
    auto it = previousKeyMap.find(keyID);
    if(it != previousKeyMap.end())
    {
        return it->second;
    }
    return false;
}

