#ifndef __V_SLIDER_H__
#define __V_SLIDER_H__

#include "lvgl/lvgl.h"
#include <stdint.h>

typedef void (*v_slider_callback_t)(uint8_t value);
void v_show_slider(v_slider_callback_t callback, lv_obj_t *parent, lv_area_t *rect, uint16_t vmax);
void v_hide_slider(void);
#endif