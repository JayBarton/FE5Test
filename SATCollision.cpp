#include "SATCollision.h"
#include <iostream>


SATCollision::SATCollision()
{
}


SATCollision::~SATCollision()
{
}

Point findNormal(Line segment)
{
    float dx = segment.b.x - segment.a.x;
    float dy = segment.b.y - segment.a.y;

    Point normal = { -dy, dx };

    return normal;
}

float dotProduct(Point a, Point b)
{
    return (a.x * b.x) + (a.y * b.y);
}

Point normalize(Point aPoint)
{
    float length = sqrt((aPoint.x * aPoint.x) + (aPoint.y * aPoint.y));
    Point normalizedPoint = { aPoint.x / length, aPoint.y / length };
    return normalizedPoint;
}

float getSlope(Line aLine)
{
    float deltaX = aLine.b.x - aLine.a.x;
    float deltaY = aLine.b.y - aLine.a.y;
    float slope = 0;
    //If delta x is equal to zero than it's slope is undefined. I just set it to infinity for simplicity.
    //If delta y is zero then the slope will be zero too, and there is no need to calculate it.
    if (deltaX != 0)
    {
        if (deltaY != 0)
        {
            slope = deltaY / deltaX;
        }
    }
    else
    {
        slope = numeric_limits<float>::infinity();
    }
    return slope;
}


bool satCollision(SimpleShape & shapeA, SimpleShape & shapeB, Point& p)
{
    //Get points(verticies)/lines of shapes
    int numberOfVerticiesA = shapeA.getNumberOfVerticies();
    int numberOfVerticiesB = shapeB.getNumberOfVerticies();

    int numberOfVerticies = numberOfVerticiesA + numberOfVerticiesB;

    //Point verticies[numberOfVerticies];
    std::vector<Point> verticies;
    verticies.resize(numberOfVerticies);

    for (int i = 0; i < numberOfVerticiesA; i++)
    {
        Point aPoint = shapeA.theVerticies[i];
        verticies[i] = aPoint;
    }

    for (int i = 0; i < numberOfVerticiesB; i++)
    {
        Point aPoint = shapeB.theVerticies[i];
        verticies[i + numberOfVerticiesA] = aPoint;
    }
    //Work with regular polygon now for now.

    //Maybe this whole part will be split off into a function or two
    //It is redundant to check parallel lines, as their normals will also be parallel
    //So, to insure that lines will only be checked once, the lines and their slopes are stored in vectors
    //The floats are then compared. If the slope is unique, then the line must be too, so it and the slope are added to the vectors

    //The number of edges for the first shape is initialized to 1, as it and it's slope are both set outside of the loop
    int numberOfEdgesA = 1;
    int numberOfEdgesB = 0;

    vector<Line> edgeVector;
    vector<float> slopes;

    Line firstLine = { shapeA.theVerticies[0],shapeA.theVerticies[1] };

    slopes.push_back(getSlope(firstLine));
    edgeVector.push_back(firstLine);

    for (int i = 1; i < numberOfVerticiesA; i++)
    {
        Point nextPoint = shapeA.theVerticies[0];
        if (i != numberOfVerticiesA - 1)
        {
            nextPoint = shapeA.theVerticies[i + 1];
        }

        Line aLine = { shapeA.theVerticies[i],nextPoint };

        float slope = getSlope(aLine);
        float currentSlope = 0;
        bool newLine = true;
        while (currentSlope < slopes.size() && newLine)
        {
            if (slope == slopes[currentSlope])
            {
                newLine = false;
            }
            currentSlope++;
        }
        if (newLine)
        {
            slopes.push_back(slope);
            edgeVector.push_back(aLine);
            numberOfEdgesA++;
        }
    }

    for (int i = 0; i < numberOfVerticiesB; i++)
    {
        Point nextPoint = shapeB.theVerticies[0];
        if (i != numberOfVerticiesB - 1)
        {
            nextPoint = shapeB.theVerticies[i + 1];
        }

        Line aLine = { shapeB.theVerticies[i],nextPoint };

        float slope = getSlope(aLine);
        int currentSlope = 0;
        bool newLine = true;

        while (currentSlope < slopes.size() && newLine)
        {
            if (slope == slopes[currentSlope])
            {
                newLine = false;
            }
            currentSlope++;
        }
        if (newLine)
        {
            slopes.push_back(slope);
            edgeVector.push_back(aLine);
            numberOfEdgesB++;
        }
    }

    int numberOfEdges = numberOfEdgesA + numberOfEdgesB;

    Point smallest;
    float overlap = 100000;

    //loop through the lines and calculate the normal
    //project every vertex of both boxes onto the normal. perform the dot product with the (normalized) normal and store the minimum and maximum.
    //if the projections do not overlap then it means there is no intersection and we break from the loop
    for (int currentEdge = 0; currentEdge < numberOfEdges; currentEdge++)
    {
        Point aNormal = findNormal(edgeVector[currentEdge]);
        aNormal = normalize(aNormal);

        float aMin = dotProduct(shapeA.theVerticies[0], aNormal);
        float bMin = dotProduct(shapeB.theVerticies[0], aNormal);
        float aMax = aMin;
        float bMax = bMin;

        Interval pro1;
        Interval pro2;

        for (int currentVertex = 0; currentVertex < numberOfVerticiesA; currentVertex++)
        {
            float proj = dotProduct(verticies[currentVertex], aNormal);

            if (proj < aMin)
            {
                aMin = proj;
            }
            else if (proj > aMax)
            {
                aMax = proj;
            }
        }
        pro1 = { aMin, aMax };

        for (int currentVertex = 0; currentVertex < numberOfVerticiesB; currentVertex++)
        {
            float proj = dotProduct(verticies[currentVertex + numberOfVerticiesA], aNormal);
            if (proj < bMin)
            {
                bMin = proj;
            }
            else if (proj > bMax)
            {
                bMax = proj;
            }
        }
        pro2 = { bMin, bMax };

        if (aMax < bMin || bMax < aMin)
        {
            return false; //no overlap
        }
        else
        {
            //Try to find smallest overlap. This is used to push the shapes apart
            float newOverlap = min(pro1.max, pro2.max) - max(pro1.min, pro2.min);
            if (newOverlap < overlap)
            {
                overlap = newOverlap;
                smallest = aNormal;
            }
        }
    }
    //if all projections overlap then there is a collision
    Point centerA = shapeA.getPosition();
    Point centerB = shapeB.getPosition();
    Point pAB = { centerB.x - centerA.x, centerB.y - centerA.y };

    if (dotProduct(pAB, smallest) > 0)
    {
        smallest.x = -smallest.x;
        smallest.y = -smallest.y;
    }

    p.x = overlap * smallest.x;
    p.y = overlap * smallest.y;
    return true;
}


