#include <stdlib.h>
#include "v_pen.h"
#include "v_canvas.h"
#include "v_icon.h"
#include "v_assets_list.h"
#include "v_colorpicker.h"
#include "v_slider.h"
#include "v_settings.h"
#include "v_core.h"
#include "v_misc.h"
#include "v_undo.h"

#define COORD_CHUNK_NUM 100

static void color_callback(uint32_t argb);
static void width_callback(uint8_t width);
static void opacue_callback(uint8_t opacue);
static void show_pen_icon(void);
static void hide_pen_icon(void);
static void set_pen_icon_visivility(bool vis);
static void hide_sample(void);
static void v_show_undo_icon(void);
static void v_hide_undo_icon(void);

static lv_obj_t *pen_button;
static lv_obj_t *book_button;
static lv_obj_t *select_button;
static v_pen_ops_t g_ops;
static v_pen_cb_ops_t *cbs;
static v_stroke_t *active;
static lv_obj_t *width_slider;
static lv_obj_t *opacue_slider;
static lv_obj_t *sample;
static v_pen_t *current_pen;
static lv_obj_t *undo_container;

static v_user_mode_t user_mode;
static v_show_mode_t show_mode;
static const uint16_t PICKER_LEFT = 120;
static const uint16_t PICKER_TOP = 0;
lv_area_t g_sample_rect = {
    .x1 = 570,
    .y1 = 6,
    .x2 = 594,
    .y2 = 30,
};
lv_area_t g_width_slider_rect = {
    .x1 = 360,
    .y1 = 12,
    .x2 = 440,
    .y2 = 18,
};
lv_area_t g_opacue_slider_rect = {
    .x1 = 470,
    .y1 = 12,
    .x2 = 550,
    .y2 = 18,
};
static void user_mode_change(lv_event_t *e)
{
    user_mode++;
    user_mode %= V_USER_MODE_MAX;
    d("%s(%d)", v_user_mode_t_to_string(user_mode), user_mode);
    if (user_mode == V_USER_MODE_PEN)
    {
        show_pen_icon();
        v_show_undo_icon();
    }
    else if (user_mode == V_USER_MODE_NORMAL)
    {
        hide_pen_icon();
        v_hide_undo_icon();
    }
    else if (user_mode == V_USER_MODE_SELECT)
    {
        hide_pen_icon();
        v_show_undo_icon();
    }
    if (cbs)
    {
        cbs->on_user_mode_change(user_mode);
    }
}

static void v_pen_draw(int32_t x, int32_t y, uint8_t pressure, v_pen_draw_mode_t mode)
{
    // d("%d,%d %d", x, y, mode);
    if (mode == V_PEN_DRAW_START)
    {
        active = malloc(sizeof(v_stroke_t));
        ERR_RET(!active, "malloc");
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
            void *tmp = realloc(active->points, sizeof(v_point_t) * active->max + COORD_CHUNK_NUM);
            ERR_RET(!tmp, "realloc");
            active->points = tmp;
            active->max += COORD_CHUNK_NUM;
        }
        active->points[active->num].x = x;
        active->points[active->num].y = y;
        active->points[active->num].pressure = pressure;
        active->num++;
        if (mode == V_PEN_DRAW_END)
        {
            v_annot_t *annot = malloc(sizeof(v_annot_t));
            ERR_RET(!annot, "malloc");
            annot->kind = V_ANNOT_INKLIST;
            annot->id = generate_id();
            annot->data.inklist.num = 1;
            annot->data.inklist.coord_type = V_ANNOT_COORD_NEW;
            memcpy(&annot->data.inklist.pen, current_pen, sizeof(v_pen_t));
            annot->data.inklist.strokes = active;
            annot->matrix = (v_matrix_t){.elm = {{1.0f, 0.0f, 0.0f},
                                                 {0.0f, 1.0f, 0.0f}}};
            active = NULL;
            v_viewer_ops_t *ops = v_get_canvas_ops();
            v_annot_t *next = v_clone_annot(annot);
            undo_push(NULL, next, UNDO_ACTION_NEW_INK);
            ops->add_annot(annot);
        }
    }
error_return:
    return;
}

