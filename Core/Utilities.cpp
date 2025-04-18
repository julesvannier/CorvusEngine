#include "Utilities.h"
#include <random>

float Utilities::RandomFloatRange(float minRange, float maxRange)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> distribution(minRange, maxRange);
    return distribution(gen);
}