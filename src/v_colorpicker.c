#include "v_colorpicker.h"
#include "lvgl/lvgl.h"
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#include "v_common.h"
#include "v_canvas.h"

#define PICKER_WIDTH 200
#define PICKER_HEIGHT 160
#define POINT_RADIUS 12
#define BAR_WIDTH PICKER_WIDTH
#define BAR_HEIGHT 10

static void update_picker_pos(uint16_t left, uint16_t top);
static void update_bar_pos(uint16_t left);
static lv_color_t pickerpoint2color(void);

static color_callback_t g_callback;
static lv_obj_t *picker, *bar;
static uint8_t *picker_buf;
static uint8_t *bar_buf;
static lv_obj_t *picker_point, *bar_point;
static lv_point_t last_picker_pos;
static lv_point_t last_bar_pos;
static lv_color_t last_bar_color;

static void set_picker_visibility(bool vis)
{
    ERR_RETn(!picker);
    void (*func)(lv_obj_t *, lv_obj_flag_t);
    func = vis ? lv_obj_remove_flag : lv_obj_add_flag;
    func(picker, LV_OBJ_FLAG_HIDDEN);
    func(bar, LV_OBJ_FLAG_HIDDEN);
error_return:
    return;
}

static void rgb_to_hsv(float *h, float *s, float *v, lv_color_t *rgb)
{
    float r = rgb->red / 255.0f;
    float g = rgb->green / 255.0f;
    float b = rgb->blue / 255.0f;
    float max = LV_MAX3(r, g, b);
    float min = LV_MIN3(r, g, b);
    float delta = max - min;

    if (delta > 0.00001f)
    {
        if (max == r)
            *h = fmodf(((g - b) / delta), 6.0f) / 6.0f;
        else if (max == g)
            *h = (((b - r) / delta) + 2.0f) / 6.0f;
        else
            *h = (((r - g) / delta) + 4.0f) / 6.0f;

        if (*h < 0)
            *h += 1.0f;
    }
    else
    {
        *h = 0.0f;
    }

    *s = (max == 0.0f) ? 0.0f : (delta / max);

    *v = max;
}

static lv_color_t hsv_to_rgb(float h, float s, float v)
{
    lv_color_t rgb;
    float r, g, b;
    float c = v * s;
    float x = c * (1.0f - fabsf(fmodf(h * 6.0f, 2.0f) - 1.0f));
    float m = v - c;

    if (h < 1.0f / 6.0f)
        r = c, g = x, b = 0;
    else if (h < 2.0f / 6.0f)
        r = x, g = c, b = 0;
    else if (h < 3.0f / 6.0f)
        r = 0, g = c, b = x;
    else if (h < 4.0f / 6.0f)
        r = 0, g = x, b = c;
    else if (h < 5.0f / 6.0f)
        r = x, g = 0, b = c;
    else
        r = c, g = 0, b = x;

    rgb.red = (uint8_t)((r + m) * 255);
    rgb.green = (uint8_t)((g + m) * 255);
    rgb.blue = (uint8_t)((b + m) * 255);
    return rgb;
}

static void change_2d_color(void *arg)
{
    lv_color_t *color = (lv_color_t *)arg;
    ERR_RETn(!color);

    float orig_h, orig_s, orig_v;

    rgb_to_hsv(&orig_h, &orig_s, &orig_v, color);
    d("h:%.02f", orig_h);
    for (int i = 0; i < PICKER_HEIGHT; i++)
    {
        float val = (float)(PICKER_HEIGHT - i) / (float)(PICKER_HEIGHT - 1.0);
        for (int j = 0; j < PICKER_WIDTH; j++)
        {
            float sat = ((float)j / (PICKER_WIDTH - 1.0));

            lv_color_t c = hsv_to_rgb(orig_h, sat, val);

            int index = (i * PICKER_WIDTH + j) * 3;
            picker_buf[index + 2] = c.red;
            picker_buf[index + 1] = c.green;
            picker_buf[index + 0] = c.blue;
        }
    }
error_return:
    return;
}
static void barchanged(void *arg)
{
    update_bar_pos(last_bar_pos.x);

    change_2d_color(arg);
    update_picker_pos(last_picker_pos.x, last_picker_pos.y);
    lv_obj_invalidate(picker);
    lv_color_t color = pickerpoint2color();
    uint32_t argb = (color.red << 16) | (color.green << 8) | (color.blue << 0);
    if (g_callback)
        g_callback(argb);
}

