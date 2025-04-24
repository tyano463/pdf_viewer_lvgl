#include <unistd.h>
#include <stdlib.h>

#include "v_misc.h"
#include "v_musicxml.h"

#define SVG_EXT_LEN 4
#define VEROVIO "verovio"

extern v_draw_ops_t *v_svg_get_ops(void);

static void init_ops(void);
static v_status_t v_musicxml_init(void);
static void v_musicxml_free(void);
static v_status_t v_musicxml_open(const char *file);
static int v_musicxml_pagenum(void);
static v_status_t v_musicxml_pixel(uint8_t *data, int page, int rowstride, v_scale_t ctm);
static v_status_t v_musicxml_size(int *width, int *height);

static v_draw_ops_t g_ops;
static v_draw_ops_t *svg_ops;
static char svg_file[] = "/tmp/temp_svg_XXXXXX.svg";
static const char *svg = NULL;


v_draw_ops_t *v_musicxml_get_ops(void)
{
    init_ops();

    return &g_ops;
}

static void init_ops(void)
{
    if (!g_ops.init)
    {
        g_ops.init = v_musicxml_init;
        g_ops.free = v_musicxml_free;
        g_ops.open = v_musicxml_open;
        g_ops.pagenum = v_musicxml_pagenum;
        g_ops.pixel = v_musicxml_pixel;
        g_ops.size = v_musicxml_size;
        svg_ops = v_svg_get_ops();
    }
}
static const char *musicxml2svg(const char *mxl)
{
    const char *file = NULL;
    int fd = mkstemps(svg_file, SVG_EXT_LEN);
    ERR_RET(fd < 0, "mkstemps");
    close(fd);

    const char *result = execute_command(VEROVIO, mxl, "-o", svg_file);
    ERR_RET(!result, "convert failed");

    file = svg_file;

error_return:
    return file;
}

static v_status_t v_musicxml_init(void)
{
    return ST_SUCCESS;
}
static void v_musicxml_free(void)
{
    svg_ops->free();
}

static v_status_t v_musicxml_open(const char *file)
{
    v_status_t status = ST_MXL_OPEN_FAILED;
    const char *svg = musicxml2svg(file);

    return svg_ops->open(svg);
}
static int v_musicxml_pagenum(void)
{
    return 1;
}
static v_status_t v_musicxml_pixel(uint8_t *data, int page, int rowstride, v_scale_t ctm)
{
    return svg_ops->pixel(data, page, rowstride, ctm);
}
static v_status_t v_musicxml_size(int *width, int *height)
{
    return svg_ops->size(width, height);
}