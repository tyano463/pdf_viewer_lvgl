#ifndef __V_ANNOT_H__
#define __V_ANNOT_H__

#include <sys/queue.h>
#include "v_color.h"
#include "v_pen.h"

typedef uint8_t v_annot_kind_t;
enum
{
    V_ANNOT_INKLIST,
    V_ANNOT_FREETEXT,
    V_ANNOT_MAX,
};

typedef struct str_v_freetext
{
    char *content;
    v_color_t color;
    uint8_t font_size;
    uint8_t refcnt;
    char *font_name;
    v_rect_t position;
} v_freetext_t;

typedef struct str_v_annot
{
    TAILQ_ENTRY(str_v_annot)
    entry;
    uint64_t id;
    void *pdf_annot_obj;
    v_matrix_t matrix;
    v_annot_kind_t kind;
    v_rect_t rect;
    union
    {
        v_freetext_t *freetext;
        v_inklist_t *inklist;
    } data;
} v_annot_t;

v_annot_t *v_create_annot(v_annot_kind_t);
v_annot_t *v_clone_annot(v_annot_t *orig);
void v_free_annot(v_annot_t *);
void v_annot_set_free_func(void (*func)(void *));
#endif