#ifndef __V_COLORPICKER_H__
#define __V_COLORPICKER_H__

#include "lvgl/lvgl.h"
#include <stdint.h>

typedef void (*color_callback_t)(uint32_t argb);

void v_show_color_picker(color_callback_t callback, lv_obj_t *parent, int16_t left, int16_t top);
void v_hide_color_picker(void);
#endif