#ifndef __V_PEN_H__
#define __V_PEN_H__

#include <stddef.h>
#include <stdint.h>
#include <cjson/cJSON.h>
#include "lvgl/lvgl.h"
#include "v_common.h"
#include "v_color.h"

#define PEN_SIZE_MIN 1
#define PEN_SIZE_MAX 24

#define DRAW_MODE_ITEMS(X)    \
    X(V_PEN_DRAW_START)       \
    X(V_PEN_DRAW_ERASE_START) \
    X(V_PEN_DRAW_MOVE)        \
    X(V_PEN_DRAW_END)         \
    X(V_PEN_DRAW_MAX)

DEFINE_ENUM_WITH_STRINGS(v_pen_draw_mode_t, DRAW_MODE_ITEMS);

typedef uint8_t v_pen_shape_kind_t;
enum
{
    V_PEN_SHAPE_ROUND,
    V_PEN_SHAPE_OVAL,
    V_PEN_SHAPE_SQUARE,
    V_PEN_SHAPE_MAX,
};

typedef uint8_t v_annot_coord_type_t;
enum
{
    V_ANNOT_COORD_TYPE_FILE,
    V_ANNOT_COORD_TYPE_SCREEEN,
};

typedef struct str_v_pen_shape
{
    v_pen_shape_kind_t kind;
} v_pen_shape_t;

typedef struct str_v_pen
{
    v_color_t color;
    v_pen_shape_t shape;
    uint8_t size;
} v_pen_t;

typedef struct str_v_point
{
    float x;
    float y;
    float pressure;
} v_point_t;

typedef struct str_v_stroke
{
    v_point_t *points;
    uint16_t num;
    uint16_t max;
} v_stroke_t;

typedef struct str_v_inklist
{
    v_pen_t pen;
    v_stroke_t *strokes;
    uint16_t num;
    v_annot_coord_type_t coord_type;
} v_inklist_t;
typedef struct str_v_pen_cb_ops
{

    void (*on_user_mode_change)(v_user_mode_t);
} v_pen_cb_ops_t;

typedef struct str_v_pen_ops
{
    v_status_t (*init)(lv_obj_t *parent, v_pen_cb_ops_t *ops);
    void (*draw)(int32_t x, int32_t y, uint8_t pressure, v_pen_draw_mode_t mode);
    void (*select)(float x, float y, v_pen_draw_mode_t mode);
    void (*show_icon)(void);
    void (*hide_icon)(void);
} v_pen_ops_t;

v_pen_ops_t *v_pen_getops(void);

cJSON *pen_to_json(v_pen_t *pen);
v_pen_t *json_to_pen(cJSON *json);
#endif