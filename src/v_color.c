#include "v_color.h"

#ifndef min
#define min(a, b) (((a) > (b)) ? (b) : (a))
#endif

v_color_t argb2vcolor(float a, float r, float g, float b)
{
    v_color_t color;
    color.c.alpha = min(a, 1) * 255;
    color.c.red = min(r, 1) * 255;
    color.c.green = min(g, 1) * 255;
    color.c.blue = min(b, 1) * 255;
    return color;
}