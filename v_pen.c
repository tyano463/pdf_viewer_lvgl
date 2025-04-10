#include <stdlib.h>
#include "v_pen.h"

#define COORD_CHUNK_NUM 100

static lv_obj_t *pen_button;
static v_pen_ops_t *ops;
static v_stroke_data_t data;
static v_stroke_t *active;

static void v_pen_draw(float x, float y, float pressure, v_pen_draw_mode_t mode)
{
    if (mode == V_PEN_DRAW_START)
    {
        active = malloc(sizeof(v_stroke_t));
        active->num = 1;
        active->max = COORD_CHUNK_NUM;
        active->points = malloc(sizeof(v_point_t) * COORD_CHUNK_NUM);
        active->points[0].x = x;
        active->points[0].y = y;
        active->points[0].pressure = pressure;
    }
    else
    {
        if (!(active->num % COORD_CHUNK_NUM))
        {
            active->points = realloc(active->points, sizeof(v_point_t) * active->max + COORD_CHUNK_NUM);
            active->max += COORD_CHUNK_NUM;
        }
        active->points[active->num].x = x;
        active->points[active->num].y = y;
        active->points[active->num].pressure = pressure;
        active->num++;
        if (mode == V_PEN_DRAW_END) {
            data.strokes[data.num++] = active;
        }
    }
}

v_status_t v_pen_init(lv_obj_t *parent, v_pen_ops_t *_ops)
{
    lv_img_dsc_t *icon;
    pen_button = lv_imagebutton_create(parent);

    lv_imagebutton_set_src(pen_button, LV_IMAGEBUTTON_STATE_RELEASED, NULL, icon, NULL);
    data.num = 0;

    ops = _ops;
}

v_pen_ops_t *v_pen_getops(void) {
    return ops;
}

v_stroke_data_t *all_strokes(void)
{
    return &data;
}