#include "v_slider.h"
#include "v_common.h"

static void set_slider_visibility(lv_obj_t *slider, bool vis);

static void slider_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target_obj(e);
    v_slider_callback_t callback = (v_slider_callback_t)lv_event_get_user_data(e);
    ERR_RETn(!slider);
    ERR_RETn(!callback);
    int32_t value = lv_slider_get_value(slider);
    if (callback)
    {
        callback(value);
    }
error_return:
    return;
}

lv_obj_t *v_create_slider(v_slider_callback_t callback, lv_obj_t *parent, lv_area_t *rect, v_slider_param_t *param)
{
    lv_obj_t *slider;
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
    lv_slider_set_range(slider, param->minimum_value, param->maximum_value);
    lv_slider_set_value(slider, param->initial_value, LV_ANIM_OFF);

    lv_obj_add_style(slider, &style_main, LV_PART_MAIN);
    lv_obj_add_style(slider, &style_indicator, LV_PART_INDICATOR);
    lv_obj_add_style(slider, &style_pressed_color, LV_PART_INDICATOR | LV_STATE_PRESSED);
    lv_obj_add_style(slider, &style_knob, LV_PART_KNOB);
    lv_obj_add_style(slider, &style_pressed_color, LV_PART_KNOB | LV_STATE_PRESSED);
    lv_obj_set_size(slider, rect->x2 - rect->x1, rect->y2 - rect->y1);
    lv_obj_set_pos(slider, rect->x1, rect->y1);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, callback);
    return slider;
}

void v_show_slider(lv_obj_t *slider)
{
    if (slider)
    {
        set_slider_visibility(slider, true);
    }
}

static void set_slider_visibility(lv_obj_t *slider, bool vis)
{
    ERR_RETn(!slider);
    void (*func)(lv_obj_t *, lv_obj_flag_t);
    func = vis ? lv_obj_remove_flag : lv_obj_add_flag;
    func(slider, LV_OBJ_FLAG_HIDDEN);
error_return:
    return;
}
void v_hide_slider(lv_obj_t *slider)
{
    if (slider)
        set_slider_visibility(slider, false);
}
