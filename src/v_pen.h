#ifndef __V_PEN_H__
#define __V_PEN_H__

#include <stddef.h>
#include <stdint.h>
#include <cjson/cJSON.h>
#include "v_common.h"
#include "v_color.h"

typedef enum
{
    V_PEN_DRAW_START,
    V_PEN_DRAW_ERASE_START,
    V_PEN_DRAW_MOVE,
    V_PEN_DRAW_END,
} v_pen_draw_mode_t;

typedef uint8_t v_pen_shape_kind_t;
enum
{
    V_PEN_SHAPE_ROUND,
    V_PEN_SHAPE_OVAL,
    V_PEN_SHAPE_SQUARE,
    V_PEN_SHAPE_MAX,
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
} v_inklist_t;
typedef struct str_v_pen_cb_ops
{

    void (*on_mode_change)(v_mode_t);
} v_pen_cb_ops_t;

typedef struct str_v_pen_ops
{
    v_status_t (*init)(lv_obj_t *parent, v_pen_cb_ops_t *ops);
    void (*draw)(float x, float y, float pressure, v_pen_draw_mode_t mode);
    void (*show_icon)(void);
    void (*hide_icon)(void);
} v_pen_ops_t;

v_pen_ops_t *v_pen_getops(void);

cJSON *pen_to_json(v_pen_t *pen);
v_pen_t *json_to_pen(cJSON *json);
#endif