#pragma once

#include "timer.h"

class Animation
{
    private:
        Timer timer;
        int frameCount;

    public:
        Animation() : timer(0), frameCount(0) {}
        Animation(int frameCount, float length) : frameCount(frameCount), timer(length)
        { }

        float getLength() const { return timer.getLength(); }
        void step (float deltaTime) { timer.step(deltaTime); }
        int getCurrentFrame() const
        {
            return static_cast<int>(timer.getTime() / timer.getLength() * frameCount);
        }
};
