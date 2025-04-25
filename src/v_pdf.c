#include "v_common.h"
#include "v_pdf.h"
#include "v_misc.h"

static void init_ops(void);
static v_status_t v_pdf_init(void);
static v_status_t v_pdf_open(const char *path);
static int v_pdf_pagecount(void);
static v_status_t v_pdf_getsize(int *width, int *height);
static v_status_t v_pdf_alloc_pixel_data(uint8_t *data, int page, int rowstride, v_scale_t scale);
static void v_pdf_release_pixel_data(void);
static v_status_t v_pdf_get_annots(void);
static void add_ink_annot_sample(const char *);
static void v_pdf_release(void);

static v_pdf_t *pdf;
static fz_pixmap *pix;
static v_draw_ops_t ops;
static pdf_annot **annots;

static void init_ops(void)
{
    ops.init = v_pdf_init;
    ops.open = v_pdf_open;
    ops.pagenum = v_pdf_pagecount;
    ops.size = v_pdf_getsize;
    ops.pixel = v_pdf_alloc_pixel_data;
    ops.annots = v_pdf_get_annots;
    ops.free = v_pdf_release;
}

v_draw_ops_t *v_pdf_get_ops(void)
{
    if (!ops.init)
    {
        v_pdf_init();
    }
    return &ops;
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
            fz_drop_document(pdf->ctx, pdf->doc);
        }
    }
    pdf->page = fz_load_page(pdf->ctx, pdf->doc, 0);
    fz_rect bounds = fz_bound_page(pdf->ctx, pdf->page);
    pdf->width = bounds.x1 - bounds.x0;
    pdf->height = bounds.y1 - bounds.y0;

    status = ST_SUCCESS;
