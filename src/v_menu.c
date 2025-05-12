#include <cjson/cJSON.h>
#include <math.h>
#include "v_menu.h"
#include "v_icon.h"
#include "v_file.h"
#include "v_misc.h"
#include "v_assets_list.h"
#include "v_settings.h"

#define MENU_JSON "menu"

extern unsigned char assets_hamburger_bmp[];

static cJSON *load_menu_settings(void);
static void create_menu(lv_obj_t *parent, cJSON *json);
static void hide_menu(void);
static void show_menu(void);
static void set_page(int16_t page, int16_t page_max);

static v_menu_t *_menu;
static v_menu_cb_ops_t *_ops;
static v_menu_ops_t g_menu_ops;
static v_show_mode_t g_show_mode;
static lv_obj_t *show_mode_button;
static lv_obj_t *page_control;
static lv_obj_t *page_indicator;
static int16_t current_page;
static int16_t current_page_max;
static char s_page[4];

static void open_file(const char *path)
{
    d("%s", path);
    if (_ops && _ops->file_opened)
    {
        _ops->file_opened(path);
    }
}

static void save_file(const char *_dummy)
{
    (void)_dummy;

    if (_ops && _ops->save)
    {
        _ops->save();
    }
}
static void save_file_as(const char *file)
{
    if (_ops && _ops->save_as)
    {
        _ops->save_as(file);
    }
}
static void export_pdf(const char *file)
{
    if (_ops && _ops->export_pdf)
    {
        _ops->export_pdf(file);
    }
}

static void event_handler(lv_event_t *e)
{
    d("clicked");
    ERR_RETn(!_menu || !_menu->menu);

    if (_menu->shown)
    {
        hide_menu();
    }
    else
    {
        show_menu();
    }
    //        show_filer(file_opend);

error_return:
    return;
}

static lv_img_dsc_t *hamburger_icon(void)
{
    lv_img_dsc_t *img;
    uint8_t w, h;
    img = lv_malloc(sizeof(lv_img_dsc_t));
    img->data = (const uint8_t *)load_bmp_data(assets_hamburger_bmp, &w, &h);
    d("assets:%p (%d x %d * 3= %d)", assets_hamburger_bmp, w, h, w * h * 4);
    img->header.magic = LV_IMAGE_HEADER_MAGIC;
    img->data_size = w * h * 4;
    img->header.stride = w * 4;
    img->header.flags = 0;
    img->header.w = w;
    img->header.h = h;
    img->header.cf = LV_COLOR_FORMAT_ARGB8888;
    //    dump(img->data, 4096);
    return img;
}

static void create_button(lv_obj_t *parent)
{
    lv_img_dsc_t *imgdsc;
    ERR_RETn(!_menu);

    _menu->btn = lv_image_create(parent);
    ERR_RETn(!_menu->btn);

    imgdsc = hamburger_icon();
    ERR_RETn(!imgdsc);

    lv_obj_flag_t flag = LV_OBJ_FLAG_CLICKABLE;
    lv_obj_add_flag(_menu->btn, flag);

    d("btn:%p imgdsc:%p", _menu->btn, imgdsc);
    lv_image_set_src(_menu->btn, imgdsc);
    lv_obj_set_pos(_menu->btn, 0, 0);
    lv_obj_set_size(_menu->btn, 64, 64);

    lv_obj_add_event_cb(_menu->btn, event_handler, LV_EVENT_SINGLE_CLICKED, NULL);
error_return:
    d("");
    return;
}

