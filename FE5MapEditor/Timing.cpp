/*
Code modified from https://github.com/Barnold1953/GraphicsTutorials/tree/master/Bengine
*/

#include "Timing.h"
#include <SDL.h>

FPSLimiter::FPSLimiter()
{

}

void FPSLimiter::init(float target)
{
    setMaxFPS(target);
}

void FPSLimiter::beginFrame()
{
    startTicks = SDL_GetTicks();
}

float FPSLimiter::end()
{
    calculateFPS();
    float frameTicks = SDL_GetTicks() - startTicks;
    //Limit FPS
    if(1000.0f / maxFPS > frameTicks)
    {
        SDL_Delay(1000.0f / maxFPS - frameTicks);
    }
    return fps;
}
void FPSLimiter::setMaxFPS(float target)
{
    maxFPS = target;
}
void FPSLimiter::calculateFPS()
{
    static const int NUMBER_OF_SAMPLES = 10;
    static float frameTimes[NUMBER_OF_SAMPLES];

    static int currentFrame = 0;

    static float previousTicks = SDL_GetTicks();
    float currentTicks;
    currentTicks = SDL_GetTicks();

    frameTime = currentTicks - previousTicks;

    frameTimes[currentFrame %  NUMBER_OF_SAMPLES] = frameTime;

    previousTicks = currentTicks;

    int count;

    currentFrame ++;
    if(currentFrame < NUMBER_OF_SAMPLES)
    {
        count = currentFrame;
    }
    else
    {
        count = NUMBER_OF_SAMPLES;
    }

    float frameTimeAverage = 0;
    for(int i = 0; i < count; i ++)
    {
        frameTimeAverage += frameTimes[i];
    }
    frameTimeAverage /= count;

    if(frameTimeAverage >= 0)
    {
        fps = 1000.0f / frameTimeAverage;
    }
    else
    {
        fps = 60;
    }
}


