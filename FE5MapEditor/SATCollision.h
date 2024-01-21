#pragma once
#include <vector>
#include <cmath>

#include <limits>
#include <stdio.h>
#include <algorithm>

#include "Sprite.h"
using namespace std;
class SATCollision
{
public:
    SATCollision();
    ~SATCollision();
};
struct Point
{
    float x;
    float y;
};

class SimpleShape
{
private:


public:
    //The position of the shape, also it's center
    Point shapePosition;
    float halfWidth;
    float halfHeight;
    int numberOfVerticies;
    vector<Point> theVerticies;
    SimpleShape()
    {

    }
    SimpleShape(Point position, int w, int h, int verticies)
    {
        init(position, w, h, verticies);
    }

    //inits an axis alligned box
    void init(int w, int h)
    {
        //shapePosition = position;
        halfWidth = w;
        halfHeight = h;
        numberOfVerticies = 4;

        theVerticies.resize(numberOfVerticies);
    }

    //Creates a polygon with a variable number of vertices
    //Honestly not entirely sure what this is doing at this point, but it's not in use so whatever
    void init(Point position, int w, int h, int verticies)
    {
        shapePosition = position;
        halfHeight = h;
        halfWidth = w;
        numberOfVerticies = verticies;

        theVerticies.resize(numberOfVerticies);
        float pi = 3.14;
        for (int i = 0; i < numberOfVerticies; i++)
        {
            theVerticies[i] = { round(shapePosition.x + halfWidth * cos(2 * pi * i / numberOfVerticies)), round(shapePosition.y + halfHeight * sin(2 * pi * i / numberOfVerticies)) };
            //  printf("%f %f\n", theVerticies[i].x, theVerticies[i].y);
            //printf("%f", asin(2) * 180/3.14159265);
            //  float testing = numeric_limits<float>::infinity();
            //            printf("%f", testing);
        }
    }

    Point getPosition()
    {
        return shapePosition;
    }

    int getNumberOfVerticies()
    {
        return numberOfVerticies;
    }

    void MoveShape(float xMove, float yMove)
    {
        shapePosition.x += xMove;
        shapePosition.y += yMove;

        for (int i = 0; i < numberOfVerticies; i++)
        {
            theVerticies[i].x += xMove;
            theVerticies[i].y += yMove;
        }
    }
    //Only works with axis aligned boxes
    //Really not sure how I would do this with other types of polygons but it doesn't matter right now
    void SetShapePosition(float x, float y)
    {
        shapePosition.x = x;
        shapePosition.y = y;

        theVerticies[0] = { x, y };
        theVerticies[1] = { x + halfWidth, y };
        theVerticies[2] = { x + halfWidth, y + halfHeight };
        theVerticies[3] = { x, y + halfHeight };
    }

    void rotateShape(float angle)
    {
        for (int i = 0; i < numberOfVerticies; i++)
        {
            //translate
            float x = theVerticies[i].x - shapePosition.x;
            float y = theVerticies[i].y - shapePosition.y;
            //rotate
            theVerticies[i].x =  x * cos(angle) - y * sin(angle);
            theVerticies[i].y =  x * sin(angle) + y * cos(angle);
            //translate back
            theVerticies[i].x = (theVerticies[i].x + shapePosition.x);
            theVerticies[i].y = (theVerticies[i].y + shapePosition.y);
        }
    }
};

struct Line
{
    Point a;
    Point b;
};

struct Interval
{
    float min;
    float max;
};

Point findNormal(Line segment);
float dotProduct(Point a, Point b);

Point normalize(Point aPoint);

float getSlope(Line aLine);

bool satCollision(SimpleShape& shapeA, SimpleShape& shapeB, Point& p);
bool satCollision(SimpleShape& shapeA, SimpleShape& shapeB, Sprite& sprite);
bool satCollision(SimpleShape& shapeA, SimpleShape& shapeB, float& x, float& y);

SimpleShape newSquare( glm::vec2 position, float w, float h);
SimpleShape testTri( glm::vec2 position, float w, float h);
