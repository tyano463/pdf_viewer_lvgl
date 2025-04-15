#ifndef __V_CANVAS_H__
#define __V_CANVAS_H__

#include "v_common.h"
#include "v_pen.h"

v_status_t show_canvas(lv_obj_t *parent);
void draw_line_to(float x, float y, v_pen_draw_mode_t mode);

#endif