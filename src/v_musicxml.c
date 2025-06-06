#include <unistd.h>
#include <stdlib.h>
#include <cairo/cairo.h>
#include <cairo/cairo-pdf.h>
#include <librsvg/rsvg.h>

#include "v_misc.h"
#include "v_core.h"
#include "v_musicxml.h"
#include "v_pdf.h"

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
static v_annots_t *v_musicxml_annots(void);
static const char *v_musicxml_path(void);

static v_draw_ops_t g_ops;
static v_draw_ops_t *pdf_ops;
static char svg_file[] = "/tmp/temp_svg_XXXXXX.svg";

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
        g_ops.annots = v_musicxml_annots;
        g_ops.path = v_musicxml_path;
        pdf_ops = v_pdf_get_ops();
    }
}
static const char *v_musicxml_path(void)
{
    return pdf_ops->path();
}

static char *pdf_concat(char **files)
{
    char *pdf_file = NULL;
    char *path;
    char *pattern;
    ERR_RET(!files || !files[0], "file name is null");
    char command[MAX_PATH];

    path = get_original_filename(files[0]);
    if (strcmp(path, files[0]) == 0)
    {
        pdf_file = path;
        goto error_return;
    }

    pattern = get_file_pattern(files[0]);

    sprintf(command, "qpdf --empty --pages %s -- %s", pattern, path);
    execute_command(command);

    pdf_file = path;
error_return:
    return pdf_file;
}
static char *multi_svg2pdf(char **files)
{
    char *file;
    char path[MAX_PATH];
    char *pdf_path;
    for (int i = 0; files[i]; i++)
    {
        strcpy(path, files[i]);
        pdf_path = svg2pdf(path);
        strcpy(files[i], pdf_path);
        free(pdf_path);
    }

    file = pdf_concat(files);
    printf("file is %s\n", file);
    return file;
}

static const char *musicxml2pdf(const char *mxl)
{
    d("%s", mxl);
    const char *file = NULL;
    int fd = mkstemps(svg_file, SVG_EXT_LEN);
    ERR_RET(fd < 0, "mkstemps");
    close(fd);

    const char *result = execute_command(VEROVIO, "--all-pages", mxl, "-o", svg_file);
    ERR_RET(!result, "convert failed");

    char **files = list_sequence_files(svg_file);

    file = multi_svg2pdf(files);

error_return:
    return file;
}

static v_status_t v_musicxml_init(void)
{
    execute_command("rm", "-rf", "/tmp/temp_*.svg", NULL);

    return ST_SUCCESS;
}
static void v_musicxml_free(void)
{
    pdf_ops->free();
}

static v_status_t v_musicxml_open(const char *file)
{
    v_status_t status = ST_MXL_OPEN_FAILED;
    const char *svg = musicxml2pdf(file);
    ERR_RETn(!svg);

    status = pdf_ops->open(svg);
error_return:
    return status;
}
static int v_musicxml_pagenum(void)
{
    return pdf_ops->pagenum();
}
static v_status_t v_musicxml_pixel(uint8_t *data, int page, int rowstride, v_scale_t ctm)
{
    return pdf_ops->pixel(data, page, rowstride, ctm);
}
static v_status_t v_musicxml_size(int *width, int *height)
{
    return pdf_ops->size(width, height);
}
static v_annots_t *v_musicxml_annots(void)
{
    return pdf_ops->annots();
}