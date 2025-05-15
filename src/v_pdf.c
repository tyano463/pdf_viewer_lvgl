#include <ctype.h>
#include <mupdf/pdf/object.h>
#include "v_common.h"
#include "v_pdf.h"
#include "v_misc.h"
#include "v_core.h"

#include "uthash.h"

typedef struct
{
    pdf_annot *key;
    v_annot_t *value;
    UT_hash_handle hh;
} annot_map_t;

static void init_ops(void);
static v_status_t v_pdf_init(void);
static v_status_t v_pdf_open(const char *path);
static int v_pdf_pagecount(void);
static v_status_t v_pdf_getsize(int *width, int *height);
static v_status_t v_pdf_alloc_pixel_data(uint8_t *data, int page, int rowstride, v_scale_t scale);
static void v_pdf_release_pixel_data(void);
static v_annots_t *v_pdf_get_annots(void);
static void v_pdf_release(void);
static void v_pdf_save(const char *, v_annots_t *annots);
static const char *v_pdf_path(void);

static v_pdf_t *pdf;
static fz_pixmap *pix;
static v_draw_ops_t g_ops;

extern char g_current_path[MAX_PATH];

static void init_ops(void)
{
    g_ops.init = v_pdf_init;
    g_ops.open = v_pdf_open;
    g_ops.pagenum = v_pdf_pagecount;
    g_ops.size = v_pdf_getsize;
    g_ops.pixel = v_pdf_alloc_pixel_data;
    g_ops.annots = v_pdf_get_annots;
    g_ops.free = v_pdf_release;
    g_ops.save = v_pdf_save;
    g_ops.path = v_pdf_path;
}

static const char *v_pdf_path(void)
{
    return g_current_path;
}

v_draw_ops_t *v_pdf_get_ops(void)
{
    if (!g_ops.init)
    {
        v_pdf_init();
    }
    return &g_ops;
}

static v_status_t v_pdf_init(void)
{
    v_status_t status;

    status = ST_PDF_OPEN_FAILED;
    if (!pdf)
        pdf = calloc(sizeof(v_pdf_t), 1);
    ERR_RETn(!pdf);

    status = ST_PDF_CONTEXT_CREAT_FAILED;
    pdf->ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
    ERR_RETn(!pdf->ctx);

    fz_register_document_handlers(pdf->ctx);

    init_ops();

    status = ST_SUCCESS;
error_return:
    return status;
}

static v_status_t v_pdf_loadpage(int page)
{
    v_status_t status = ST_PDF_OPEN_FAILED;
    ERR_RET(!pdf, "pdf is null");

    page = page - 1;
    page = max(page, 0);
    if (pdf->page)
    {
        if (pdf->page->number == page)
        {
            status = ST_SUCCESS;
            goto error_return;
        }
        else
        {
            v_pdf_release_pixel_data();
            // fz_drop_document(pdf->ctx, pdf->doc);
        }
    }
    pdf->page = fz_load_page(pdf->ctx, pdf->doc, page);
    fz_rect bounds = fz_bound_page(pdf->ctx, pdf->page);
    pdf->width = bounds.x1 - bounds.x0;
    pdf->height = bounds.y1 - bounds.y0;

    status = ST_SUCCESS;
error_return:
    return status;
}
static v_status_t v_pdf_open(const char *path)
{
    v_status_t status = ST_SUCCESS;
    ERR_RET(path == g_current_path || strcmp(path, g_current_path) == 0, "page change only");

    sprintf(g_current_path, "%s", path);
    fz_try(pdf->ctx)
        pdf->doc = fz_open_document(pdf->ctx, path);
    fz_catch(pdf->ctx)
    {
        fz_drop_context(pdf->ctx);
        status = ST_PDF_DOCUMENT_OPEN_FAILED;
        goto error_return;
    }

    pdf->page_num = fz_count_pages(pdf->ctx, pdf->doc);
    pdf->changed = false;

    // Load the first page
    status = v_pdf_loadpage(1);

error_return:
    return status;
}

