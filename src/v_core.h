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
void matrix_add(v_matrix_t *orig, v_matrix_t *m);
void matrix_multiply(v_matrix_t *result, v_matrix_t *m1, v_matrix_t *m2);
/**
 * @brief Solves for the affine transformation matrix m from the equation a × m = b.
 *
 * This function computes a 2D affine transformation matrix `m` such that multiplying
 * input matrix `a` with `m` yields the output matrix `b`, i.e., `a × m = b`.
 *
 * Both `a` and `b` are expected to be 2x3 matrices where each row represents
 * a 2D point in homogeneous coordinates, i.e., [x, y, 1].
 *
 * The resulting affine matrix `m` is also a 2x3 matrix of the form:
 * @code
 *     [ m00 m01 m02 ]
 *     [ m10 m11 m12 ]
 * @endcode
 *
 * Where:
 * - m00, m01, m10, m11 represent the linear transformation components (rotation, scaling, shear)
 * - m02, m12 represent the translation (bias) components
 *
 * This function assumes that the two rows of `a` provide enough information to determine
 * the affine transformation uniquely. If the determinant of the linear part is zero,
 * the system is singular and no unique solution exists.
 *
 * @param[out] m   The output affine transformation matrix (2x3)
 * @param[in]  a   Input matrix of two 2D points in homogeneous coordinates ([x, y, 1])
 * @param[in]  b   Output matrix of the transformed 2D points ([x', y', 1])
 *
 * @retval 1 if the solution was successfully computed
 * @retval 0 if the system is singular (e.g., input points are colinear)
 */
bool solve_matrix(v_matrix_t *m, const v_matrix_t *a, const v_matrix_t *b);
v_status_t v_check_external_command(v_ext_command_t *ext_command);
v_status_t v_init_external_receiver(void);
bool inverse_matrix(const v_matrix_t *orig, v_matrix_t *inv);
char *svg2pdf(const char *file);
char *png2pdf(const char *file);
char *jpeg2pdf(const char *file);

char *dump_matrix(v_matrix_t *m);
#endif