static void v_pen_select(float x, float y, v_pen_draw_mode_t mode)
{
    static lv_point_t touch_start;
    if (mode == V_PEN_DRAW_START)
    {
        touch_start.x = x;
        touch_start.y = y;
    }
    else if (mode == V_PEN_DRAW_END)
    {
        lv_point_t pos = {
            .x = x,
            .y = y,
        };
        float d = distance(&touch_start, &pos);
        v_viewer_ops_t *ops = v_get_canvas_ops();
        if (is_click(d))
        {
            ops->select(&pos);
        }
        else
        {
            ops->move(&touch_start, &pos);
        }
    }
}

static v_status_t v_pen_init(lv_obj_t *parent, v_pen_cb_ops_t *_cbs)
{
    const lv_img_dsc_t *pen_icon;
    const lv_img_dsc_t *book_icon;
    const lv_img_dsc_t *select_icon;
    pen_button = lv_image_create(parent);
    book_button = lv_image_create(parent);
    select_button = lv_image_create(parent);

    lv_obj_flag_t flag = LV_OBJ_FLAG_CLICKABLE;
    lv_obj_add_flag(pen_button, flag);
    lv_obj_add_flag(book_button, flag);
    lv_obj_add_flag(select_button, flag);

    pen_icon = get_icon_dsc("pen");
    book_icon = get_icon_dsc("open_book");
    select_icon = get_icon_dsc("select");
    lv_image_set_src(pen_button, pen_icon);
    lv_image_set_src(book_button, book_icon);
    lv_image_set_src(select_button, select_icon);
    lv_obj_align(pen_button, LV_ALIGN_TOP_RIGHT, -5, 5);
    lv_obj_align(book_button, LV_ALIGN_TOP_RIGHT, -5, 5);
    lv_obj_align(select_button, LV_ALIGN_TOP_RIGHT, -5, 5);
    lv_obj_add_event_cb(pen_button, user_mode_change, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(book_button, user_mode_change, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(select_button, user_mode_change, LV_EVENT_CLICKED, NULL);

    lv_obj_add_flag(pen_button, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(select_button, LV_OBJ_FLAG_HIDDEN);

    cbs = _cbs;
    return ST_SUCCESS;
}

static void set_pen_icon_visivility(bool vis)
{
    if (show_mode != V_SHOW_MODE_ANNOT_WITH_MENU)
    {
        lv_obj_add_flag(book_button, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(pen_button, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(select_button, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        if (user_mode == V_USER_MODE_PEN)
        {
            lv_obj_add_flag(book_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(select_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(pen_button, LV_OBJ_FLAG_HIDDEN);
        }
        else if (user_mode == V_USER_MODE_SELECT)
        {
            lv_obj_add_flag(pen_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(book_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(select_button, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(pen_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(select_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(book_button, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void update_sample(void)
{
    ERR_RETn(!sample);

    lv_canvas_fill_bg(sample, lv_color_hex3(0xccc), LV_OPA_TRANSP);

    lv_layer_t layer;
    lv_canvas_init_layer(sample, &layer);

    v_color_t *color = &current_pen->color;
    d("pen:%02x %02x%02x%02x", color->c.alpha, color->c.blue, color->c.green, color->c.red);
    uint8_t size = current_pen->size;

    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color = (lv_color_t){
        color->c.blue,
        color->c.green,
        color->c.red,
    };
    dsc.bg_opa = color->c.alpha;
    dsc.radius = LV_RADIUS_CIRCLE;

    lv_area_t circle_area;
    uint8_t offset = ((PEN_SIZE_MAX - size) / 2);
    circle_area.x1 = 0 + offset;
    circle_area.y1 = 0 + offset;
    circle_area.x2 = PEN_SIZE_MAX - 1 - offset;
    circle_area.y2 = PEN_SIZE_MAX - 1 - offset;

    lv_draw_rect(&layer, &dsc, &circle_area);
    lv_canvas_finish_layer(sample, &layer);
error_return:
    return;
}
static void opacue_callback(uint8_t opacue)
{
    //  d("opacue: %d", opacue);
    current_pen->color.c.alpha = opacue;
    update_sample();
}
static void width_callback(uint8_t width)
{
    //    d("width: %d", width);
    current_pen->size = width;
    update_sample();
}
static void color_callback(uint32_t argb)
{
    //   d("color: %08x", argb);
    current_pen->color.c.red = (argb & 0xff0000) >> 16;
    current_pen->color.c.green = (argb & 0x00ff00) >> 8;
    current_pen->color.c.blue = (argb & 0x0000ff) >> 0;
    update_sample();
}

static void hide_pen_icon(void)
{
    set_pen_icon_visivility(false);
    v_hide_color_picker();
    v_hide_slider(width_slider);
    v_hide_slider(opacue_slider);
    hide_sample();
}

static void _hide_pen_icon(void)
{
    show_mode = V_SHOW_MODE_ANNOT_NO_MENU;
    hide_pen_icon();
    v_hide_undo_icon();
}

static void set_visibility(lv_obj_t *obj, bool vis)
{
    if (!obj)
        return;
    void (*func)(lv_obj_t *, lv_obj_flag_t);
    func = vis ? lv_obj_remove_flag : lv_obj_add_flag;
    func(obj, LV_OBJ_FLAG_HIDDEN);
}

static void hide_sample(void)
{
    set_visibility(sample, false);
}

static v_pen_t *last_pen(void)
{
    v_pen_t *pen = NULL;
    v_pen_t _pen = {
        .color.c.alpha = 255,
        .color.c.red = 0,
        .color.c.green = 0,
        .color.c.blue = 0,
        .shape.kind = V_PEN_SHAPE_ROUND,
        .size = 3,
    };
    v_settings_ops_t *ops = v_get_settings_ops();
    if (ops)
        pen = ops->get_pen();

    if (!pen)
    {
        pen = malloc(sizeof(v_pen_t));
        ERR_RET(!pen, "malloc");
        memcpy(pen, &_pen, sizeof(v_pen_t));
    }
error_return:
    return pen;
}
static void show_sample(lv_area_t *p)
{
    if (sample)
    {
        set_visibility(sample, true);
        goto error_return;
    }

    int16_t w, h;
    w = PEN_SIZE_MAX;
    h = PEN_SIZE_MAX;

    current_pen = last_pen();

    d("%d,%d - %d, %d", p->x1, p->y1, p->x2, p->y2);
    sample = lv_canvas_create(lv_screen_active());
    lv_obj_set_pos(sample, p->x1, p->y1);
    lv_obj_set_size(sample, w, h);
    lv_draw_buf_t *buf = malloc(sizeof(lv_draw_buf_t) + w * h * 4);
    ERR_RET(!buf, "malloc");
    lv_draw_buf_init(buf, w, h, LV_COLOR_FORMAT_ARGB8888, w * 4, (void *)&buf[1], w * h * 4);
    memset(buf->data, 128, w * h * 4);
    lv_canvas_set_draw_buf(sample, buf);

    update_sample();
error_return:
    return;
}

static void show_pen_icon(void)
{
    if (user_mode == V_USER_MODE_PEN)
    {
        v_pen_t *pen = current_pen = last_pen();
        v_color_t c = pen->color;
        lv_color32_t color = (lv_color32_t){c.c.blue, c.c.green, c.c.red, c.c.alpha};

        v_show_color_picker(color_callback, lv_screen_active(), PICKER_LEFT, PICKER_TOP, color);
        if (!width_slider)
        {
            v_slider_param_t param = {
                .initial_value = max(pen->size, PEN_SIZE_MIN),
                .minimum_value = PEN_SIZE_MIN,
                .maximum_value = PEN_SIZE_MAX,
            };
            width_slider = v_create_slider(width_callback, lv_screen_active(), &g_width_slider_rect, &param);
        }
        if (!opacue_slider)
        {
            v_slider_param_t param = {
                .initial_value = c.c.alpha,
                .minimum_value = 0,
                .maximum_value = 255,
            };
            opacue_slider = v_create_slider(opacue_callback, lv_screen_active(), &g_opacue_slider_rect, &param);
        }
        v_show_slider(width_slider);
        v_show_slider(opacue_slider);
        show_sample(&g_sample_rect);
        v_show_undo_icon();
    }
    else if (user_mode == V_USER_MODE_SELECT)
    {
        v_show_undo_icon();
    }
    else
    {
        v_hide_undo_icon();
    }
    set_pen_icon_visivility(true);
}
static void _show_pen_icon(void)
{
    show_mode = V_SHOW_MODE_ANNOT_WITH_MENU;
    show_pen_icon();
}
static void init_ops(void)
{
    if (!g_ops.init)
    {
        g_ops.init = v_pen_init;
        g_ops.draw = v_pen_draw;
        g_ops.select = v_pen_select;
        g_ops.show_icon = _show_pen_icon;
        g_ops.hide_icon = _hide_pen_icon;
    }
}
v_pen_ops_t *v_pen_getops(void)
{
    init_ops();
    return &g_ops;
}

cJSON *pen_to_json(v_pen_t *pen)
{
    cJSON *json = NULL;
    cJSON *color;
    cJSON *shape;
    cJSON *size;
    cJSON *_json = cJSON_CreateObject();
    ERR_RET(!_json, "json");

    color = cJSON_CreateNumber(pen->color.value);
    ERR_RET(!color, "color");

    shape = cJSON_CreateNumber(pen->shape.kind);
    ERR_RET(!shape, "shape");

    size = cJSON_CreateNumber(pen->size);
    ERR_RET(!size, "size");

    cJSON_AddItemToObject(_json, "color", color);
    cJSON_AddItemToObject(_json, "shape", shape);
    cJSON_AddItemToObject(_json, "size", size);

    json = _json;
error_return:
    return json;
}

v_pen_t *json_to_pen(cJSON *json)
{
    v_pen_t *pen = NULL;
    cJSON *color;
    cJSON *shape;
    cJSON *size;

    color = cJSON_GetObjectItemCaseSensitive(json, "color");
    ERR_RET(!color, "color");
    shape = cJSON_GetObjectItemCaseSensitive(json, "shape");
    ERR_RET(!shape, "shape");
    size = cJSON_GetObjectItemCaseSensitive(json, "size");
    ERR_RET(!color, "size");

    pen = malloc(sizeof(v_pen_t));
    ERR_RET(!pen, "malloc");
    pen->color.value = color->valueint;
    pen->shape.kind = shape->valueint;
    pen->size = size->valueint;

error_return:
    return pen;
}

static void v_hide_undo_icon(void)
{
    if (!undo_container)
        return;
    lv_obj_add_flag(undo_container, LV_OBJ_FLAG_HIDDEN);
}

static void undo_redo_callback(lv_event_t *e)
{
    v_undo_type_t kind = (v_undo_type_t)(intptr_t)lv_event_get_user_data(e);
    v_viewer_ops_t *ops = v_get_canvas_ops();
    if (!ops)
        return;
    if (kind == V_UNDO_TYPE_UNDO)
    {
        ops->undo();
    }
    else if (kind == V_UNDO_TYPE_REDO)
    {
        ops->redo();
    }
}

static void v_show_undo_icon(void)
{
    if (undo_container)
    {
        lv_obj_remove_flag(undo_container, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    d("");
    undo_container = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(undo_container);
    lv_obj_align(undo_container, LV_ALIGN_TOP_RIGHT, -60, 10);
    lv_obj_set_flex_flow(undo_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(undo_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(undo_container, 20, 0);
    lv_obj_set_size(undo_container, 100, 30);

    lv_obj_t *undo_button = lv_image_create(undo_container);
    lv_obj_t *redo_button = lv_image_create(undo_container);
    const lv_image_dsc_t *undo_image = get_icon_dsc("undo");
    const lv_image_dsc_t *redo_image = get_icon_dsc("redo");

    lv_image_set_src(undo_button, undo_image);
    lv_image_set_src(redo_button, redo_image);

    lv_obj_add_flag(undo_button, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(redo_button, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(undo_button, undo_redo_callback, LV_EVENT_SINGLE_CLICKED, (void *)(intptr_t)V_UNDO_TYPE_UNDO);
    lv_obj_add_event_cb(redo_button, undo_redo_callback, LV_EVENT_SINGLE_CLICKED, (void *)(intptr_t)V_UNDO_TYPE_REDO);
}