static int v_pdf_pagecount(void)
{
    int pagenum = 0;

    ERR_RETn(!pdf);
    pagenum = pdf->page_num;

error_return:
    return pagenum;
}

static v_status_t v_pdf_getsize(int *width, int *height)
{
    v_status_t status = ST_PDF_OPEN_FAILED;

    ERR_RETn(!pdf);

    *width = pdf->width;
    *height = pdf->height;
    status = ST_SUCCESS;

error_return:
    return status;
}

static void _newPage(fz_context *ctx, pdf_document *pdf, int pno, float width, float height)
{
    fz_rect mediabox = fz_unit_rect;
    mediabox.x1 = width;
    mediabox.y1 = height;
    pdf_obj *resources = NULL, *page_obj = NULL;
    fz_buffer *contents = NULL;
    fz_var(contents);
    fz_var(page_obj);
    fz_var(resources);
    fz_try(ctx)
    {
        if (pno < -1)
        {
            return;
        }
        // create /Resources and /Contents objects
        resources = pdf_add_new_dict(ctx, pdf, 1);
        page_obj = pdf_add_page(ctx, pdf, mediabox, 0, resources, contents);
        pdf_insert_page(ctx, pdf, pno, page_obj);
    }
    fz_always(ctx)
    {
        fz_drop_buffer(ctx, contents);
        pdf_drop_obj(ctx, page_obj);
        pdf_drop_obj(ctx, resources);
    }
    fz_catch(ctx)
    {
    }
}

static int get_annot_num(void)
{
    int n = -1;
    pdf_annot *annot;
    ERR_RET(!pdf || !pdf->ctx || !pdf->page, "no load pdf");

    annot = pdf_first_annot(pdf->ctx, (pdf_page *)pdf->page);
    n = 0;
    while (annot)
    {
        n++;
        annot = pdf_next_annot(pdf->ctx, annot);
    }
error_return:
    return n;
}

static void parse_da(const char *s, v_pdf_da_t *d)
{
    if (!s || !d)
        return;

    d->color = argb2vcolor(1, 0, 0, 0);

    const char *p = s;
    while (*p)
    {
        if (*p == '/')
        {
            p++;
            const char *start = p;
            while (*p && !isspace(*p))
                p++;
            size_t len = p - start;
            if (len > 0)
            {
                char *fontname = (char *)malloc(len + 1);
                if (!fontname)
                    return;
                memcpy(fontname, start, len);
                fontname[len] = '\0';
                d->fontname = fontname;
            }
        }
        // Tf がきたらフォントサイズ
        else if (p[0] == 'T' && p[1] == 'f')
        {
            // フォントサイズはTfの直前にある
            const char *q = p - 1;
            while (q > s && isspace(*q))
                q--;

            // 数字の終わりを探したので、逆に数字を読む
            const char *num_end = q + 1;
            while (q > s && (isdigit(*q) || *q == '.' || *q == '-'))
                q--;

            if (q != num_end)
            {
                float fontsize = atof(q + 1);
                if (fontsize > 0 && fontsize < 255)
                    d->fontsize = (uint8_t)fontsize;
            }

            p += 2;
        }
        // rg がきたらカラー
        else if (p[0] == 'r' && p[1] == 'g')
        {
            // rgの直前に3つの数字
            const char *q = p - 1;
            while (q > s && isspace(*q))
                q--;

            // qから左へ数字を3個読む
            float vals[3] = {0};
            int found = 0;
            for (int i = 2; i >= 0; i--)
            {
                while (q > s && (isdigit(*q) || *q == '.' || *q == '-'))
                    q--;
                vals[i] = atof(q + 1);

                // 次に前の数字を探す
                while (q > s && isspace(*q))
                    q--;
                found++;
            }

            if (found == 3)
            {
                d->color = argb2vcolor(1.0f, vals[0], vals[1], vals[2]);
            }

            p += 2;
        }
        else
        {
            p++;
        }
    }
}