static uint8_t show_mode_buf[32 * 32 * 4];
static void update_show_mode_button(void)
{
#define SHOW_MODE_ICON_SIZE 32
    static lv_img_dsc_t dsc;
    lv_color32_t basecolor;
    dsc.data = show_mode_buf;
    dsc.data_size = 32 * 32 * 4;
    dsc.reserved = 0;
    dsc.reserved_2 = 0;
    dsc.header.cf = LV_COLOR_FORMAT_ARGB8888;
    dsc.header.flags = 0;
    dsc.header.h = 32;
    dsc.header.w = 32;
    dsc.header.stride = 32 * 4;
    dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    if (g_show_mode == V_SHOW_MODE_ANNOT_WITH_MENU)
    {
        basecolor.red = 165;
        basecolor.green = 245;
        basecolor.blue = 159;
        basecolor.alpha = 255;
    }
    else if (g_show_mode == V_SHOW_MODE_ANNOT_NO_MENU)
    {
        basecolor.red = 240;
        basecolor.green = 235;
        basecolor.blue = 197;
        basecolor.alpha = 255;
    }
    else if (g_show_mode == V_SHOW_MODE_SCORE_ONLY)
    {
        basecolor.red = 219;
        basecolor.green = 219;
        basecolor.blue = 213;
        basecolor.alpha = 255;
    }
    float cx = SHOW_MODE_ICON_SIZE / 2.0f;
    float cy = SHOW_MODE_ICON_SIZE / 2.0f;
    float radius = SHOW_MODE_ICON_SIZE / 2.0f;
    for (int i = 0; i < SHOW_MODE_ICON_SIZE; i++)
    {
        for (int j = 0; j < SHOW_MODE_ICON_SIZE; j++)
        {

            float dx = j - cx;
            float dy = i - cy;
            float dist = sqrtf(dx * dx + dy * dy);
            // TODO
            uint8_t alpha = 0;
            if (dist <= radius)
            {
                alpha = 128 + (radius - dist) / radius * (255 - 128);
            }

            show_mode_buf[(i * SHOW_MODE_ICON_SIZE + j) * 4 + 3] = alpha;
            show_mode_buf[(i * SHOW_MODE_ICON_SIZE + j) * 4 + 2] = basecolor.red;
            show_mode_buf[(i * SHOW_MODE_ICON_SIZE + j) * 4 + 1] = basecolor.green;
            show_mode_buf[(i * SHOW_MODE_ICON_SIZE + j) * 4 + 0] = basecolor.blue;
        }
    }
    lv_image_set_src(show_mode_button, &dsc);
}
static void mode_changed(lv_event_t *e)
{
    g_show_mode++;
    g_show_mode %= V_SHOW_MODE_MAX;
    d("%s(%d)", v_show_mode_t_to_string(g_show_mode), g_show_mode);
    update_show_mode_button();
    _ops->show_mode(g_show_mode);
}

static void page_move(lv_event_t *e)
{
    int8_t direction = (int8_t)(intptr_t)lv_event_get_user_data(e);
    d("cur:%d %d", current_page, direction);
    if (!direction)
    {
        lv_obj_t *slider = lv_event_get_target_obj(e);
        int16_t value = lv_slider_get_value(slider);
        d("page:%d", value);
        set_page(value, current_page_max);
    }
    else
    {
        set_page(current_page + direction, current_page_max);
    }

    _ops->change_page(current_page);
}

