#ifndef __V_PDF_H__
#define __V_PDF_H__

#include <mupdf/fitz.h>


typedef struct
{
    fz_context *ctx;
    fz_document *doc;
    fz_page *page;
    int width;
    int height;
    int page_num;
} PdfData;

typedef struct {
    float sx;
    float sy;
} pdf_scale_t;

v_status_t v_pdf_init(void);
v_status_t v_pdf_open(const char *path);
int v_pdf_pagecount(void);
v_status_t v_pdf_getsize(int *width, int *height);
PdfData *v_pdf_data(void);
v_status_t v_pdf_alloc_pixel_data(uint8_t *data, int page, int rowstride, pdf_scale_t ctm);
void v_pdf_release_pixel_data(void);
#endif
