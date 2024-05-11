#pragma once
#include <string>
#include <cstdio>
#include <iostream>
#include <sstream>

// The Width of the screen
const int SCREEN_WIDTH = 800;
// The height of the screen
const int SCREEN_HEIGHT = 600;

//I don't like this I'm not keeping it

static float heldSpeed = 7.5f;

static std::string intToString(int i)
{
	std::stringstream s;
	s << i;
	return s.str();
}
template<typename... Args>
struct Observer
{
	virtual ~Observer() {}
	virtual void onNotify(Args...) = 0;
};

template<typename... Args>
struct Subject
{
	std::vector<Observer<Args...>*> observers;

	void addObserver(Observer<Args...>* observer)
	{
		observers.push_back(observer);
	}
	void removeObserver(Observer<Args...>* observer)
	{
		auto it = std::find(observers.begin(), observers.end(), observer);
		if (it != observers.end())
		{
			delete* it;
			*it = observers.back();
			observers.pop_back();
		}
	}
	void notify(Args... args)
	{
		for (auto observer : observers)
		{
			observer->onNotify(args...);
		}
	}
};