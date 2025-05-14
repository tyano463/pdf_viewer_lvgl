#ifndef __V_CORE_H__
#define __V_CORE_H__

#include "lvgl/lvgl.h"
#include "v_common.h"

typedef void (*v_message_callback_t)(bool result);
float distance(const lv_point_t *a, const lv_point_t *b);
float distancef(const lv_point_precise_t *a, const lv_point_t *b);
bool in_rect(lv_point_t *a, v_rect_t *rect);
void show_modal_dialog(lv_obj_t *parent, const char *title, v_message_callback_t callback);
lv_point_precise_t rect_center(const v_rect_t *p, const v_matrix_t *m);
float scale_factor(const v_matrix_t *m);
void matrix_multiply(v_matrix_t *result, v_matrix_t *m1, v_matrix_t *m2);
v_status_t v_check_external_command(v_ext_command_t *ext_command);
v_status_t v_init_external_receiver(void);
bool inverse_matrix(const v_matrix_t *orig, v_matrix_t *inv);
#endif