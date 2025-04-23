#include "v_svg.h"


static v_draw_ops_t g_ops;

v_draw_ops_t *v_svg_get_ops(void)
{
    return &g_ops;
}