#include "v_core.h"
#include <math.h>
#include <sys/eventfd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define EVENTFD_PATH "/tmp/ipc_fifo"

static int fd_in;

float distance(const lv_point_t *a, const lv_point_t *b)
{
    float d2 = (b->x - a->x) * (b->x - a->x) + (b->y - a->y) * (b->y - a->y);
    return sqrtf(d2);
}

float distancef(const lv_point_precise_t *a, const lv_point_t *b)
{
    float d2 = (b->x - a->x) * (b->x - a->x) + (b->y - a->y) * (b->y - a->y);
    return sqrtf(d2);
}

bool in_rect(lv_point_t *a, v_rect_t *rect)
{
    bool ret = false;
    ERR_RET(!a || !rect, "point:%p, rect:%p", a, rect);

    // clang-format off
    ret = rect->left <= a->x && a->x <= rect->right 
        && rect->top <= a->y && a->y <= rect->bottom;
    // clang-format on

error_return:
    return ret;
}

static void modal_btn_event_cb(lv_event_t *e)
{
    const lv_obj_t *btn = lv_event_get_target(e);
    const char *btn_text = lv_label_get_text(lv_obj_get_child(btn, 0));
    v_message_callback_t callback = lv_event_get_user_data(e);
    bool result;

    result = strcmp(btn_text, "Yes") == 0;
    lv_obj_delete(lv_obj_get_parent(lv_obj_get_parent(lv_obj_get_parent(btn))));
    if (callback)
        callback(result);
}

