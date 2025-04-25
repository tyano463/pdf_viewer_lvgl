#ifndef __V_CANVAS_H__
#define __V_CANVAS_H__

#include "v_common.h"
#include "v_pen.h"

typedef struct str_v_image
{
    uint8_t *buf;
    uint16_t width;
    uint16_t height;
    uint32_t size;
    lv_color_format_t format;
} v_image_t;

typedef struct str_v_draw_ops
{
    v_status_t (*init)(void);
    v_status_t (*open)(const char *path);
    int (*pagenum)(void);
    v_status_t (*size)(int *width, int *height);
    v_status_t (*pixel)(uint8_t *data, int page, int rowstride, v_scale_t ctm);
    void (*free)(void);
    v_status_t (*annots)(void);
} v_draw_ops_t;

typedef uint8_t v_annot_kind_t;
enum
{
    V_ANNOT_INKLIST,
    V_ANNOT_FREETEXT,
    V_ANNOT_MAX,
};

typedef struct str_v_annot
{
    void *pdf_annot_obj;
    v_annot_kind_t kind;
    union
    {
        struct
        {
            const char *content;
            uint8_t font_size;
            const char *font_name;
            v_rect_t position;
        } freetext;
        v_stroke_t stroke;
    } data;
} v_annot_t;

typedef struct str_v_viewer_ops
{
    v_status_t (*init)(lv_obj_t *parent);
    void (*add_annot)(v_annot_t *annot);
    void (*show_frame)(v_annot_t *annot);
    void (*remove_annot)(v_annot_t *annot);
    void (*set_mode)(v_show_mode_t mode);
    v_status_t (*show_image)(v_image_t *image);
    void (*set_touch_callback)(lv_event_cb_t cb);
} v_viewer_ops_t;

v_viewer_ops_t *v_get_canvas_ops(void);

#endif