#include <stdbool.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <librsvg/rsvg.h>
#include <cairo/cairo.h>
#include "misc/lv_types.h"
#include "v_core.h"
#include "v_common.h"
#include "v_pdf.h"
#include "v_menu.h"
#include "v_pen.h"
#include "v_misc.h"
#include "v_canvas.h"
#include "v_icon.h"
#include "v_settings.h"
#include "v_musicxml.h"
#include "v_png.h"
#include "v_svg.h"
#include "v_midi.h"

#define SWIPE_MARGIN (50)

#define WINDOW_TITLE "PDF Viewer(LVGL)"

static void init(void);
static v_status_t disp_init(void);
static void file_opened(const char *);
static void mode_changed(v_mode_t);
static void pdf_callback(lv_event_t *e);
static v_format_t get_format(const char *path);
static v_status_t show_page(v_format_t format, const char *path, uint16_t page);

static lv_display_t *disp;
static v_pen_cb_ops_t pen_cbs;
static v_pen_ops_t *pen_ops;
static v_menu_cb_ops_t menu_cbs;
static v_menu_ops_t *menu_ops;
static v_mode_t mode;
static lv_point_t touch_point;
static v_draw_ops_t *(*draw_ops[V_FORMAT_MAX])(void);
static uint8_t *orig_data;
static v_viewer_ops_t *view_ops;
static char *current_path;
static v_draw_ops_t *current_ops;

int main(int argc, char **argv)
{
    v_status_t status;
    char *path;
    uint16_t page;
    v_log_init();

    lv_init();

    status = disp_init();
    ERR_RETn(status != ST_SUCCESS);

    lv_obj_t *scr = lv_screen_active();
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM);

    (void)v_load_settings();

    init();

    view_ops = v_get_canvas_ops();
    ERR_RET(!view_ops || !view_ops->init, "canvas ops");

    status = view_ops->init(scr);
    ERR_RET(status != ST_SUCCESS, "init canvas");

    v_settings_ops_t *settings = v_get_settings_ops();

    menu_ops = v_get_menu_ops();
    ERR_RET(!menu_ops, "menu ops");
    status = menu_ops->init(scr, &menu_cbs);
    ERR_RET(status != ST_SUCCESS, "menu init");

    view_ops->set_touch_callback(pdf_callback);
    pen_cbs.on_mode_change = mode_changed;
    pen_ops = v_pen_getops();
    ERR_RET(!pen_ops, "pen ops");

    status = pen_ops->init(scr, &pen_cbs);
    d("pen init %d", status);

    path = strdup(settings->get_path());
    page = settings->get_page();
    current_path = path;

    v_format_t format = get_format(path);
    ERR_RET(format >= V_FORMAT_MAX, "get format @%s", path);
    status = show_page(format, path, page);
    ERR_RETn(status != ST_SUCCESS);

    while (1)
    {
        lv_timer_handler();
        usleep(5000);
    }
error_return:
    return status;
}

static void export_pdf(const char *file)
{
    d("%s", file);
}
static void save_current_file(void)
{
    d("");
}
static void save_as(const char *file)
{
    d("%s", file);
}

static void show_mode(v_show_mode_t mode)
{
    d("mode:%d", mode);
    switch (mode)
    {
    case V_SHOW_MODE_ANNOT_WITH_MENU:
        menu_ops->show_icon();
        view_ops->show_annot();
        pen_ops->show_icon();
        break;
    case V_SHOW_MODE_ANNOT_NO_MENU:
        menu_ops->hide_icon();
        view_ops->show_annot();
        pen_ops->hide_icon();
        break;
    case V_SHOW_MODE_SCORE_ONLY:
        menu_ops->hide_icon();
        view_ops->hide_annot();
        pen_ops->hide_icon();
        break;
    }
}

static void init(void)
{
    menu_cbs.file_opened = file_opened;
    menu_cbs.export_pdf = export_pdf;
    menu_cbs.save = save_current_file;
    menu_cbs.save_as = save_as;
    menu_cbs.show_mode = show_mode;

    draw_ops[V_FORMAT_JPEG] = v_jpeg_get_ops;
    draw_ops[V_FORMAT_MIDI] = v_midi_get_ops;
    draw_ops[V_FORMAT_MUSICXML] = v_musicxml_get_ops;
    draw_ops[V_FORMAT_PDF] = v_pdf_get_ops;
    draw_ops[V_FORMAT_PNG] = v_png_get_ops;
    draw_ops[V_FORMAT_SVG] = v_svg_get_ops;
}

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
            // ops->draw(point.x, point.y, 0, V_PEN_DRAW_START);
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
                //    ops->draw(point.x, point.y, 0, V_PEN_DRAW_END);
            }
        }
    }
    else
    {
        if (mode == MODE_PEN)
        {
            // ops->draw(point.x, point.y, 0, V_PEN_DRAW_MOVE);
        }
    }
}

static v_status_t disp_init(void)
{
    extern lv_image_dsc_t mouse_cursor_icon;
    disp = lv_x11_window_create(WINDOW_TITLE, WIDTH, HEIGHT);
    if (disp)
        lv_x11_inputs_create(disp, &mouse_cursor_icon);
    return (disp) ? ST_SUCCESS : ST_DISPLAY_INIT_FAILED;
}

