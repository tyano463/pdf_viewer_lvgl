#include <cairo/cairo.h>
#include <librsvg/rsvg.h>
#include "v_svg.h"
#include "v_core.h"
#include "v_pdf.h"

static void init_ops(void);
static v_status_t v_svg_init(void);
static void v_svg_free(void);
static v_status_t v_svg_open(const char *file);
static int v_svg_pagenum(void);
static v_status_t v_svg_pixel(uint8_t *_data, int page, int rowstride, v_scale_t ctm);
static v_status_t v_svg_size(int *width, int *height);
static v_annots_t *v_svg_annots(void);
static const char *v_svg_path(void);
static void v_svg_save(const char *path, v_annots_t *annots);

static v_draw_ops_t g_ops;
static v_draw_ops_t *pdf_ops;

extern char g_current_path[MAX_PATH];

v_draw_ops_t *v_svg_get_ops(void)
{
    init_ops();
    return &g_ops;
}
static void init_ops(void)
{
    if (!g_ops.init)
    {
        g_ops.init = v_svg_init;
        g_ops.free = v_svg_free;
        g_ops.open = v_svg_open;
        g_ops.pagenum = v_svg_pagenum;
        g_ops.pixel = v_svg_pixel;
        g_ops.size = v_svg_size;
        g_ops.annots = v_svg_annots;
        g_ops.save = v_svg_save;
        g_ops.path = v_svg_path;
    }
}

static const char *v_svg_path(void)
{
    return pdf_ops->path();
}

static void v_svg_save(const char *path, v_annots_t *annots)
{
    pdf_ops->save(path, annots);
}

static v_annots_t *v_svg_annots(void)
{
    return pdf_ops->annots();
}
static v_status_t v_svg_init(void)
{
    pdf_ops = v_pdf_get_ops();
    return pdf_ops->init();
}

static void v_svg_free(void)
{
    pdf_ops->free();
}

static v_status_t v_svg_open(const char *file)
{
    v_status_t status = ST_SUCCESS;
    ERR_RETn(file && strcmp(file, g_current_path) == 0);

    status = ST_SVG_OPEN_FAILED;
    char *pdf_path = svg2pdf(file);
    ERR_RET(!pdf_path, "svg2pdf");

    status = pdf_ops->open(pdf_path);

error_return:
    return status;
}

static int v_svg_pagenum(void)
{
    return pdf_ops->pagenum();
}
static v_status_t v_svg_pixel(uint8_t *_data, int page, int rowstride, v_scale_t ctm)
{
    return pdf_ops->pixel(_data, page, rowstride, ctm);
}

static v_status_t v_svg_size(int *width, int *height)
{
    return pdf_ops->size(width, height);
}