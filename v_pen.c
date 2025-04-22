#include <stdlib.h>
#include "v_pen.h"
#include "v_canvas.h"
#include "v_icon.h"
#include "v_assets_list.h"

#define COORD_CHUNK_NUM 100

static lv_obj_t *pen_button;
static v_pen_ops_t *ops;
static v_stroke_data_t data;
static v_stroke_t *active;

static v_mode_t mode;

static void mode_change(lv_event_t *e)
{
    if (ops)
    {
        if (mode == MODE_NORMAL)
        {
            mode = MODE_PEN;
        }
        else
        {
            mode = MODE_NORMAL;
        }
        d("mode -> %d", mode);
        ops->mode(mode);
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
        if (mode == V_PEN_DRAW_END)
        {
            data.strokes[data.num++] = active;
        }
    }
}

v_status_t v_pen_init(lv_obj_t *parent, v_pen_ops_t *_ops)
{
    lv_img_dsc_t *icon;
    uint8_t w, h;
    pen_button = lv_image_create(parent);

    lv_obj_flag_t flag = LV_OBJ_FLAG_CLICKABLE;
    lv_obj_add_flag(pen_button, flag);

    icon = get_icon_dsc("pen24");
    lv_image_set_src(pen_button, icon);
    lv_obj_align(pen_button, LV_ALIGN_TOP_RIGHT, -5, 5);
    lv_obj_add_event_cb(pen_button, mode_change, LV_EVENT_CLICKED, NULL);
    data.num = 0;

    mode = MODE_NORMAL;
    ops = _ops;
}

v_pen_ops_t *v_pen_getops(void)
{
    return ops;
}

v_stroke_data_t *all_strokes(void)
{
    return &data;
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