static void file_opened(const char *path)
{
    v_status_t status;
    ERR_RETn(strcmp(current_path, path) == 0);

    current_ops->free();

    v_format_t format = get_format(path);
    status = show_page(format, path, 0);
    ERR_RET(status != ST_SUCCESS, "show page %d %d @%s", format, 0, path);
error_return:
    return;
}

static void mode_changed(v_mode_t _mode)
{
    mode = _mode;
}

static v_draw_ops_t *get_ops(v_format_t format)
{
    current_ops = draw_ops[format]();
    return current_ops;
}

static uint8_t *autoscale(uint8_t *orig, int *_w, int *_h, float *_scale)
{
    uint8_t *data = NULL;
    int w = *_w;
    int h = *_h;
    float scale_x = (float)lv_obj_get_width(lv_screen_active()) / w;
    float scale_y = (float)lv_obj_get_height(lv_screen_active()) / h;

    float scale = min(scale_x, scale_y);
    *_scale = scale;

    int scaled_w = (int)((float)w * scale);
    int scaled_h = (int)((float)h * scale);
    d("scale %.02f,%.02f -> %.02f (%d, %d) => (%d, %d)", scale_x, scale_y, scale, w, h, scaled_w, scaled_h);

    static cairo_surface_t *scaled_surface = NULL;

    cairo_surface_t *orig_surface = cairo_image_surface_create_for_data(orig, CAIRO_FORMAT_ARGB32, w, h, w * 4);
    ERR_RET(!orig_surface, "cairo_image_surface_create_for_data");
    if (scaled_surface)
    {
        cairo_surface_destroy(scaled_surface);
    }
    scaled_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, scaled_w, scaled_h);

    cairo_t *cr = cairo_create(scaled_surface);
    cairo_scale(cr, (double)scale, (double)scale);
    cairo_set_source_surface(cr, orig_surface, 0, 0);
    cairo_paint(cr);

    cairo_destroy(cr);
    cairo_surface_destroy(orig_surface);

    *_w = scaled_w;
    *_h = scaled_h;
    data = cairo_image_surface_get_data(scaled_surface);
error_return:
    return data;
}

static v_status_t show_page(v_format_t format, const char *path, uint16_t page)
{
    d("format:%d", format);
    v_draw_ops_t *ops = get_ops(format);
    ERR_RET(!ops || !ops->init || !ops->open || !ops->size || !ops->pixel, "get ops %p", ops);

    v_status_t status = ST_PDF_OPEN_FAILED;
    int w, h;
    status = ops->init();
    ERR_RET(status != ST_SUCCESS, "pdf init failed");

    status = ops->open(path);
    ERR_RET(status != ST_SUCCESS, "pdf open failed");

    status = ops->size(&w, &h);
    ERR_RETn(status != ST_SUCCESS);

    d("(w, h) = (%d, %d)", w, h);
    if (orig_data)
    {
        free(orig_data);
    }
    orig_data = calloc(h * w * 4, 1);
    status = ops->pixel(orig_data, 0, w * 4, (v_scale_t){1, 1});
    ERR_RET(status != ST_SUCCESS, "### ERROR get pixel");

    float scale;
    uint8_t *data = autoscale(orig_data, &w, &h, &scale);

    static v_image_t image;
    image.buf = data;
    image.format = LV_COLOR_FORMAT_ARGB8888;
    image.height = h;
    image.width = w;
    image.size = w * h * 4;
    view_ops->show_image(&image, scale);

    status = ST_SUCCESS;
    ERR_RETn(!ops->annots);
    v_annots_t *annots = ops->annots();
    ERR_RETn(!annots);

    for (int i = 0; i < annots->num; i++)
    {
        v_annot_kind_t k = annots->annot[i].kind;
        d("type: %d ", k);
        if (k == V_ANNOT_FREETEXT)
        {
            v_annot_t *a = &annots->annot[i];
            d("pos: %p %.0f, %.0f", &a->data.freetext.position, a->data.freetext.position.left, a->data.freetext.position.top);
        }
        view_ops->add_annot(&annots->annot[i]);
    }

error_return:
    return status;
}

static v_format_t get_format(const char *path)
{
    if (!file_exists(path))
        return V_FORMAT_MAX;

    if (ends_with_ignore_case(path, ".pdf"))
        return V_FORMAT_PDF;
    if (ends_with_ignore_case(path, ".jpeg"))
        return V_FORMAT_JPEG;
    if (ends_with_ignore_case(path, ".jpg"))
        return V_FORMAT_JPEG;
    if (ends_with_ignore_case(path, ".png"))
        return V_FORMAT_PNG;
    if (ends_with_ignore_case(path, ".svg"))
        return V_FORMAT_SVG;
    if (ends_with_ignore_case(path, ".mxl"))
        return V_FORMAT_MUSICXML;
    if (ends_with_ignore_case(path, ".xml"))
        return V_FORMAT_MUSICXML;
    if (ends_with_ignore_case(path, ".musicxml"))
        return V_FORMAT_MUSICXML;
    if (ends_with_ignore_case(path, ".mid"))
        return V_FORMAT_MIDI;
    if (ends_with_ignore_case(path, ".midi"))
        return V_FORMAT_MIDI;

    return V_FORMAT_MAX;
}
