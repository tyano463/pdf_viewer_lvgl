#include "v_common.h"
#include "v_pdf.h"

static void init_ops(void);
static v_status_t v_pdf_init(void);
static v_status_t v_pdf_open(const char *path);
static int v_pdf_pagecount(void);
static v_status_t v_pdf_getsize(int *width, int *height);
static v_status_t v_pdf_alloc_pixel_data(uint8_t *data, int page, int rowstride, v_scale_t scale);
static void v_pdf_release_pixel_data(void);

static PdfData *pdf;
static fz_pixmap *pix;
static v_draw_ops_t ops;

static void init_ops(void)
{
    ops.init = v_pdf_init;
    ops.open = v_pdf_open;
    ops.pagenum = v_pdf_pagecount;
    ops.size = v_pdf_getsize;
    ops.pixel = v_pdf_alloc_pixel_data;
    ops.free = NULL;
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
        pdf = calloc(sizeof(PdfData), 1);
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

static v_status_t v_pdf_open(const char *path)
{
    v_status_t status;

    fz_try(pdf->ctx)
        pdf->doc = fz_open_document(pdf->ctx, path);
    fz_catch(pdf->ctx)
    {
        fz_drop_context(pdf->ctx);
        status = ST_PDF_DOCUMENT_OPEN_FAILED;
        goto error_return;
    }

    pdf->page_num = fz_count_pages(pdf->ctx, pdf->doc);

    // Load the first page
    pdf->page = fz_load_page(pdf->ctx, pdf->doc, 0);
    fz_rect bounds = fz_bound_page(pdf->ctx, pdf->page);
    pdf->width = bounds.x1 - bounds.x0;
    pdf->height = bounds.y1 - bounds.y0;

    status = ST_SUCCESS;
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

static v_status_t v_pdf_alloc_pixel_data(uint8_t *data, int page, int rowstride, v_scale_t scale)
{
    int i, w, h;
    v_status_t status = ST_PDF_OPEN_FAILED;
    fz_matrix ctm;
    fz_colorspace *cs;
    ctm = fz_scale(scale.sx, scale.sy);
    cs = fz_device_rgb(pdf->ctx);

    v_pdf_release_pixel_data();
    pix = fz_new_pixmap_from_page_number(pdf->ctx, pdf->doc, page, ctm, cs, 0);

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
        fz_drop_pixmap(pdf->ctx, pix);
    pix = NULL;
}