static v_annots_t *v_pdf_get_annots(void)
{
    int n;
    v_annots_t *ret = NULL;
    v_annots_t *annots;

    n = get_annot_num();
    ERR_RETn(n <= 0);

    annots = (v_annots_t *)malloc(sizeof(v_annots_t) + sizeof(v_annot_t) * n);
    ERR_RET(!annots, "malloc");

    annots->num = n;

    annots->annot[0].pdf_annot_obj = pdf_first_annot(pdf->ctx, (pdf_page *)pdf->page);

    for (int i = 1; i < n; i++)
    {
        annots->annot[i].pdf_annot_obj = pdf_next_annot(pdf->ctx, annots->annot[i - 1].pdf_annot_obj);
    }

    for (int i = 0; i < n; i++)
    {
        v_annot_t *a = &annots->annot[i];
        a->matrix = (v_matrix_t){.elm = {{1.0f, 0.0f, 0.0f},
                                         {0.0f, 1.0f, 0.0f}}};
        a->id = generate_id();
        enum pdf_annot_type t = pdf_annot_type(pdf->ctx, a->pdf_annot_obj);
        switch (t)
        {
        case PDF_ANNOT_INK:
        {
            pdf_obj *obj = pdf_annot_obj(pdf->ctx, a->pdf_annot_obj);
            if (!obj)
                continue;

            a->kind = V_ANNOT_INKLIST;

            pdf_obj *color = pdf_dict_get(pdf->ctx, obj, PDF_NAME(C));
            float r = pdf_to_real(pdf->ctx, pdf_array_get(pdf->ctx, color, 0));
            float g = pdf_to_real(pdf->ctx, pdf_array_get(pdf->ctx, color, 1));
            float b = pdf_to_real(pdf->ctx, pdf_array_get(pdf->ctx, color, 2));

            pdf_obj *ca = pdf_dict_get(pdf->ctx, obj, PDF_NAME(CA));
            float alpha = 1;
            if (ca)
                alpha = pdf_to_real(pdf->ctx, ca);

            d("alpha: %.02f", alpha);
            a->data.inklist.pen.color = argb2vcolor(alpha, r, g, b);

            pdf_obj *bs = pdf_dict_get(pdf->ctx, obj, PDF_NAME(BS));
            float w = pdf_to_real(pdf->ctx, pdf_dict_get(pdf->ctx, bs, PDF_NAME(W)));
            a->data.inklist.pen.size = (int)w;

            pdf_obj *inklist = pdf_dict_get(pdf->ctx, obj, PDF_NAME(InkList));
            a->data.inklist.num = pdf_array_len(pdf->ctx, inklist);
            a->data.inklist.strokes = malloc(sizeof(v_stroke_t) * a->data.inklist.num);
            a->data.inklist.coord_type = V_ANNOT_COORD_ORIGINAL;
            a->matrix = (v_matrix_t){.elm = {{1.0f, 0.0f, 0.0f},
                                             {0.0f, -1.0f, pdf->height}}};
            for (int j = 0; j < pdf_array_len(pdf->ctx, inklist); j++)
            {
                v_stroke_t *s = &a->data.inklist.strokes[j];
                pdf_obj *stroke = pdf_array_get(pdf->ctx, inklist, j);
                int sn = pdf_array_len(pdf->ctx, stroke);
                s->max = sn / 2;
                s->num = sn / 2;
                s->points = malloc(sizeof(v_point_t) * s->max);
                for (int k = 0; k < sn / 2; k++)
                {
                    s->points[k].x = pdf_to_real(pdf->ctx, pdf_array_get(pdf->ctx, stroke, k * 2));
                    s->points[k].y = pdf_to_real(pdf->ctx, pdf_array_get(pdf->ctx, stroke, k * 2 + 1));
                }
            }
        }
        break;
        case PDF_ANNOT_FREE_TEXT:
        {
            pdf_obj *obj = pdf_annot_obj(pdf->ctx, a->pdf_annot_obj);
            if (!obj)
                continue;
            a->kind = V_ANNOT_FREETEXT;

            pdf_obj *contents = pdf_dict_get(pdf->ctx, obj, PDF_NAME(Contents));
            if (contents)
            {
                const char *text = pdf_to_text_string(pdf->ctx, contents);
                if (text)
                {
                    a->data.freetext.content = strdup(text);
                    d("t: %s", a->data.freetext.content);
                }
            }

            pdf_obj *rect = pdf_dict_get(pdf->ctx, obj, PDF_NAME(Rect));
            if (rect && pdf_is_array(pdf->ctx, rect) && pdf_array_len(pdf->ctx, rect) == 4)
            {
                float x0 = pdf_to_real(pdf->ctx, pdf_array_get(pdf->ctx, rect, 0));
                float y0 = pdf_to_real(pdf->ctx, pdf_array_get(pdf->ctx, rect, 1));
                float x1 = pdf_to_real(pdf->ctx, pdf_array_get(pdf->ctx, rect, 2));
                float y1 = pdf_to_real(pdf->ctx, pdf_array_get(pdf->ctx, rect, 3));

                a->data.freetext.position.left = x0;
                a->data.freetext.position.top = pdf->height - y1;
                a->data.freetext.position.right = x1;
                a->data.freetext.position.bottom = pdf->height - y0;
                d("pos: %p %.0f,%.0f,%.0f,%.0f", &a->data.freetext.position, a->data.freetext.position.left, a->data.freetext.position.top, a->data.freetext.position.right, a->data.freetext.position.bottom);
            }

            pdf_obj *da = pdf_dict_get(pdf->ctx, obj, PDF_NAME(DA));
            if (!da)
                continue;

            const char *da_str = pdf_to_text_string(pdf->ctx, da);
            v_pdf_da_t dat = {0};
            parse_da(da_str, &dat);
            a->data.freetext.color = dat.color;
            a->data.freetext.font_name = dat.fontname;
            a->data.freetext.font_size = dat.fontsize;
            d("%s(%d)", a->data.freetext.font_name, a->data.freetext.font_size);
        }
        break;
        default:
            break;
        }
    }

    ret = annots;
error_return:
    return ret;
}

