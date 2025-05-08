#ifndef __V_SLIDER_H__
#define __V_SLIDER_H__

#include "lvgl/lvgl.h"
#include <stdint.h>

typedef struct str_v_slider_param
{
    float minimum_value;
    float initial_value;
    float maximum_value;
} v_slider_param_t;

typedef void (*v_slider_callback_t)(uint8_t value);
lv_obj_t *v_create_slider(v_slider_callback_t callback, lv_obj_t *parent, lv_area_t *rect, v_slider_param_t *param);
void v_show_slider(lv_obj_t *);
void v_hide_slider(lv_obj_t *);
#endif