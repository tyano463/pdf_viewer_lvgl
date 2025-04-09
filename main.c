#include <stdbool.h>
#include <unistd.h>
#include <X11/Xlib.h>

#include "misc/lv_types.h"
#include "v_common.h"
#include "v_pdf.h"
#include "v_menu.h"

#define PDF_FILE "/usr/share/sample.pdf"
#define SWIPE_MARGIN (50)

#define WIDTH 1024
#define HEIGHT 768

static v_status_t show_pdf(lv_obj_t *parent);

int main(int argc, char **argv)
{
    v_status_t status;

    lv_init();

    lv_display_t *disp = lv_x11_window_create("A Title", WIDTH, HEIGHT);
    lv_x11_inputs_create(disp, NULL);
    lv_obj_t *scr = lv_screen_active();
    status = show_pdf(scr);

    d("show_pdf:%d", status);

    while (1)
    {
        lv_timer_handler();
        usleep(5000);
    }
error_return:
    return status;
}

static lv_color_t *pdf_data;
static v_status_t show_pdf(lv_obj_t *parent)
{
    v_status_t status = ST_PDF_OPEN_FAILED;
    int w, h, stride;
    status = v_pdf_init();
    ERR_RETn(status != ST_SUCCESS);

    status = v_pdf_open(PDF_FILE);
    ERR_RETn(status != ST_SUCCESS);
    status = v_pdf_getsize(&w, &h);
    ERR_RETn(status != ST_SUCCESS);
    if (pdf_data)
        free(pdf_data);
    pdf_data = calloc(h * w * 3, 1);
    status = v_pdf_alloc_pixel_data(pdf_data, 0, w * 3, (pdf_scale_t){1, 1});
    ERR_RETn(status != ST_SUCCESS);

    size_t size = w * h * sizeof(lv_color_t);

    static lv_img_dsc_t pdf_image;
    pdf_image.data = (const uint8_t *)pdf_data;
    pdf_image.header.magic = LV_IMAGE_HEADER_MAGIC;
    pdf_image.data_size = size;
    pdf_image.header.w = w;
    pdf_image.header.h = h;
    pdf_image.header.cf = LV_COLOR_FORMAT_RGB888;
    lv_obj_t *img_obj = lv_img_create(parent);
    lv_img_set_src(img_obj, &pdf_image);
    lv_obj_align(img_obj, LV_ALIGN_TOP_MID, 0, 0);

    status = ST_SUCCESS;
error_return:
    return status;
}