error_return:
    return status;
}
static v_status_t v_pdf_open(const char *path)
{
    v_status_t status;

    // add_ink_annot_sample(path);

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
    status = v_pdf_loadpage(0);

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

static uint32_t annot_num(void)
{
    uint32_t n = 0;
    ERR_RETn(!pdf);
    ERR_RETn(!pdf->ctx | !pdf->page);

    pdf_annot *annot = pdf_first_annot(pdf->ctx, (pdf_page *)pdf->page);
    while (annot)
    {
        annot = pdf_next_annot(pdf->ctx, annot);
        n++;
    }

error_return:
    return n;
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

static void add_ink_annot_sample(const char *input_pdf)
{
    const char *output_pdf = next_file_name(input_pdf);
    fz_context *ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);

    fz_try(ctx)
    {
        pdf_document *doc = pdf_open_document(ctx, input_pdf);
        fz_page *page = fz_load_page(ctx, (fz_document *)doc, 0);
        pdf_page *pdfpage = (pdf_page *)page;

        fz_point points[3] = {
            {100, 500},
            {150, 520},
            {180, 580}};

        pdf_annot *annot = pdf_create_annot(ctx, pdfpage, PDF_ANNOT_INK);

        fz_colorspace *cs = fz_device_rgb(ctx);
        float color[3] = {0.57f, 0.25f, 0.67f};
        pdf_set_annot_color(ctx, annot, 3, color);

        pdf_set_annot_border(ctx, annot, 2.0f);

        pdf_obj *obj = pdf_annot_obj(ctx, annot);
        pdf_dict_puts(ctx, obj, "CA", pdf_new_real(ctx, 0.5f));

        pdf_obj *inklist = pdf_new_array(ctx, doc, 1);
        pdf_obj *stroke = pdf_new_array(ctx, doc, 6);
        for (int i = 0; i < 3; i++)
        {
            pdf_array_push(ctx, stroke, pdf_new_real(ctx, points[i].x));
            pdf_array_push(ctx, stroke, pdf_new_real(ctx, points[i].y));
        }
        pdf_array_push(ctx, inklist, stroke);
        pdf_dict_puts(ctx, obj, "InkList", inklist);

        pdf_update_annot(ctx, annot);

        pdf_save_document(ctx, doc, output_pdf, NULL);

        fz_drop_page(ctx, page);
        fz_drop_document(ctx, &doc->super);
    }
    fz_catch(ctx)
    {
        d("Error: %s", fz_caught_message(ctx));
    }

    fz_drop_context(ctx);
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

static v_status_t v_pdf_get_annots(void)
{
    int i, n;
    v_status_t status = ST_PDF_ANNOTATION_FAILED;
    pdf_annot *annot;

    n = get_annot_num();
    ERR_RETn(n < 0);
    if (!n)
    {
        status = ST_SUCCESS;
        goto error_return;
    }

    if (annots)
        free(annots);
    annots = (pdf_annot **)malloc(sizeof(pdf_annot *) * n);
    ERR_RET(!annots, "malloc");

    annots[0] = pdf_first_annot(pdf->ctx, (pdf_page *)pdf->page);

    for (i = 1; i < n; i++)
    {
        annots[i] = pdf_next_annot(pdf->ctx, annots[i - 1]);
    }

    for (i = 0; i < n; i++)
    {
        if (pdf_annot_type(pdf->ctx, annots[i]) == PDF_ANNOT_INK)
        {
            pdf_obj *obj = pdf_annot_obj(pdf->ctx, annots[i]);
            if (!obj)
                continue;
            pdf_obj *color = pdf_dict_get(pdf->ctx, obj, PDF_NAME(C));
            float r = pdf_to_real(pdf->ctx, pdf_array_get(pdf->ctx, color, 0));
            float g = pdf_to_real(pdf->ctx, pdf_array_get(pdf->ctx, color, 1));
            float b = pdf_to_real(pdf->ctx, pdf_array_get(pdf->ctx, color, 2));
            d("r,g,b = %.02f,%.02f,%.02f", r, g, b);

            pdf_obj *bs = pdf_dict_get(pdf->ctx, obj, PDF_NAME(BS));
            float w = pdf_to_real(pdf->ctx, pdf_dict_get(pdf->ctx, bs, PDF_NAME(W)));
            d("w: %.02f", w);

            pdf_obj *inklist = pdf_dict_get(pdf->ctx, obj, PDF_NAME(InkList));
            for (int i = 0; i < pdf_array_len(pdf->ctx, inklist); i++)
            {
                pdf_obj *stroke = pdf_array_get(pdf->ctx, inklist, i);
                for (int j = 0; j < pdf_array_len(pdf->ctx, stroke) / 2; j++)
                {
                    float x = pdf_to_real(pdf->ctx, pdf_array_get(pdf->ctx, stroke, j * 2));
                    float y = pdf_to_real(pdf->ctx, pdf_array_get(pdf->ctx, stroke, j * 2 + 1));
                    //                    d("Stroke[%d] Point[%d]: (%f, %f)", i, j, x, y);
                }
            }
            pdf_obj *ca = pdf_dict_get(pdf->ctx, obj, PDF_NAME(CA));
            float alpha = 0;
            if (ca)
            {
                alpha = pdf_to_real(pdf->ctx, ca);
            }
            d("alpha: %.02f", alpha);
        }
    }

    status = ST_SUCCESS;
error_return:
    return status;
}

static v_status_t v_pdf_alloc_pixel_data(uint8_t *data, int page, int rowstride, v_scale_t scale)
{
    int i, w, h;
    v_status_t status = ST_PDF_OPEN_FAILED;
    fz_matrix ctm;
    fz_colorspace *cs;

    ctm = fz_scale(scale.sx, scale.sy);
    cs = fz_device_rgb(pdf->ctx);

    v_pdf_release_pixel_data();

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

static void v_pdf_save(const char *path)
{
    ERR_RET(!path, "path is null");
    ERR_RET(!pdf || !pdf->ctx, "ctx is null");

    char *dirname = get_dir_name(path);
    ERR_RET(!dirname, "unknown path: %s", path);

    if (!directory_exists(dirname))
    {
        mkdir_p(dirname, 0666);
    }
    free(dirname);

    pdf_save_document(pdf->ctx, (pdf_document *)pdf->doc, path, NULL);

error_return:
    return;
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