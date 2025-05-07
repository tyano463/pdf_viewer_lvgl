#include <stdlib.h>
#include "v_pen.h"
#include "v_canvas.h"
#include "v_icon.h"
#include "v_assets_list.h"
#include "v_colorpicker.h"
#include "v_slider.h"

#define COORD_CHUNK_NUM 100

static void color_callback(uint32_t argb);
static void width_callback(uint8_t width);

static lv_obj_t *pen_button;
static lv_obj_t *book_button;
static v_pen_ops_t ops;
static v_pen_cb_ops_t *cbs;
static v_stroke_t *active;

static v_mode_t mode;
lv_area_t g_slider_rect = {
    .x1 = 600,
    .y1 = 0,
    .x2 = 700,
    .y2 = 20,
};
static void mode_change(lv_event_t *e)
{
    if (cbs)
    {
        if (mode == MODE_NORMAL)
        {
            mode = MODE_PEN;
            lv_obj_add_flag(book_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(pen_button, LV_OBJ_FLAG_HIDDEN);
            v_show_color_picker(color_callback, lv_screen_active(), 300, 0);
            v_show_slider(width_callback, lv_screen_active(), &g_slider_rect, 48);
        }
        else
        {
            mode = MODE_NORMAL;
            lv_obj_remove_flag(book_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(pen_button, LV_OBJ_FLAG_HIDDEN);
            v_hide_color_picker();
            v_hide_slider();
        }
        d("mode -> %d", mode);
        cbs->on_mode_change(mode);
    }
}

static void v_pen_draw(float x, float y, float pressure, v_pen_draw_mode_t mode)
{
    if (mode == V_PEN_DRAW_START)
    {
        active = malloc(sizeof(v_stroke_t));
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
            active->points = realloc(active->points, sizeof(v_point_t) * active->max + COORD_CHUNK_NUM);
            active->max += COORD_CHUNK_NUM;
        }
        active->points[active->num].x = x;
        active->points[active->num].y = y;
        active->points[active->num].pressure = pressure;
        active->num++;
    }
}

static void v_pen_select(float x, float y, v_pen_draw_mode_t mode)
{
    if (mode == V_PEN_DRAW_START)
    {
    }
    else if (mode == V_PEN_DRAW_END)
    {
    }
}

v_status_t v_pen_init(lv_obj_t *parent, v_pen_cb_ops_t *_cbs)
{
    lv_img_dsc_t *pen_icon;
    lv_img_dsc_t *book_icon;
    pen_button = lv_image_create(parent);
    book_button = lv_image_create(parent);

    lv_obj_flag_t flag = LV_OBJ_FLAG_CLICKABLE;
    lv_obj_add_flag(pen_button, flag);
    lv_obj_add_flag(book_button, flag);

    pen_icon = get_icon_dsc("pen");
    book_icon = get_icon_dsc("open_book");
    lv_image_set_src(pen_button, pen_icon);
    lv_image_set_src(book_button, book_icon);
    lv_obj_align(pen_button, LV_ALIGN_TOP_RIGHT, -5, 5);
    lv_obj_align(book_button, LV_ALIGN_TOP_RIGHT, -5, 5);
    lv_obj_add_event_cb(pen_button, mode_change, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(book_button, mode_change, LV_EVENT_CLICKED, NULL);

    lv_obj_add_flag(pen_button, LV_OBJ_FLAG_HIDDEN);

    mode = MODE_NORMAL;
    cbs = _cbs;
    return ST_SUCCESS;
}

static void set_pen_icon_visivility(bool vis)
{
    if (vis)
    {
        lv_obj_t *target = (mode == MODE_NORMAL) ? book_button : pen_button;
        lv_obj_remove_flag(target, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_add_flag(pen_button, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(book_button, LV_OBJ_FLAG_HIDDEN);
    }
}

static void width_callback(uint8_t width)
{
    d("width: %d", width);
}
static void color_callback(uint32_t argb)
{
    d("color: %08x", argb);
}

static void hide_pen_icon(void)
{
    set_pen_icon_visivility(false);
    v_hide_color_picker();
    v_hide_slider();
}
static void show_pen_icon(void)
{
    set_pen_icon_visivility(true);
    if (mode == MODE_PEN)
    {
        d("");
        v_show_color_picker(color_callback, lv_screen_active(), 300, 0);
        v_show_slider(width_callback, lv_screen_active(), &g_slider_rect, 48);
    }
}
static void init_ops(void)
{
    if (!ops.init)
    {
        ops.init = v_pen_init;
        ops.draw = v_pen_draw;
        ops.select = v_pen_select;
        ops.show_icon = show_pen_icon;
        ops.hide_icon = hide_pen_icon;
    }
}
v_pen_ops_t *v_pen_getops(void)
{
    init_ops();
    return &ops;
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