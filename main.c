#include <stdbool.h>
#include <unistd.h>
#include <X11/Xlib.h>

#include "misc/lv_types.h"
#include "v_common.h"
#include "v_pdf.h"
#include "v_menu.h"
#include "v_pen.h"
#include "v_misc.h"

#define PDF_FILE "/usr/share/sample.pdf"
#define SWIPE_MARGIN (50)

#define WIDTH 1024
#define HEIGHT 768

#define WINDOW_TITLE "A Title"


static v_status_t disp_init(void);
static v_status_t show_pdf(lv_obj_t *parent, const char *path);
static void file_opened(char *);
static void mode_changed(v_mode_t);
static const char *get_last_opened(void);

static lv_display_t *disp;
static lv_color_t *pdf_data;
static lv_obj_t *draw_area;
static v_menu_ops_t menu_ops;
static v_pen_ops_t pen_ops;
static bool initialized = false;
static v_mode_t mode;

int main(int argc, char **argv)
{
    v_status_t status;
    const char *pdf_path;
    v_log_init();

    lv_init();

    status = disp_init();
    ERR_RETn(status != ST_SUCCESS);
    d("display init");

    lv_obj_t *scr = lv_screen_active();
    d("scr:%p", scr);

    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM);

    pdf_path = get_last_opened();
    status = show_pdf(scr, pdf_path);
    ERR_RETn(status != ST_SUCCESS);
    d("show_pdf:%d", status);

    menu_ops.file_opened = file_opened;
    status = v_menu_init(scr, &menu_ops);

    pen_ops.mode = mode_changed;
    status = v_pen_init(scr, &pen_ops);

    while (1)
    {
        lv_timer_handler();
        usleep(5000);
    }
error_return:
    return status;
}

static lv_point_t touch_point;

static void pdf_callback(lv_event_t *e)
{
#define THRESHOLD 10
    lv_event_code_t code = lv_event_get_code(e);
    if (code > LV_EVENT_RELEASED)
        return;

    lv_point_t point;
    lv_indev_t *indev = lv_indev_active();
    lv_indev_get_point(indev, &point);

    if (code == LV_EVENT_PRESSED)
    {
        touch_point.x = point.x;
        touch_point.y = point.y;
        d("touch %d,%d", point.x, point.y);
        if (mode == MODE_PEN)
        {
            v_pen_ops_t *ops = v_pen_getops();
            ops->draw(point.x, point.y, 0, V_PEN_DRAW_START);
        }
        else
        {
        }
    }
    else if (code == LV_EVENT_RELEASED)
    {
        if (distance(&touch_point, &point) < THRESHOLD)
        {
            d("tapped");
        }
        else
        {
            d("swiped %d,%d", point.x, point.y);
            if (mode == MODE_PEN)
            {
                v_pen_ops_t *ops = v_pen_getops();
                ops->draw(point.x, point.y, 0, V_PEN_DRAW_END);
            }
        }
    }
    else
    {
        if (mode == MODE_PEN)
        {
            v_pen_ops_t *ops = v_pen_getops();
            ops->draw(point.x, point.y, 0, V_PEN_DRAW_MOVE);
        }
    }
}

static v_status_t show_pdf(lv_obj_t *parent, const char *path)
{
    v_status_t status = ST_PDF_OPEN_FAILED;
    int w, h, stride;
    status = v_pdf_init();
    ERR_RETn(status != ST_SUCCESS);

    status = v_pdf_open(path);
    ERR_RETn(status != ST_SUCCESS);
    status = v_pdf_getsize(&w, &h);
    ERR_RETn(status != ST_SUCCESS);
    if (pdf_data)
        free(pdf_data);
    pdf_data = calloc(h * w * 3, 1);
    status = v_pdf_alloc_pixel_data((uint8_t *)pdf_data, 0, w * 3, (pdf_scale_t){1, 1});
    ERR_RETn(status != ST_SUCCESS);

    size_t size = w * h * sizeof(lv_color_t);

    static lv_img_dsc_t pdf_image;
    pdf_image.data = (const uint8_t *)pdf_data;
    pdf_image.header.magic = LV_IMAGE_HEADER_MAGIC;
    pdf_image.data_size = size;
    pdf_image.header.w = w;
    pdf_image.header.h = h;
    pdf_image.header.cf = LV_COLOR_FORMAT_RGB888;

    if (!initialized)
        draw_area = lv_img_create(parent);

    lv_img_set_src(draw_area, &pdf_image);

    if (!initialized)
    {
        lv_obj_align(draw_area, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_flag_t flag = LV_OBJ_FLAG_CLICKABLE;
        lv_obj_add_flag(draw_area, flag);
        flag = LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM;
        lv_obj_remove_flag(draw_area, flag);
        lv_obj_add_event_cb(draw_area, pdf_callback, LV_EVENT_ALL, NULL);
        d("add callback");
        initialized = true;
    }
    status = ST_SUCCESS;
error_return:
    return status;
}

static v_status_t disp_init(void)
{
    extern lv_image_dsc_t mouse_cursor_icon;
    disp = lv_x11_window_create(WINDOW_TITLE, WIDTH, HEIGHT);
    if (disp)
        lv_x11_inputs_create(disp, &mouse_cursor_icon);
    return (disp) ? ST_SUCCESS : ST_DISPLAY_INIT_FAILED;
}

static void file_opened(char *path)
{
    show_pdf(lv_screen_active(), path);
}

static void mode_changed(v_mode_t _mode)
{
    mode = _mode;
}

static const char *get_last_opened(void)
{
    return PDF_FILE;
}