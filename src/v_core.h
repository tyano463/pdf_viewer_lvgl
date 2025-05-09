#ifndef __V_CORE_H__
#define __V_CORE_H__

#include "lvgl/lvgl.h"
#include "v_common.h"

typedef void (*v_message_callback_t)(bool result);
float distance(lv_point_t *a, lv_point_t *b);
float distancef(lv_point_precise_t *a, lv_point_t *b);
bool in_rect(lv_point_t *a, v_rect_t *rect);
void show_modal_dialog(lv_obj_t *parent, const char *title, v_message_callback_t callback);
lv_point_precise_t rect_center(v_rect_t *p, v_matrix_t *m);
float scale_factor(v_matrix_t *m);
void matrix_multiply(v_matrix_t *result, v_matrix_t *m1, v_matrix_t *m2);
#endif