static lv_color_t barpoint2color(lv_point_t *p)
{
    float r, g, b;
    float t = (float)p->x / (BAR_WIDTH - 1);

    if (t < 0.2f)
    {
        float ratio = t / 0.2f;
        r = 1.0f;
        g = ratio * 0.2f;
        b = ratio * 0.5f;
    }
    else if (t < 0.4f)
    {
        float ratio = (t - 0.2f) / 0.2f;
        r = (1.0f - ratio * 1.0f);
        g = ratio * 0.2f;
        b = 0.5f + ratio * 0.5f;
    }
    else if (t < 0.6f)
    {
        float ratio = (t - 0.4f) / 0.2f;
        r = 0.0f;
        g = ratio * 1.0f;
        b = (1.0f - ratio * 1.0f);
    }
    else if (t < 0.8f)
    {
        float ratio = (t - 0.6f) / 0.2f;
        r = ratio * 1.0f;
        g = 1.0f;
        b = 0.0f;
    }
    else
    {
        float ratio = (t - 0.8f) / 0.2f;
        r = 1.0f;
        g = (1.0f - ratio * 1.0f);
        b = 0.0f;
    }

    lv_color_t color = {
        .red = (uint8_t)(r * 255),
        .green = (uint8_t)(g * 255),
        .blue = (uint8_t)(b * 255),
    };
    return color;
}

static void pickerbar_callback(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code > LV_EVENT_RELEASED)
        return;
    lv_point_t point, rel;
    lv_area_t coord;

    lv_indev_t *indev = lv_indev_active();

    lv_indev_get_point(indev, &point);
    lv_obj_get_coords(bar, &coord);
    rel.x = point.x - coord.x1;
    rel.y = point.y - coord.y1;
    last_bar_pos.x = rel.x;
    last_bar_pos.y = rel.y;

    switch (code)
    {
    case LV_EVENT_PRESSING:
    case LV_EVENT_RELEASED:
        lv_color_t *c = malloc(sizeof(lv_color_t));
        ERR_RETn(!c);
        *c = barpoint2color(&rel);
        last_bar_color.red = c->red;
        last_bar_color.green = c->green;
        last_bar_color.blue = c->blue;
        //        d("%d, %d (%d,%d,%d)", rel.x, rel.y, c->red, c->green, c->blue);
        lv_async_call(barchanged, (void *)c);

        break;
    default:
        break;
    }
error_return:
    return;
}

static lv_color_t pickerpoint2color(void)
{
    lv_point_t *p = &last_picker_pos;

    float orig_h, orig_s, orig_v;

    rgb_to_hsv(&orig_h, &orig_s, &orig_v, &last_bar_color);

    int16_t x, y;
    x = min(max(p->x, 0), PICKER_WIDTH - 1);
    y = min(max(p->y, 0), PICKER_HEIGHT - 1);

    float val = (float)(PICKER_HEIGHT - y) / (float)(PICKER_HEIGHT - 1.0);
    float sat = ((float)x / (PICKER_WIDTH - 1.0));

    lv_color_t c = hsv_to_rgb(orig_h, sat, val);

    return c;
}
static void picker2d_callback(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code > LV_EVENT_RELEASED)
        return;

    lv_point_t point, rel;
    lv_area_t coord;
    lv_indev_t *indev = lv_indev_active();
    lv_indev_get_point(indev, &point);
    lv_obj_get_coords(picker, &coord);
    rel.x = point.x - coord.x1;
    rel.y = point.y - coord.y1;
    //    d("%d,%d = %d,%d - %d,%d", rel.x, rel.y, point.x, point.y, coord.x1, coord.y1);
    last_picker_pos.x = rel.x;
    last_picker_pos.y = rel.y;
    switch (code)
    {
    case LV_EVENT_PRESSING:
    case LV_EVENT_RELEASED:
        lv_color_t color = pickerpoint2color();
        uint32_t argb = (color.red << 16) | (color.green << 8) | (color.blue << 0);

        // d("%d,%d -> %02x%02x%02x argb:%06x", rel.x, rel.y, color.red, color.green, color.blue, argb);

        update_picker_pos(rel.x, rel.y);
        if (g_callback)
            g_callback(argb);
        break;
    default:
        break;
    }
}

static void update_bar_pos(uint16_t left)
{
    lv_obj_set_pos(bar_point, left - POINT_RADIUS, BAR_HEIGHT / 2 - POINT_RADIUS);
    last_bar_pos.x = left;
}
static void update_picker_pos(uint16_t left, uint16_t top)
{
    left = min(left, PICKER_WIDTH);
    top = min(top, PICKER_HEIGHT);
    lv_obj_set_pos(picker_point, left - POINT_RADIUS, top - POINT_RADIUS);
    last_picker_pos.x = left;
    last_picker_pos.y = top;
}

