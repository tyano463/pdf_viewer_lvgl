#include "v_png.h"
#include <cairo/cairo.h>
#include <stdlib.h>
#include <stdio.h>
#include <jpeglib.h>

#include "v_core.h"
#include "v_pdf.h"

static void init_ops(void);
static v_status_t v_png_init(void);
static v_status_t v_png_open(const char *path);
static v_status_t v_jpeg_open(const char *path);
static int v_png_page(void);
static v_status_t v_png_get_pixel(uint8_t *_data, int page, int rowstride, v_scale_t ctm);
static v_status_t v_png_get_size(int *width, int *height);
static const char *v_png_path(void);

extern char g_current_path[MAX_PATH];
static v_draw_ops_t g_ops;
static v_draw_ops_t *pdf_ops;
static cairo_surface_t *surface;

v_draw_ops_t *v_png_get_ops(void)
{
    if (!g_ops.init)
    {
        init_ops();
    }
    return &g_ops;
}

v_draw_ops_t *v_jpeg_get_ops(void)
{
    if (!g_ops.init)
    {
        init_ops();
        g_ops.open = v_jpeg_open;
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
    g_ops.path = v_png_path;
}

static const char *v_png_path(void)
{
    return g_current_path;
}
static v_status_t v_png_init(void)
{
    pdf_ops = v_pdf_get_ops();
    return pdf_ops->init();
}

static v_status_t v_png_open(const char *path)
{
    v_status_t status = ST_SUCCESS;
    ERR_RETn(path == g_current_path || strcmp(g_current_path, path) == 0);

    status = ST_PNG_OPEN_FAILED;
    char *pdf_path = png2pdf(path);
    ERR_RET(!pdf_path, "png2pdf");

    status = pdf_ops->open(pdf_path);
error_return:
    return status;
}

static v_status_t v_jpeg_open(const char *path)
{
    v_status_t status = ST_SUCCESS;
    ERR_RETn(path == g_current_path || strcmp(path, g_current_path) == 0);
    status = ST_JPEG_OPEN_FAILED;
    char *pdf_path = jpeg2pdf(path);
    ERR_RET(!pdf_path, "jpeg2pdf");
    status = pdf_ops->open(pdf_path);
error_return:
    return status;
#if 0
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    FILE *infile = fopen(path, "rb");
    ERR_RET(!infile, "fopen");

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);

    int width = cinfo.output_width;
    int height = cinfo.output_height;
    int row_stride = cinfo.output_width * cinfo.output_components; // RGBの場合、1ピクセルあたり3バイト

    uint8_t *buffer = malloc(height * row_stride);
    ERR_RET(!buffer, "malloc");

    while (cinfo.output_scanline < cinfo.output_height)
    {
        uint8_t *row_pointer[1]; // 現在のスキャンライン
        row_pointer[0] = buffer + cinfo.output_scanline * row_stride;
        jpeg_read_scanlines(&cinfo, row_pointer, 1);
    }

    // Cairoでサーフェスを作成（RGBデータをARGBに変換）
    uint8_t *argb_data = malloc(width * height * 4); // ARGB8888形式
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            const uint8_t *src_pixel = &buffer[(y * width + x) * 3];
            uint8_t *dst_pixel = &argb_data[(y * width + x) * 4];
            dst_pixel[0] = src_pixel[2]; // Blue
            dst_pixel[1] = src_pixel[1]; // Green
            dst_pixel[2] = src_pixel[0]; // Red
            dst_pixel[3] = 0xFF;         // Alpha
        }
    }

    if (surface)
    {
        cairo_surface_destroy(surface);
    }
    surface = cairo_image_surface_create_for_data(
        argb_data,
        CAIRO_FORMAT_ARGB32,
        width,
        height,
        width * 4);
    status = ST_SUCCESS;

error_return:
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    return status;
#endif
}

static int v_png_page(void)
{
    return pdf_ops->pagenum();
}

static v_status_t v_png_get_pixel(uint8_t *_data, int page, int rowstride, v_scale_t ctm)
{
    return pdf_ops->pixel(_data, page, rowstride, ctm);
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
        return ST_PNG_OPEN_FAILED;
    }

    return ST_SUCCESS;
}

static v_status_t v_png_get_size(int *width, int *height)
{
    return pdf_ops->size(width, height);
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