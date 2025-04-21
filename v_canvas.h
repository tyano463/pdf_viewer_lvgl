#ifndef __V_CANVAS_H__
#define __V_CANVAS_H__

#include "v_common.h"
#include "v_pen.h"

typedef struct str_v_image
{
    uint8_t *buf;
    uint16_t width;
    uint16_t height;
    uint32_t size;
    lv_color_format_t format;
} v_image_t;

v_status_t v_init_canvas(lv_obj_t *parent);
v_status_t v_show_image(v_image_t *image);
void v_set_touch_callback(lv_event_cb_t cb);

#endif