static void create_picker(lv_obj_t *parent, uint16_t left, uint16_t top, lv_color32_t bgra)
{
    picker_buf = malloc(PICKER_HEIGHT + PICKER_WIDTH * PICKER_HEIGHT * 3);
    picker = lv_canvas_create(parent);
    d("picker:%p", picker);
    lv_color_t default_color = {
        .red = bgra.red,
        .green = bgra.green,
        .blue = bgra.blue};
    float orig_h, orig_s, orig_v;

    rgb_to_hsv(&orig_h, &orig_s, &orig_v, &default_color);
    d("h:%.02f", orig_h);
    for (int i = 0; i < PICKER_HEIGHT; i++)
    {
        float val = (float)(PICKER_HEIGHT - i) / (float)(PICKER_HEIGHT - 1.0);
        for (int j = 0; j < PICKER_WIDTH; j++)
        {
            float sat = ((float)j / (PICKER_WIDTH - 1.0));

            lv_color_t c = hsv_to_rgb(orig_h, sat, val);

            int index = (i * PICKER_WIDTH + j) * 3;
            picker_buf[index + 2] = c.red;
            picker_buf[index + 1] = c.green;
            picker_buf[index + 0] = c.blue;
        }
    }
    lv_canvas_set_buffer(picker, picker_buf, PICKER_WIDTH, PICKER_HEIGHT, LV_COLOR_FORMAT_RGB888);
    lv_obj_set_size(picker, PICKER_WIDTH, PICKER_HEIGHT);
    lv_obj_set_pos(picker, left, top);

    picker_point = lv_obj_create(picker);
    lv_obj_set_size(picker_point, POINT_RADIUS * 2, POINT_RADIUS * 2);
    lv_obj_set_style_radius(picker_point, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(picker_point, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(picker_point, lv_color_white(), 0);
    lv_obj_set_style_border_width(picker_point, 8, 0);
    lv_obj_set_style_border_opa(picker_point, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_width(picker_point, 0, 0);

    int point_x = orig_s * (PICKER_WIDTH - 1);
    int point_y = (1.0f - orig_v) * (PICKER_HEIGHT - 1);

    lv_obj_set_pos(picker_point, point_x - POINT_RADIUS, point_y - POINT_RADIUS);

    last_picker_pos.x = point_x;
    last_picker_pos.y = point_y;

    lv_obj_add_flag(picker, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(picker, picker2d_callback, LV_EVENT_ALL, NULL);
}

static void create_bar(lv_obj_t *parent, uint16_t left, uint16_t top, lv_color32_t bgra)
{
    bar = lv_canvas_create(parent);
    bar_buf = malloc(BAR_WIDTH * BAR_HEIGHT * 3);

    for (int j = 0; j < BAR_WIDTH; j++)
    {
        float r, g, b;
        float t = (float)j / (BAR_WIDTH - 1);

        if (t < 0.2f)
        {
            float ratio = t / 0.2f;
            r = 1.0f;
            g = ratio * 0.2f;
            b = ratio * 0.5f;
        }
        else if (t < 0.4f)
        {
            float ratio = (t - 0.2f) / 0.2f;
            r = (1.0f - ratio * 1.0f);
            g = ratio * 0.2f;
            b = 0.5f + ratio * 0.5f;
        }
        else if (t < 0.6f)
        {
            float ratio = (t - 0.4f) / 0.2f;
            r = 0.0f;
            g = ratio * 1.0f;
            b = (1.0f - ratio * 1.0f);
        }
        else if (t < 0.8f)
        {
            float ratio = (t - 0.6f) / 0.2f;
            r = ratio * 1.0f;
            g = 1.0f;
            b = 0.0f;
        }
        else
        {
            float ratio = (t - 0.8f) / 0.2f;
            r = 1.0f;
            g = (1.0f - ratio * 1.0f);
            b = 0.0f;
        }
        for (int i = 0; i < BAR_HEIGHT; i++)
        {
            bar_buf[(i * BAR_WIDTH + j) * 3 + 2] = (uint8_t)(r * 255.0);
            bar_buf[(i * BAR_WIDTH + j) * 3 + 1] = (uint8_t)(g * 255.0);
            bar_buf[(i * BAR_WIDTH + j) * 3 + 0] = (uint8_t)(b * 255.0);
        }
    }
    lv_canvas_set_buffer(bar, bar_buf, BAR_WIDTH, BAR_HEIGHT, LV_COLOR_FORMAT_RGB888);
    lv_obj_set_size(bar, BAR_WIDTH, BAR_HEIGHT);
    lv_obj_set_pos(bar, left, top + PICKER_HEIGHT + 10);

    bar_point = lv_obj_create(bar);
    lv_obj_set_size(bar_point, POINT_RADIUS * 2, POINT_RADIUS * 2);
    lv_obj_set_style_radius(bar_point, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(bar_point, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(bar_point, lv_color_white(), 0);
    lv_obj_set_style_border_width(bar_point, 8, 0);
    lv_obj_set_style_border_opa(bar_point, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_width(bar_point, 0, 0);

    float hue, s, v;
    lv_color_t color = (lv_color_t){bgra.blue, bgra.green, bgra.red};
    rgb_to_hsv(&hue, &s, &v, &color);
    int16_t x_pos = (int16_t)(hue * (BAR_WIDTH - 1)) - POINT_RADIUS;
    int16_t y_pos = (BAR_HEIGHT / 2) - POINT_RADIUS;
    lv_obj_set_pos(bar_point, x_pos, y_pos);

    lv_obj_add_flag(bar, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(bar, pickerbar_callback, LV_EVENT_ALL, NULL);
}
void v_show_color_picker(color_callback_t _callback, lv_obj_t *parent, int16_t left, int16_t top, lv_color32_t bgra)
{
    if (picker)
    {
        set_picker_visibility(true);
        return;
    }

    create_picker(parent, left, top, bgra);
    create_bar(parent, left, top, bgra);

    g_callback = _callback;
}

void v_hide_color_picker(void)
{
    set_picker_visibility(false);
}
