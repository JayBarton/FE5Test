#include "Sprite.h"
Sprite::Sprite() : position(0.0f), currentFrame(0), rotation(0.0f)
{
    color = glm::vec3(1.0f);
}


Sprite::~Sprite()
{
}

void Sprite::init(glm::vec2 p, glm::vec2 s, const glm::ivec4 & hb,  std::vector<glm::vec4> & u )
{
    SetPosition(p);
    size = s;
    uv = &u;
    hitBox = hb;
}


glm::mat4 Sprite::getTranslation()
{
    if (parent == nullptr)
    {
        drawMatrix = matrix;
    }
    else
    {
        if (updateDrawMatrix)
        {
            if (parent->parent != nullptr)
            {
                drawMatrix = parent->drawMatrix * localMatrix;
            }
            else
            {
                drawMatrix = parent->getTranslation() * localMatrix;
            }
        }
    }

    return drawMatrix;
}

void Sprite::setTranslation(const glm::mat4 &m)
{
    if (parent == nullptr)
    {
        matrix = m;
    }
    else
    {
        localMatrix = glm::inverse(parent->getTranslation()) * m;
    }
}

bool Sprite::collide(glm::vec4 a, glm::vec4 b)
{
    return a.x + a.z >= b.x &&
           a.x <= b.x + b.z &&
           a.y + a.w >= b.y &&
           a.y <= b.y + b.w;
}

//first check if the parent being set is not already the parent. If not, check if this sprite has a parent at all. If it does then
//the transform needs to be recalculated. Afterwards if the parent to be set is not null calculate the local transform
void Sprite::setParent(Sprite* p)
{
    if (p != parent)
    {
        if (parent != nullptr)
        {
            matrix = getTranslation();
        }
        parent = p;

        if (p != nullptr)
        {
            localMatrix = glm::inverse(parent->getTranslation()) * matrix;
            if (p->updateDrawMatrix)
            {
                updateDrawMatrix = true;
            }
            else
            {
                //	updateDrawMatrix = false;
            }
        }
        else
        {
            updateDrawMatrix = true;
        }
    }
}


void Sprite::Move(glm::vec2 movement)
{

    glm::vec2 newPosition(0);
    newPosition = getPosition() + movement;

    SetPosition(newPosition);
}

void Sprite::SetPosition(glm::vec2 newPosition)
{
    //float xOffset = hitBox.x - position.x;
    //float yOffset = hitBox.y - position.y;

    //previousPosition = position;
    position = newPosition;


    glm::mat4 translate = glm::translate(glm::vec3(newPosition, 0.0f));
    setTranslation(translate);

//	hitBox.x = position.x + xOffset;
    //hitBox.y = position.y + yOffset;
    updateDrawMatrix = true;
}

void Sprite::HandleAnimation(float deltaTime, int idleFrame)
{
    if (moveAnimate)
    {
        //playAnimation(deltaTime, 4, true);
        timeForFrame += deltaTime;
        float animationDelay = 0.0f;
        //This delay was eyeballed from the original game. I don't think it's exact, but I don't know how else to derive it.
        animationDelay = 0.11f; 
        if (timeForFrame >= animationDelay)
        {
            timeForFrame = 0;
            moveDelay++;
            if (moveDelay > 2)
            {
                moveDelay = 0;
            }
            else
            {
                int lastFrame = startingFrame + 4 - 1;
                if (currentFrame < lastFrame)
                {
                    currentFrame++;
                }
                else
                {
                    currentFrame = startingFrame;
                }
            }
        }
    }
    else
    {
        currentFrame = idleFrame;
    }
}

void Sprite::checkMatrixUpdate()
{
    if (parent == nullptr)
    {
        if (previousPosition == getPosition())
        {
            updateDrawMatrix = false;
        }
        else
        {
            previousPosition = glm::vec2(drawMatrix[3]);
        }
    }
    if (parent != nullptr)
    {
        if (parent->updateDrawMatrix)
        {
            updateDrawMatrix = true;
        }
        else
        {
            //	matrix = getTranslation();
            updateDrawMatrix = false;
        }
    }
}

void Sprite::setFocus()
{
    currentFrame = 3 + (focusedFacing * 4);
    startingFrame = currentFrame;
    moveAnimate = true;
}

void Sprite::playAnimation(float delta, int numberOfFrames, bool doubleFrames)
{
    timeForFrame += delta;
    float animationDelay = 0.0f;
    if (doubleFrames)
    {
        animationDelay = (numberOfFrames / 60.0f) * 2;
    }
    else
    {
        animationDelay = (numberOfFrames / 60.0f);
    }
    if (timeForFrame >= animationDelay)
    {
        timeForFrame = 0;
        int lastFrame = startingFrame + numberOfFrames - 1;
        if (currentFrame < lastFrame)
        {
            currentFrame++;
        }
        else
        {
            currentFrame = startingFrame;
        }
    }
}

//Would like to find a way to do this with less code duplication
bool Sprite::playAnimationOnce(float delta, int numberOfFrames, bool doubleFrames)
{
    timeForFrame += delta;
    float animationDelay = 0.0f;
    bool finished = false;
    if (doubleFrames)
    {
        animationDelay = (numberOfFrames / 60.0f) * 2;
    }
    else
    {
        animationDelay = (numberOfFrames / 60.0f);
    }
    if (timeForFrame >= animationDelay)
    {
        timeForFrame = 0;
        int lastFrame = startingFrame + numberOfFrames - 1;
        if (currentFrame < lastFrame)
        {
            currentFrame++;
        }
        else
        {
            finished = true;
        }
    }
    return finished;
}

//if
//play animation
