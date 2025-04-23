#ifndef __V_PDF_H__
#define __V_PDF_H__

#include <mupdf/fitz.h>
#include "v_canvas.h"


typedef struct
{
    fz_context *ctx;
    fz_document *doc;
    fz_page *page;
    int width;
    int height;
    int page_num;
} PdfData;


v_draw_ops_t *v_pdf_get_ops(void);
#endif
