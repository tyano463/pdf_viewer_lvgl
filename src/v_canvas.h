#ifndef __V_CANVAS_H__
#define __V_CANVAS_H__

#include <sys/queue.h>
#include <lvgl/lvgl.h>

#include "v_common.h"
#include "v_pen.h"

typedef uint8_t v_draw_kind_t;
enum
{
    V_DRAW_KIND_ANNOT,
    V_DRAW_KIND_ERASE,
    V_DRAW_KIND_SHOW_ALL,
    V_DRAW_KIND_HIDE_ALL,
    V_DRAW_KIND_USER,
    V_DRAW_KIND_MAX,
};

typedef struct str_v_draw_event
{
    TAILQ_ENTRY(str_v_draw_event)
    entry;
    v_draw_kind_t kind;
    void *arg;
    void (*user_callback)(void *);
} v_draw_event_t;

typedef struct str_v_image
{
    uint8_t *buf;
    uint16_t width;
    uint16_t height;
    uint32_t size;
    lv_color_format_t format;
} v_image_t;

typedef uint8_t v_annot_kind_t;
enum
{
    V_ANNOT_INKLIST,
    V_ANNOT_FREETEXT,
    V_ANNOT_MAX,
};

typedef struct str_v_freetext
{

    const char *content;
    v_color_t color;
    uint8_t font_size;
    char *font_name;
    v_rect_t position;
} v_freetext_t;

typedef struct str_v_annot
{
    TAILQ_ENTRY(str_v_annot)
    entry;
    void *pdf_annot_obj;
    v_matrix_t matrix;
    v_annot_kind_t kind;
    v_rect_t rect;
    union
    {
        v_freetext_t freetext;
        v_inklist_t inklist;
    } data;
} v_annot_t;

typedef struct str_v_annnots
{
    uint32_t num;
    v_annot_t annot[];
} v_annots_t;

typedef struct str_v_annot_control
{
    v_annot_t *annot;
    lv_obj_t *remove_button;
    lv_obj_t *resize_button;
} v_annot_control_t;

typedef struct str_v_draw_ops
{
    v_status_t (*init)(void);
    v_status_t (*open)(const char *path);
    int (*pagenum)(void);
    v_status_t (*size)(int *width, int *height);
    v_status_t (*pixel)(uint8_t *data, int page, int rowstride, v_scale_t ctm);
    void (*free)(void);
    v_annots_t *(*annots)(void);
    void (*save)(const char *path, v_annots_t *);
    const char *(*path)(void);
} v_draw_ops_t;

typedef struct str_v_viewer_ops
{
    v_status_t (*init)(lv_obj_t *parent);
    void (*add_annot)(v_annot_t *annot);
    void (*remove_annot)(v_annot_t *annot);
    void (*set_mode)(v_show_mode_t mode);
    v_status_t (*show_image)(v_image_t *image);
    void (*set_touch_callback)(lv_event_cb_t cb);
    void (*show_annot)(void);
    void (*hide_annot)(void);
    void (*queue)(v_draw_event_t *ev);
    void (*select)(lv_point_t *);
    void (*move)(const lv_point_t *, const lv_point_t *);
    v_annots_t *(*annots)(void);
    void (*matrix)(v_matrix_t *m);
} v_viewer_ops_t;

v_viewer_ops_t *v_get_canvas_ops(void);

#endif