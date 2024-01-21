/*
Code modified from https://github.com/Barnold1953/GraphicsTutorials/tree/master/Bengine
*/

#ifndef TIMING_H_INCLUDED
#define TIMING_H_INCLUDED

class FPSLimiter
{
public:
    FPSLimiter();
    void init(float target);
    void beginFrame();

    //Returns the current fps
    float end();

    void setMaxFPS(float target);
private:

    float maxFPS;
    float fps;
    float frameTime;

    unsigned int startTicks;

    void calculateFPS();
};


#endif // TIMING_H_INCLUDED
