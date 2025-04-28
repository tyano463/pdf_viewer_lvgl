#ifndef __V_COLOR_H__
#define __V_COLOR_H__

#include <stdint.h>

typedef union
{
    uint32_t value;
    struct
    {
        uint8_t alpha;
        uint8_t red;
        uint8_t green;
        uint8_t blue;
    } c;
} v_color_t;

v_color_t argb2vcolor(float a, float r, float g, float b);
#endif