#pragma once
#include <vector>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include<gtx/rotate_vector.hpp>

class Sprite
{
public:
    Sprite();
    ~Sprite();

    void init(glm::vec2 p, glm::vec2 s, const glm::ivec4 & hb,  std::vector<glm::vec4> & u );

    glm::mat4 getTranslation();

    void setTranslation(const glm::mat4 & m);

    glm::vec2 getPosition()
    {
        return glm::vec2(getTranslation()[3]);
    }

    glm::vec4 getHitBox()
    {
        glm::vec2 s = getPosition();
        glm::vec4 hb = hitBox;
        hb.x += s.x;
        hb.y += s.y;
        return hb;
    }
    void setHitBox(glm::vec4 hb)
    {
        hitBox = hb;
    }

    void setSize(glm::vec2 size)
    {
        this->size = size;
    }

    //I think this is an ivec because the dimensions will probably always be integer numbers
    glm::vec2 getHitBoxDimensions()
    {
        return glm::vec2(hitBox.z, hitBox.w);
    }

    glm::vec4 getUV()
    {
        std::vector<glm::vec4> v = *uv;
        return v[currentFrame];
    }

    glm::vec2 getSize()
    {
        return size;
    }

    glm::vec2 getCenter()
    {
        return getPosition() + size * 0.5f;
    }

    glm::vec2 getHitBoxCenter()
    {
        glm::vec4 hb = getHitBox();

        return glm::vec2(hb.x, hb.y) + getHitBoxDimensions() * 0.5f;
    }

    //I don't know where to put this honestly
    glm::vec2 rotateVector(glm::vec2 a, glm::vec2 b, float angle)
    {

        /*  float x = ((a.x - b.x) * cos(angle)) - ((b.y - a.y) * sin(angle)) + b.x;
          float y = ((b.y - a.y) * cos(angle)) - ((a.x - b.x) * sin(angle)) + b.y;*/

        float s = sin(angle);
        float c = cos(angle);

        float x = ((a.x - b.x) * c) - ((a.y - b.y) * s) + b.x;
        float y = ((a.x - b.x) * s) + ((a.y - b.y) * c) + b.y;

        return glm::vec2(x, y);
    }

    void addUVs(std::vector<glm::vec4>& newUVs)
    {
        if(uv == nullptr)
        {
            uv = &newUVs;
        }
        else
        {
            std::vector<glm::vec4> n = *uv;
            n.insert(n.end(), newUVs.begin(), newUVs.end());
            uv = &n;
            /*std::vector<int> AB = A;
            AB.insert(AB.end(), B.begin(), B.end());*/
        }
    }

    //TODO this doesn't need two parameters, can just use hitbox in place of parameter "a"
    bool collide(glm::vec4 a, glm::vec4 b);

    void setParent(Sprite * p);

    void Move(glm::vec2 movement); // add movement to position
    void SetPosition(glm::vec2 newPosition);

    void playAnimation(float delta, int numberOfFrames, bool doubleFrames);
    bool playAnimationOnce(float delta, int numberOfFrames, bool doubleFrames);
    //Will play the animation and then reverse it
    bool playAnimationReverse(float delta, int numberOfFrames, bool doubleFrames);

    void HandleAnimation(float deltaTime, int idleFrame);

    void checkMatrixUpdate();

    void setFocus();

    bool active;
    bool updateDrawMatrix;
    bool moveAnimate = false;

    int currentFrame = 0;
    int startingFrame = 0;

    //0 left, 1 up, 2 right, 3 down
    int facing;
    int focusedFacing;

    float rotation; //this and facing are kind of odd

    float alpha = 1.0f;
    float timeForFrame = 0.0f;

    glm::vec3 color;
    std::vector<glm::vec4>* uv = nullptr;
    //For sprites that are not 16x16, but still need to be position at such locations.
    glm::vec2 drawOffset = glm::vec2(0);

    Sprite *parent = nullptr;

private:

    glm::vec2 size;
    glm::vec2 position;
    glm::vec2 previousPosition;

    glm::vec4 hitBox; //offsets and size

    glm::mat4 matrix;
    glm::mat4 localMatrix;
    glm::mat4 drawMatrix;

};