void show_modal_dialog(lv_obj_t *parent, const char *title, v_message_callback_t callback)
{
    lv_obj_t *modal_bg = lv_obj_create(parent);
    lv_obj_remove_style_all(modal_bg);
    lv_obj_set_size(modal_bg, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(modal_bg, LV_OPA_50, 0);
    lv_obj_center(modal_bg);

    lv_obj_t *dialog = lv_obj_create(modal_bg);
    lv_obj_remove_style_all(dialog);
    lv_obj_set_size(dialog, 200, 120);
    lv_obj_center(dialog);
    lv_obj_set_style_border_width(dialog, 1, 0);
    lv_obj_set_style_border_color(dialog, lv_color_hex3(0x888), 0);
    lv_obj_set_style_shadow_width(dialog, 20, 0);
    lv_obj_set_style_bg_color(dialog, (lv_color_t){255, 255, 255}, 0);
    lv_obj_set_style_bg_opa(dialog, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(dialog, 10, 0);

    lv_obj_t *label = lv_label_create(dialog);
    lv_obj_remove_style_all(label);
    lv_label_set_text(label, title);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -30);

    lv_obj_t *btn_row = lv_obj_create(dialog);
    lv_obj_remove_style_all(btn_row);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_size(btn_row, 200, 40);
    lv_obj_align(btn_row, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_pad_gap(btn_row, 20, 0);
    lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);

    lv_obj_t *btn_yes = lv_btn_create(btn_row);
    lv_obj_t *label_yes = lv_label_create(btn_yes);
    lv_obj_remove_style_all(btn_yes);
    lv_obj_remove_style_all(label_yes);
    lv_label_set_text(label_yes, "Yes");
    lv_obj_set_size(btn_yes, 80, 30);
    lv_obj_center(label_yes);
    lv_obj_set_style_border_width(btn_yes, 1, 0);
    lv_obj_set_style_shadow_width(btn_yes, 20, 0);
    lv_obj_set_style_border_color(btn_yes, lv_color_hex3(0x888), 0);
    lv_obj_set_style_radius(btn_yes, 10, 0);
    lv_obj_add_event_cb(btn_yes, modal_btn_event_cb, LV_EVENT_CLICKED, callback);

    lv_obj_t *btn_no = lv_btn_create(btn_row);
    lv_obj_t *label_no = lv_label_create(btn_no);
    lv_obj_remove_style_all(btn_no);
    lv_obj_remove_style_all(label_no);
    lv_obj_set_size(btn_no, 80, 30);
    lv_label_set_text(label_no, "No");
    lv_obj_center(label_no);
    lv_obj_set_style_border_width(btn_no, 1, 0);
    lv_obj_set_style_shadow_width(btn_no, 20, 0);
    lv_obj_set_style_border_color(btn_no, lv_color_hex3(0x888), 0);
    lv_obj_set_style_radius(btn_no, 10, 0);
    lv_obj_add_event_cb(btn_no, modal_btn_event_cb, LV_EVENT_CLICKED, callback);
}

lv_point_precise_t rect_center(const v_rect_t *p, const v_matrix_t *m)
{
    lv_point_precise_t center = {0.0f, 0.0f};

    if (p)
    {
        center.x = (p->left + p->right) / 2.0f;
        center.y = (p->top + p->bottom) / 2.0f;

        if (m)
        {
            lv_point_precise_t transformed;
            transformed.x = m->elm[0][0] * center.x + m->elm[0][1] * center.y + m->elm[0][2];
            transformed.y = m->elm[1][0] * center.x + m->elm[1][1] * center.y + m->elm[1][2];

            center = transformed;
        }
    }

    return center;
}

float scale_factor(const v_matrix_t *m)
{
    if (!m)
        return 1.0f;

    float scale_x = sqrtf(m->elm[0][0] * m->elm[0][0] + m->elm[0][1] * m->elm[0][1]);
    float scale_y = sqrtf(m->elm[1][0] * m->elm[1][0] + m->elm[1][1] * m->elm[1][1]);

    return (scale_x + scale_y) / 2.0f;
}

void matrix_multiply(v_matrix_t *result, v_matrix_t *m1, v_matrix_t *m2)
{
    if (!result || !m1 || !m2)
        return;

    v_matrix_t temp;

    // 行列の積を計算 (2×3 行列の乗算)
    temp.elm[0][0] = m1->elm[0][0] * m2->elm[0][0] + m1->elm[0][1] * m2->elm[1][0];
    temp.elm[0][1] = m1->elm[0][0] * m2->elm[0][1] + m1->elm[0][1] * m2->elm[1][1];
    temp.elm[0][2] = m1->elm[0][0] * m2->elm[0][2] + m1->elm[0][1] * m2->elm[1][2] + m1->elm[0][2];

    temp.elm[1][0] = m1->elm[1][0] * m2->elm[0][0] + m1->elm[1][1] * m2->elm[1][0];
    temp.elm[1][1] = m1->elm[1][0] * m2->elm[0][1] + m1->elm[1][1] * m2->elm[1][1];
    temp.elm[1][2] = m1->elm[1][0] * m2->elm[0][2] + m1->elm[1][1] * m2->elm[1][2] + m1->elm[1][2];

    // 結果を返す
    *result = temp;
}

v_status_t v_init_external_receiver(void)
{
    fd_in = 0;
    int ret;
    ret = mkfifo(EVENTFD_PATH, 0666);
    ERR_RET(ret < 0 && errno != EEXIST, "fd create fail");

    fd_in = open(EVENTFD_PATH, O_RDONLY | O_NONBLOCK);

error_return:
    return fd_in >= 0 ? ST_SUCCESS : ST_EXT_RECEIVER_INIT_FAILED;
}

v_status_t v_check_external_command(v_ext_command_t *ext_command)
{
    v_status_t status = ST_EXT_RECEIVER_INIT_FAILED;
    ERR_RET(fd_in <= 0, "efd open failed");

    ssize_t n = read(fd_in, ext_command, sizeof(v_ext_command_t));

    if (n > 0)
    {
        status = ST_SUCCESS;
    }
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
        status = ST_EXT_NO_RECEIVE;
    }
error_return:
    return status;
}

bool inverse_matrix(const v_matrix_t *orig, v_matrix_t *inv)
{
    // アフィン変換行列の要素を抽出
    float a = orig->elm[0][0];
    float c = orig->elm[0][1];
    float e = orig->elm[0][2];
    float b = orig->elm[1][0];
    float d = orig->elm[1][1];
    float f = orig->elm[1][2];

    // 行列の 2x2 部分の行列式を計算
    float det = a * d - b * c;
    if (det == 0.0f)
        return false; // 逆行列が存在しない

    float inv_det = 1.0f / det;

    // 逆行列を計算
    inv->elm[0][0] = d * inv_det;
    inv->elm[0][1] = -c * inv_det;
    inv->elm[0][2] = (c * f - d * e) * inv_det;

    inv->elm[1][0] = -b * inv_det;
    inv->elm[1][1] = a * inv_det;
    inv->elm[1][2] = (b * e - a * f) * inv_det;

    return true;
}
