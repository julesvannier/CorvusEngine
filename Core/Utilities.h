#pragma once

class Utilities
{
public:
    static float RandomFloatRange(float minRange, float maxRange);
    static float Lerp(float a, float b, float t) { return a + (b - a) * t; }
};