static v_status_t v_pdf_alloc_pixel_data(uint8_t *data, int page, int rowstride, v_scale_t scale)
{
    int i, w, h;
    v_status_t status = ST_PDF_OPEN_FAILED;
    fz_matrix ctm;
    fz_colorspace *cs;

    ERR_RET(!pdf || !pdf->ctx || !pdf->page, "pdf not load");

    ctm = fz_scale(scale.sx, scale.sy);
    cs = fz_device_rgb(pdf->ctx);

    v_pdf_release_pixel_data();
    v_pdf_loadpage(page);
    // exclude annotation
    pix = fz_new_pixmap_from_page_contents(pdf->ctx, pdf->page, ctm, cs, 0);
    // include annotation
    //    pix = fz_new_pixmap_from_page_number(pdf->ctx, pdf->doc, page, ctm, cs, 0);

    d("%d, %d str:%d", pix->w, pix->h, pix->stride);
    w = min(pix->w, pdf->width);
    h = min(pix->h, pdf->height);

    //    FILE *fp = fopen("/tmp/data.bin", "wb");
    //    fwrite(pix->samples, pix->stride * pix->h, 1, fp);
    //    fclose(fp);

    for (i = 0; i < h; i++)
    {
        uint8_t *s = &pix->samples[i * pix->stride];
        for (int j = 0; j < w; j++)
        {
            uint8_t *p = data + i * rowstride + j * 4;
            p[0] = s[2];
            p[1] = s[1];
            p[2] = s[0];
            p[3] = 0xff;
            s += pix->n;
        }
    }

    status = ST_SUCCESS;
error_return:
    return status;
}

