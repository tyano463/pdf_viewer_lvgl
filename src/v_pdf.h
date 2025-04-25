#ifndef __V_PDF_H__
#define __V_PDF_H__

#include <stdbool.h>
#include <mupdf/fitz.h>
#include <mupdf/pdf.h>
#include "v_canvas.h"

typedef struct
{
    fz_context *ctx;
    fz_document *doc;
    fz_page *page;
    const char *path;
    int16_t width;
    int16_t height;
    int16_t page_num;
    bool changed;
} v_pdf_t;

v_draw_ops_t *v_pdf_get_ops(void);
#endif
