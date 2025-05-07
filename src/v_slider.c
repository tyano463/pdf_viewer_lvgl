#include "v_slider.h"
#include "v_common.h"

static void set_slider_visibility(bool vis);

static lv_obj_t *slider;
static v_slider_callback_t g_callback;

static void slider_event_cb(lv_event_t *e)
{
    int32_t value = lv_slider_get_value(slider);
    if (g_callback)
    {
        g_callback(value);
    }
}

void v_show_slider(v_slider_callback_t callback, lv_obj_t *parent, lv_area_t *rect, uint16_t vmax)
{
    if (slider)
    {
        set_slider_visibility(true);
        return;
    }
    g_callback = callback;
    /*Create a transition*/
    static const lv_style_prop_t props[] = {LV_STYLE_BG_COLOR, 0};
    static lv_style_transition_dsc_t transition_dsc;
    lv_style_transition_dsc_init(&transition_dsc, props, lv_anim_path_linear, 300, 0, NULL);

    static lv_style_t style_main;
    static lv_style_t style_indicator;
    static lv_style_t style_knob;
    static lv_style_t style_pressed_color;
    lv_style_init(&style_main);
    lv_style_set_bg_opa(&style_main, LV_OPA_COVER);
    lv_style_set_bg_color(&style_main, lv_color_hex3(0xbbb));
    lv_style_set_radius(&style_main, LV_RADIUS_CIRCLE);
    lv_style_set_pad_ver(&style_main, -2); /*Makes the indicator larger*/

    lv_style_init(&style_indicator);
    lv_style_set_bg_opa(&style_indicator, LV_OPA_COVER);
    lv_style_set_bg_color(&style_indicator, (lv_color_t)LV_COLOR_MAKE(255, 255, 255));
    lv_style_set_radius(&style_indicator, LV_RADIUS_CIRCLE);
    lv_style_set_border_width(&style_indicator, 1);
    lv_style_set_transition(&style_indicator, &transition_dsc);
    lv_style_set_height(&style_indicator, 8);

    lv_style_init(&style_knob);
    lv_style_set_bg_opa(&style_knob, LV_OPA_COVER);
    lv_style_set_bg_color(&style_knob, (lv_color_t)LV_COLOR_MAKE(255, 255, 255));
    lv_style_set_border_color(&style_knob, (lv_color_t)LV_COLOR_MAKE(0, 0, 0));
    lv_style_set_border_width(&style_knob, 2);
    lv_style_set_radius(&style_knob, LV_RADIUS_CIRCLE);
    lv_style_set_pad_all(&style_knob, 3); /*Makes the knob larger*/
    lv_style_set_transition(&style_knob, &transition_dsc);

    lv_style_init(&style_pressed_color);
    lv_style_set_bg_color(&style_pressed_color, (lv_color_t)LV_COLOR_MAKE(255, 255, 255));

    /*Create a slider and add the style*/
    slider = lv_slider_create(parent);
    lv_obj_remove_style_all(slider); /*Remove the styles coming from the theme*/

    lv_obj_add_style(slider, &style_main, LV_PART_MAIN);
    lv_obj_add_style(slider, &style_indicator, LV_PART_INDICATOR);
    lv_obj_add_style(slider, &style_pressed_color, LV_PART_INDICATOR | LV_STATE_PRESSED);
    lv_obj_add_style(slider, &style_knob, LV_PART_KNOB);
    lv_obj_add_style(slider, &style_pressed_color, LV_PART_KNOB | LV_STATE_PRESSED);
    lv_obj_set_size(slider, 100, 6);
    lv_obj_set_pos(slider, 600, 10);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

static void set_slider_visibility(bool vis)
{
    ERR_RETn(!slider);
    void (*func)(lv_obj_t *, lv_obj_flag_t);
    func = vis ? lv_obj_remove_flag : lv_obj_add_flag;
    func(slider, LV_OBJ_FLAG_HIDDEN);
error_return:
    return;
}
void v_hide_slider(void)
{
    set_slider_visibility(false);
}