static void v_pdf_release_pixel_data(void)
{
    if (pdf && pix)
    {
        fz_drop_pixmap(pdf->ctx, pix);
        pix = NULL;
    }
}

static bool apply_annotation(v_annot_t *a, v_matrix_t *inv)
{
    bool ret = false;
    pdf_annot *annot = NULL;
    v_inklist_t *inklist = &a->data.inklist;

    ERR_RETn(!inklist);
    ERR_RETn(!inklist->num);

    annot = pdf_create_annot(pdf->ctx, (pdf_page *)pdf->page, PDF_ANNOT_INK);
    ERR_RET(!annot, "annotation create failed");

    {
        float color[3] = {
            inklist->pen.color.c.red / 255.0f,
            inklist->pen.color.c.green / 255.0f,
            inklist->pen.color.c.blue / 255.0f,
        };
        pdf_set_annot_color(pdf->ctx, annot, 3, color);
        d("color:%.02f,%.02f,%.02f", color[0], color[1], color[2]);
    }

    {
        float width = (float)inklist->pen.size;
        pdf_set_annot_border(pdf->ctx, annot, width);
        d("w:%.02f", width);
    }

    pdf_obj *obj = pdf_annot_obj(pdf->ctx, annot);
    pdf_dict_puts(pdf->ctx, obj, "CA", pdf_new_real(pdf->ctx, inklist->pen.color.c.alpha / 255.0f));

    pdf_obj *list = pdf_new_array(pdf->ctx, (pdf_document *)pdf->doc, inklist->num);
    d("inklist num:%d", inklist->num);
    for (int i = 0; i < inklist->num; i++)
    {
        v_stroke_t *st = &inklist->strokes[i];
        pdf_obj *stroke = pdf_new_array(pdf->ctx, (pdf_document *)pdf->doc, st->num);

        d("stroke num:%d", st->num);
        for (int j = 0; j < st->num; j++)
        {
            float x = st->points[j].x;
            float y = st->points[j].y;

            x = inv->elm[0][0] * x + inv->elm[0][1] * y + inv->elm[0][2];
            y = inv->elm[1][0] * x + inv->elm[1][1] * y + inv->elm[1][2];
            pdf_array_push(pdf->ctx, stroke, pdf_new_real(pdf->ctx, x));
            pdf_array_push(pdf->ctx, stroke, pdf_new_real(pdf->ctx, y));
        }
        pdf_array_push(pdf->ctx, list, stroke);
    }
    pdf_dict_puts(pdf->ctx, obj, "InkList", list);

    pdf_add_annot_ink_list_stroke(pdf->ctx, annot);
    pdf_update_annot(pdf->ctx, annot);

    ret = true;
error_return:
    return ret;
}