//Returns true if a collision and pushes shapeA out of shapeB
bool satCollision(SimpleShape& shapeA, SimpleShape& shapeB, Sprite& sprite)
{
    Point p = { 0.0f, 0.0f };
    if (satCollision(shapeA, shapeB, p))
    {
        shapeA.MoveShape(p.x, p.y);
        sprite.Move(glm::vec2(p.x, p.y));
        return true;
    }
    return false;

}

//Returns true if a collision and returns without pushing the shapes appart. The overlap is returned.
bool satCollision(SimpleShape & shapeA, SimpleShape & shapeB, float & x, float & y)
{
    Point p = { 0.0f, 0.0f };
    if (satCollision(shapeA, shapeB, p))
    {
        x = p.x;
        y = p.y;
        return true;
    }
    return false;
}

SimpleShape newSquare(glm::vec2 center, float w, float h)
{

    SimpleShape square;
    square.numberOfVerticies = 4;
    square.theVerticies.resize(4);
    square.shapePosition = {center.x, center.y};

    float halfWidth = w * 0.5f;
    float halfHeight = h * 0.5f;

    square.halfWidth = halfWidth;
    square.halfHeight = halfHeight;

    square.theVerticies[0] = {center.x - halfWidth, center.y - halfHeight};
    square.theVerticies[1] = {center.x + halfWidth, center.y - halfHeight};
    square.theVerticies[2] = {center.x + halfWidth, center.y + halfHeight};
    square.theVerticies[3] = {center.x - halfWidth, center.y + halfHeight};

    return square;
}

SimpleShape testTri(glm::vec2 position, float w, float h)
{

    SimpleShape tri;
    tri.numberOfVerticies = 3;
    tri.theVerticies.resize(3);
    tri.shapePosition = {position.x, position.y};

    float halfWidth = w * 0.5f;
    float halfHeight = h * 0.5f;

    tri.halfWidth = halfWidth;
    tri.halfHeight = halfHeight;

    tri.theVerticies[0] = {position.x + halfWidth, position.y - halfHeight};
    tri.theVerticies[1] = {position.x + halfWidth, position.y + halfHeight};
    tri.theVerticies[2] = {position.x - halfWidth, position.y + halfHeight};

    return tri;
}
