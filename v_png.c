#include "v_png.h"
#include <cairo/cairo.h>

static void init_ops(void);
static v_status_t v_png_init(void);
static v_status_t v_png_open(const char *path);
static int v_png_page(void);
static v_status_t v_png_get_pixel(uint8_t *data, int page, int rowstride, v_scale_t ctm);
static v_status_t v_png_get_size(int *width, int *height);

static v_draw_ops_t g_ops;
static cairo_surface_t *surface;

v_draw_ops_t *v_png_get_ops(void)
{
    if (!g_ops.init)
    {
        init_ops();
    }
    return &g_ops;
}

static void init_ops(void)
{
    g_ops.init = v_png_init;
    g_ops.open = v_png_open;
    g_ops.pagenum = v_png_page;
    g_ops.pixel = v_png_get_pixel;
    g_ops.size = v_png_get_size;
}
static v_status_t v_png_init(void)
{
    return ST_SUCCESS;
}

static v_status_t v_png_open(const char *path)
{
    surface = cairo_image_surface_create_from_png(path);
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS)
    {
        surface = NULL;
        return ST_PNG_OPEN_FAILED;
    }
    return ST_SUCCESS;
}

#include <stdio.h>
static int v_png_page(void) { return 0; }
static v_status_t v_png_get_pixel(uint8_t *_data, int page, int rowstride, v_scale_t ctm)
{
    int w, h;
    if (!surface)
        return ST_PNG_OPEN_FAILED;

    cairo_format_t format = cairo_image_surface_get_format(surface);
    unsigned char *data = cairo_image_surface_get_data(surface);
    int stride = cairo_image_surface_get_stride(surface);

    w = cairo_image_surface_get_width(surface);
    h = cairo_image_surface_get_height(surface);
    d("stride:%d %d,%d format:%d", stride, w, h, format);

    //    FILE *fp = fopen("/tmp/data.bin", "wb");
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
            float *s = (float *)&data[stride * i];
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
        return ST_PNG_OPEN_FAILED;
    }

    return ST_SUCCESS;
}

static v_status_t v_png_get_size(int *width, int *height)
{
    if (surface)
    {
        *width = cairo_image_surface_get_width(surface);
        *height = cairo_image_surface_get_height(surface);
        return ST_SUCCESS;
    }
    else
    {
        return ST_PNG_OPEN_FAILED;
    }
}