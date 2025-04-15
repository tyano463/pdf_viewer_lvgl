#include "v_menu.h"
#include "v_icon.h"
#include "v_file.h"
#include "v_misc.h"

extern unsigned char assets_hamburger_bmp[];

static void create_menu(lv_obj_t *parent);
static void hide_menu(void);
static void show_menu(void);

static v_menu_t *_menu;
static v_menu_ops_t *_ops;

static void file_opend(char *path)
{
    d("%s", path);
    if (_ops && _ops->file_opened)
    {
        _ops->file_opened(path);
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
        //        show_menu();
        show_filer(file_opend);
    }

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
    static lv_style_t style;
    ERR_RETn(!_menu);

    // style = lv_malloc(sizeof(lv_style_t));
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

v_status_t v_menu_init(lv_obj_t *parent, v_menu_ops_t *ops)
{
    v_status_t status;

    _menu = lv_malloc(sizeof(v_menu_t));

    create_button(parent);

    create_menu(parent);

    _ops = ops;
    status = ST_SUCCESS;

    return status;
}

void v_menu_update(void)
{
}

static void set_menu_visible(bool visible)
{
    lv_obj_flag_t flag;
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

void create_menu(lv_obj_t *parent)
{
    lv_obj_t *menu = lv_menu_create(parent);
    lv_menu_set_mode_root_back_button(menu, LV_MENU_ROOT_BACK_BUTTON_ENABLED);
    lv_obj_add_event_cb(menu, back_event_handler, LV_EVENT_CLICKED, menu);
    lv_obj_set_size(menu, lv_display_get_horizontal_resolution(NULL), lv_display_get_vertical_resolution(NULL));
    lv_obj_center(menu);

    lv_obj_t *cont;
    lv_obj_t *label;

    /*Create a sub page*/
    lv_obj_t *sub_page = lv_menu_page_create(menu, NULL);

    cont = lv_menu_cont_create(sub_page);
    label = lv_label_create(cont);
    lv_label_set_text(label, "Hello, I am hiding here");

    /*Create a main page*/
    lv_obj_t *main_page = lv_menu_page_create(menu, NULL);

    cont = lv_menu_cont_create(main_page);
    label = lv_label_create(cont);
    lv_label_set_text(label, "Item 1");

    cont = lv_menu_cont_create(main_page);
    label = lv_label_create(cont);
    lv_label_set_text(label, "Item 2");

    cont = lv_menu_cont_create(main_page);
    label = lv_label_create(cont);
    lv_label_set_text(label, "Item 3 (Click me!)");
    lv_menu_set_load_page_event(menu, cont, sub_page);

    lv_menu_set_page(menu, main_page);

    _menu->menu = menu;
    hide_menu();
}