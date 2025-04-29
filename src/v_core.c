#include "v_core.h"
#include <math.h>

float distance(lv_point_t *a, lv_point_t *b)
{
    float d2 = (b->x - a->x) * (b->x - a->x) + (b->y - a->y) * (b->y - a->y);
    return sqrtf(d2);
}