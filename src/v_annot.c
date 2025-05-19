#include <stdlib.h>
#include "v_common.h"
#include "v_misc.h"
#include "v_annot.h"

void (*pdf_obj_free_func)(void *);

v_annot_t *v_create_annot(v_annot_kind_t kind)
{
    v_annot_t *ret = NULL;
    v_annot_t *a;
    ERR_RETn(kind >= V_ANNOT_MAX);

    a = malloc(sizeof(v_annot_t));
    ERR_RET(!a, "malloc");

    a->entry.tqe_next = NULL;
    a->entry.tqe_prev = NULL;
    a->kind = kind;
    a->pdf_annot_obj = NULL;
    if (a->kind == V_ANNOT_INKLIST)
    {
        void *tmp = malloc(sizeof(v_inklist_t));
        if (!tmp)
        {
            free(a);
            ERR_RET(!tmp, "malloc");
        }
        a->data.inklist = tmp;
        a->data.inklist->num = 0;
        a->data.inklist->strokes = NULL;
        a->data.inklist->refcnt = 0;
    }
    else if (a->kind == V_ANNOT_FREETEXT)
    {
        void *tmp = malloc(sizeof(v_freetext_t));
        if (!tmp)
        {
            free(a);
            ERR_RET(!tmp, "malloc");
        }
        a->data.freetext = tmp;
        a->data.freetext->refcnt = 1;
    }
    a->matrix = (v_matrix_t){.elm = {{1.0f, 0.0f, 0.0f},
                                     {0.0f, 1.0f, 0.0f}}};
    a->id = generate_id();
    ret = a;
error_return:
    return ret;
}

void v_annot_set_free_func(void (*func)(void *))
{
    pdf_obj_free_func = func;
}

v_annot_t *v_clone_annot(v_annot_t *orig)
{
    v_annot_t *ret = NULL;
    v_annot_t *clone = malloc(sizeof(v_annot_t));
    ERR_RET(!clone, "malloc");
    memcpy(clone, orig, sizeof(v_annot_t));
    clone->data.inklist->refcnt++;
    ret = clone;
error_return:
    return ret;
}

void v_free_annot(v_annot_t *annot)
{
    ERR_RETn(!annot);

    d("%p from:%p", annot, __builtin_return_address(0));
    if (annot->kind == V_ANNOT_INKLIST)
    {
        v_inklist_t *il = annot->data.inklist;
        il->refcnt--;
        ERR_RETn(il->refcnt);
        ERR_RETn(!il->strokes);
        for (int i = 0; i < il->num; i++)
        {
            v_stroke_t *st = &il->strokes[i];
            for (int j = 0; j < st->num; j++)
            {
                if (!st->points)
                    continue;

                free(st->points);
                st->points = NULL;
            }
            free(il->strokes);
            il->strokes = NULL;
        }
        if (annot->pdf_annot_obj)
        {
            pdf_obj_free_func(annot->pdf_annot_obj);
        }
        free(annot->data.inklist);
        annot->data.inklist = NULL;
    }
    else if (annot->kind == V_ANNOT_FREETEXT)
    {
        v_freetext_t *ft = annot->data.freetext;
        ft->refcnt--;
        ERR_RETn(ft->refcnt);
        if (ft->content)
            free(ft->content);
        free(ft);
        annot->data.freetext = NULL;
        if (annot->pdf_annot_obj)
        {
            pdf_obj_free_func(annot->pdf_annot_obj);
        }
    }

error_return:
    if (annot)
        free(annot);
    return;
}