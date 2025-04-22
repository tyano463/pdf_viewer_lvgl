#include "v_jpeg.h"

static v_draw_ops_t g_ops;

v_draw_ops_t *v_jpeg_get_ops(void)
{
    return &g_ops;
}