static void annots_to_document(v_annots_t *annots)
{
    pdf_annot *annot;
    pdf_annot **delete_candidates = NULL;
    int delete_num = 0;
    annot_map_t *annot_map = NULL;

    ERR_RETn(!annots);

    v_viewer_ops_t *ops = v_get_canvas_ops();
    v_matrix_t m, inv;
    ops->matrix(&m);
    bool invertible = inverse_matrix(&m, &inv);

    for (int i = 0; i < annots->num; i++)
    {
        v_annot_t *a = &annots->annot[i];
        if (a->kind == V_ANNOT_COORD_NEW)
            continue;

        annot_map_t *entry = malloc(sizeof(annot_map_t));
        ERR_RET(!entry, "malloc failed");
        entry->key = a->pdf_annot_obj;
        entry->value = a;
        HASH_ADD_PTR(annot_map, key, entry);
    }

    annot = pdf_first_annot(pdf->ctx, (pdf_page *)pdf->page);
    while (annot)
    {
        annot_map_t *entry;
        HASH_FIND_PTR(annot_map, &annot, entry);
        if (entry)
        {
            if (invertible && entry->value->kind == V_ANNOT_COORD_MODIFIED)
            {
                // 修正処理
                d("modified found");
                pdf_obj *obj = pdf_annot_obj(pdf->ctx, annot);
                if (!obj)
                    continue;

                pdf_obj *inklist = pdf_dict_get(pdf->ctx, obj, PDF_NAME(InkList));
                int n = pdf_array_len(pdf->ctx, inklist);
                if (entry->value->data.inklist.num != n)
                    continue;

                for (int i = 0; i < n; i++)
                {
                    pdf_obj *stroke = pdf_array_get(pdf->ctx, inklist, i);
                    int sn = pdf_array_len(pdf->ctx, stroke);
                    for (int j = 0; j < (sn / 2); j++)
                    {
                        lv_point_precise_t point, after;
                        point.x = pdf_to_real(pdf->ctx, pdf_array_get(pdf->ctx, stroke, j * 2));
                        point.y = pdf_to_real(pdf->ctx, pdf_array_get(pdf->ctx, stroke, j * 2 + 1));

                        after.x = inv.elm[0][0] * point.x +
                                  inv.elm[0][1] * point.y +
                                  inv.elm[0][2];
                        after.y = inv.elm[1][0] * point.x +
                                  inv.elm[1][1] * point.y +
                                  inv.elm[1][2];
                        pdf_array_put_real(pdf->ctx, stroke, j * 2, after.x);
                        pdf_array_put_real(pdf->ctx, stroke, j * 2 + 1, after.y);
                    }
                }
                pdf_update_annot(pdf->ctx, annot);
            }
        }
        else
        {
            void *tmp = realloc(delete_candidates, sizeof(pdf_annot *) * (delete_num + 1));
            ERR_RET(!tmp, "realloc");
            delete_candidates = tmp;
            delete_candidates[delete_num++] = annot;
        }
        annot = pdf_next_annot(pdf->ctx, annot);
    }

    // 削除処理
    for (int i = 0; i < delete_num; i++)
    {
        d("delete found");
        annot = delete_candidates[i];
        pdf_delete_annot(pdf->ctx, (pdf_page *)pdf->page, annot);
    }

    // 追加処理
    for (int i = 0; i < annots->num; i++)
    {
        v_annot_t *a = &annots->annot[i];
        if (a->data.inklist.coord_type != V_ANNOT_COORD_NEW)
            continue;
        d("new annot found");
        apply_annotation(a, &inv);
    }

error_return:
    if (delete_candidates)
        free(delete_candidates);
    return;
}

static void v_pdf_save(const char *path, v_annots_t *annots)
{
    d("IN");
    ERR_RET(!path, "path is null");
    ERR_RET(!pdf || !pdf->ctx, "ctx is null");

    char *dirname = get_dir_name(path);
    ERR_RET(!dirname, "unknown path: %s", path);

    if (!directory_exists(dirname))
    {
        mkdir_p(dirname, 0666);
    }
    free(dirname);

    annots_to_document(annots);

    pdf_save_document(pdf->ctx, (pdf_document *)pdf->doc, path, NULL);

    d("OUT");
error_return:
    return;
}

static void v_pdf_reset_annot(void)
{
}

static void v_pdf_release(void)
{
    ERR_RETn(!pdf);
    ERR_RETn(!pdf->page);

    v_pdf_release_pixel_data();
    fz_drop_page(pdf->ctx, pdf->page);
    pdf->page = NULL;
    fz_drop_document(pdf->ctx, pdf->doc);
    pdf->doc = NULL;
    fz_drop_context(pdf->ctx);
    pdf->ctx = NULL;

error_return:
    return;
}