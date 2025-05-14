#include <cairo/cairo.h>
#include <librsvg/rsvg.h>
#include "v_svg.h"

static void init_ops(void);
static v_status_t v_svg_init(void);
static void v_svg_free(void);
static v_status_t v_svg_open(const char *file);
static int v_svg_pagenum(void);
static v_status_t v_svg_pixel(uint8_t *_data, int page, int rowstride, v_scale_t ctm);
static v_status_t v_svg_size(int *width, int *height);

static v_draw_ops_t g_ops;
static RsvgHandle *handle;
static cairo_surface_t *surface;
static cairo_t *cr;

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
    }
}

static v_status_t v_svg_init(void)
{
    return ST_SUCCESS;
}

static void v_svg_free(void)
{
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    g_object_unref(handle);
    cr = NULL;
    surface = NULL;
    handle = NULL;
}

static v_status_t v_svg_open(const char *file)
{
    v_status_t status = ST_SVG_OPEN_FAILED;
    GError *error = NULL;
    handle = rsvg_handle_new_from_file(file, &error);
    ERR_RET(!handle, "svg open failed");

    gdouble w, h;
    rsvg_handle_get_intrinsic_size_in_pixels(handle, &w, &h);
    RsvgRectangle viewport = {
        .x = 0.0,
        .y = 0.0,
        .width = w,
        .height = h};

    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (int)w, (int)h);
    cr = cairo_create(surface);

    ERR_RET(!rsvg_handle_render_document(handle, cr, &viewport, &error), "render failed");

    status = ST_SUCCESS;
error_return:
    return status;
}

static int v_svg_pagenum(void) { return 1; }
static v_status_t v_svg_pixel(uint8_t *_data, int page, int rowstride, v_scale_t ctm)
{
    int w, h;

    cairo_format_t format = cairo_image_surface_get_format(surface);
    unsigned char *data = cairo_image_surface_get_data(surface);
    int stride = cairo_image_surface_get_stride(surface);

    w = cairo_image_surface_get_width(surface);
    h = cairo_image_surface_get_height(surface);

    switch (format)
    {
    case CAIRO_FORMAT_RGB24:
        for (int i = 0; i < h; i++)
        {
            uint8_t *s = &data[stride * i];
            for (int j = 0; j < w; j++)
            {
                uint8_t *p = &_data[i * w * 4 + j * 4];
                p[0] = s[2];
                p[1] = s[1];
                p[2] = s[0];
                p[3] = 0xff;
                s += 3;
            }
        }
        break;
    case CAIRO_FORMAT_ARGB32:
        for (int i = 0; i < h; i++)
        {
            uint8_t *s = &data[stride * i];
            for (int j = 0; j < w; j++)
            {
                uint8_t *p = &_data[i * w * 4 + j * 4];
                p[0] = s[0];
                p[1] = s[1];
                p[2] = s[2];
                p[3] = s[3];
                s += 4;
            }
        }
        break;
    case CAIRO_FORMAT_RGBA128F:
        for (int i = 0; i < h; i++)
        {
            float *s = (float *)(intptr_t)&data[stride * i];
            for (int j = 0; j < w; j++)
            {
                uint8_t *p = &_data[i * w * 4 + j * 4];
                //                d("i,j:%d,%d s:%p p:%p", i, j, s, p);
                p[0] = (uint8_t)(s[2] * 255.0f);
                p[1] = (uint8_t)(s[1] * 255.0f);
                p[2] = (uint8_t)(s[0] * 255.0f);
                p[3] = (uint8_t)(s[3] * 255.0f);
                s += 4;
                //                fwrite(p, 4, 1, fp);
            }
        }
        //        fclose(fp);
        break;
    case CAIRO_FORMAT_A8:
        for (int i = 0; i < h; i++)
        {
            uint8_t *s = &data[stride * i];
            for (int j = 0; j < w; j++)
            {
                uint8_t *p = &_data[i * w * 4 + j * 4];
                p[0] = p[1] = p[2] = 0;
                p[3] = s[0];
                s += 1;
            }
        }
        break;
    default:
        return ST_MXL_OPEN_FAILED;
    }

    return ST_SUCCESS;
}

static v_status_t v_svg_size(int *width, int *height)
{
    v_status_t status = ST_MXL_OPEN_FAILED;

    ERR_RETn(!surface);

    *width = cairo_image_surface_get_width(surface);
    *height = cairo_image_surface_get_height(surface);
    status = ST_SUCCESS;
error_return:
    return status;
}