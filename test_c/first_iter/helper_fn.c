// helper_fn.c
#include"codec.h"

/**
 * clamp - Clamp float value between min and max
 * @x: Value to clamp
 * @min: Minimum value
 * @max: Maximum value
 * 
 * Return: Clamped value
 */
float clamp(float x, float min, float max)
{
    if (x < min)
        return min;
    if (x > max)
        return max;
    return x;
}