static void page_button(lv_obj_t *parent)
{

    lv_image_dsc_t *prev_image = get_icon_dsc("prev_page");
    lv_image_dsc_t *next_image = get_icon_dsc("next_page");
    page_control = lv_obj_create(parent);
    lv_obj_remove_style_all(page_control);
    lv_obj_set_flex_flow(page_control, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(page_control, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_size(page_control, 200, 40);
    lv_obj_align(page_control, LV_ALIGN_BOTTOM_MID, 0, -10);

    page_indicator = lv_label_create(page_control);
    lv_obj_remove_style_all(page_indicator);
    sprintf(s_page, "%d", 1);
    lv_label_set_text(page_indicator, s_page);
    lv_obj_set_user_data(page_control, page_indicator);

    lv_obj_t *container = lv_obj_create(page_control);
    lv_obj_remove_style_all(container);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_size(container, 200, 40);
    lv_obj_align(container, LV_ALIGN_BOTTOM_MID, 0, -10);

    lv_obj_t *prev_img = lv_img_create(container);
    lv_img_set_src(prev_img, prev_image);
    lv_obj_add_flag(prev_img, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(prev_img, page_move, LV_EVENT_SINGLE_CLICKED, (void *)(intptr_t)-1);

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

    lv_obj_t *slider = lv_slider_create(container);
    lv_obj_remove_style_all(slider);
    lv_slider_set_range(slider, 1, 10);
    lv_slider_set_value(slider, 1, LV_ANIM_OFF);

    lv_obj_add_style(slider, &style_main, LV_PART_MAIN);
    lv_obj_add_style(slider, &style_indicator, LV_PART_INDICATOR);
    lv_obj_add_style(slider, &style_pressed_color, LV_PART_INDICATOR | LV_STATE_PRESSED);
    lv_obj_add_style(slider, &style_knob, LV_PART_KNOB);
    lv_obj_add_style(slider, &style_pressed_color, LV_PART_KNOB | LV_STATE_PRESSED);
    lv_obj_set_size(slider, 80, 6);
    lv_obj_add_event_cb(slider, page_move, LV_EVENT_VALUE_CHANGED, 0);

    lv_obj_t *next_img = lv_img_create(container);
    lv_img_set_src(next_img, next_image);
    lv_obj_add_flag(next_img, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(next_img, page_move, LV_EVENT_SINGLE_CLICKED, (void *)(intptr_t)1);
    lv_obj_set_style_pad_column(container, 12, 0);
}

static void switch_button(lv_obj_t *parent)
{
    //    lv_img_dsc_t *dsc = get_icon_dsc("circle");
    show_mode_button = lv_image_create(parent);
    //    lv_image_set_src(show_mode_button, dsc);
    update_show_mode_button();
    lv_obj_align(show_mode_button, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_add_flag(show_mode_button, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(show_mode_button, mode_changed, LV_EVENT_SINGLE_CLICKED, NULL);
}

static v_status_t v_menu_init(lv_obj_t *parent, v_menu_cb_ops_t *ops)
{
    v_status_t status;

    _menu = lv_malloc(sizeof(v_menu_t));

    create_button(parent);

    cJSON *json = load_menu_settings();
    create_menu(parent, json);

    switch_button(parent);

    page_button(parent);

    _ops = ops;
    g_show_mode = V_SHOW_MODE_ANNOT_WITH_MENU;
    status = ST_SUCCESS;

    return status;
}

static void set_icon_visibiliry(bool vis)
{
    ERR_RETn(!_menu || !_menu->btn);

    void (*func)(lv_obj_t *, lv_obj_flag_t) = vis ? lv_obj_remove_flag : lv_obj_add_flag;

    func(_menu->btn, LV_OBJ_FLAG_HIDDEN);
error_return:
    return;
}

static void hide_icon(void)
{
    set_icon_visibiliry(false);
}
static void show_icon(void)
{
    set_icon_visibiliry(true);
}

static void set_page(int16_t _page, int16_t page_max)
{
    current_page_max = page_max;
    d("page:%d", _page);
    _page = max(1, _page);
    _page = min(_page, page_max);

    ERR_RETn(_page == current_page);
    current_page = _page;
    uint16_t page = current_page;
    snprintf(s_page, sizeof(s_page), "%d", page % 1000);
    d("%s", s_page);
    if (page_indicator)
        lv_label_set_text(page_indicator, s_page);

error_return:
    return;
}

static void init_ops(void)
{
    if (!g_menu_ops.init)
    {
        g_menu_ops.init = v_menu_init;
        g_menu_ops.show_icon = show_icon;
        g_menu_ops.hide_icon = hide_icon;
        g_menu_ops.set_page = set_page;
    }
}

v_menu_ops_t *v_get_menu_ops(void)
{
    init_ops();
    return &g_menu_ops;
}

static void set_menu_visible(bool visible)
{
    void (*func)(lv_obj_t *obj, lv_obj_flag_t f);

    ERR_RETn(!_menu || !_menu->menu);

    func = visible ? lv_obj_remove_flag : lv_obj_add_flag;
    func(_menu->menu, LV_OBJ_FLAG_HIDDEN);

    _menu->shown = visible;
    d("menu shown:%d", visible);
error_return:
    return;
}

static void show_menu(void)
{
    set_menu_visible(true);
}

static void hide_menu(void)
{
    set_menu_visible(false);
}

static void back_event_handler(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target_obj(e);
    lv_obj_t *menu = (lv_obj_t *)lv_event_get_user_data(e);

    if (lv_menu_back_button_is_root(menu, obj))
    {
        hide_menu();
    }
}

static cJSON *get_lang_json(void)
{
    v_settings_ops_t *ops = v_get_settings_ops();
    const char *lang = ops->get_lang();
    const char *json_str = (const char *)get_json_ptr(lang);
    return cJSON_Parse(json_str);
}

static const char *translate(cJSON *lang, const char *key)
{
    cJSON *label = cJSON_GetObjectItem(lang, key);
    if (cJSON_IsString(label))
    {
        d("v: %s", label->valuestring);
        return label->valuestring;
    }
    else
    {
        d("");
        return key;
    }
}

static void button_callback(lv_event_t *e)
{
    const char *key = (const char *)lv_event_get_user_data(e);
    d("key: %s", key);
    if (strcmp(key, "open_file") == 0)
    {
        show_filer(open_file, V_FILE_DIALOG_OPEN);
    }
    else if (strcmp(key, "save_file") == 0)
    {
        save_file(NULL);
    }
    else if (strcmp(key, "save_file_as") == 0)
    {
        show_filer(save_file_as, V_FILE_DIALOG_SAVE);
    }
    else if (strcmp(key, "export_pdf") == 0)
    {
        show_filer(export_pdf, V_FILE_DIALOG_SAVE);
    }

    hide_menu();
}

static void create_menu(lv_obj_t *parent, cJSON *menu_json)
{
    cJSON *lang = get_lang_json();
    d("menu:%p lang:%p", menu_json, lang);

    lv_obj_t *menu = lv_menu_create(parent);
    lv_menu_set_mode_root_back_button(menu, LV_MENU_ROOT_BACK_BUTTON_ENABLED);
    lv_obj_add_event_cb(menu, back_event_handler, LV_EVENT_CLICKED, menu);
    lv_obj_set_size(menu, lv_display_get_horizontal_resolution(NULL), lv_display_get_vertical_resolution(NULL));
    lv_obj_center(menu);

    lv_obj_t *cont;
    lv_obj_t *label;

    /*Create a sub page*/
    //    lv_obj_t *sub_page = lv_menu_page_create(menu, NULL);
    //
    //    cont = lv_menu_cont_create(sub_page);
    //    label = lv_label_create(cont);
    //    lv_label_set_text(label, "Hello, I am hiding here");

    /*Create a main page*/
    lv_obj_t *main_page = lv_menu_page_create(menu, NULL);

    cJSON *menu_section;
    cJSON_ArrayForEach(menu_section, menu_json)
    {
        const char *section_name = menu_section->string;
        d("Section name: %s", section_name);

        if (!cJSON_IsArray(menu_section))
            continue;

        cJSON *entry = NULL;
        cJSON_ArrayForEach(entry, menu_section)
        {
            cJSON *item = cJSON_GetObjectItem(entry, "item");
            if (cJSON_IsString(item))
            {
                d(" - item: %s", item->valuestring);
                cont = lv_menu_cont_create(main_page);
                label = lv_label_create(cont);
                const char *text = translate(lang, item->valuestring);
                lv_label_set_text(label, text);
                lv_obj_add_flag(cont, LV_OBJ_FLAG_CLICKABLE);
                lv_obj_add_event_cb(cont, button_callback, LV_EVENT_SINGLE_CLICKED, item->valuestring);
            }
        }
    }
    //    lv_label_set_text(label, "Item 1");
    //
    //    cont = lv_menu_cont_create(main_page);
    //    label = lv_label_create(cont);
    //    lv_label_set_text(label, "Item 2");
    //
    //    cont = lv_menu_cont_create(main_page);
    //    label = lv_label_create(cont);
    //    lv_label_set_text(label, "Item 3 (Click me!)");
    //    lv_menu_set_load_page_event(menu, cont, sub_page);

    lv_menu_set_page(menu, main_page);

    _menu->menu = menu;
    hide_menu();
}

static cJSON *load_menu_settings(void)
{
    const char *menu_str = (const char *)get_json_ptr(MENU_JSON);
    d("menu:%p", menu_str);
    return cJSON_Parse(menu_str);
}