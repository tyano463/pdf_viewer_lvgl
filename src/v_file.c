#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "lvgl/lvgl.h"

#include "v_file.h"
#include "v_common.h"
#include "v_icon.h"

static void do_hide_filer(v_file_dialog_mode_t mode);
static void do_show_filer(v_file_dialog_mode_t mode);

static v_file_t _file;
static lv_obj_t *base;
static lv_obj_t *save_area;
static lv_obj_t *input;
static v_file_dialog_mode_t g_mode;
static void file_explorer_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    if (code == LV_EVENT_VALUE_CHANGED)
    {
        d("code: %s(%d)", lv_event_code_get_name(code), code);
        const char *p;
        const char *cur_path = lv_file_explorer_get_current_path(obj);
        const char *sel_fn = lv_file_explorer_get_selected_file_name(obj);
        LV_LOG_USER("%s%s", cur_path, sel_fn);
        if (_file.callback)
        {
            if (cur_path[0] == LV_FS_DEFAULT_DRIVER_LETTER && cur_path[1] == ':')
            {
                p = &cur_path[2];
            }
            else
            {
                p = cur_path;
            }
            sprintf(_file.path, "%s%s", p, sel_fn);
            if (g_mode == V_FILE_DIALOG_OPEN)
            {
                do_hide_filer(g_mode);
                _file.callback(_file.path);
            }
            else if (g_mode == V_FILE_DIALOG_SAVE)
            {
                lv_textarea_set_text(input, _file.path);
            }
        }
    }
    else if (g_mode == V_FILE_DIALOG_SAVE && code == LV_EVENT_READY)
    {
        const char *param = (const char *)lv_event_get_param(e);
        lv_textarea_set_text(input, param);
    }
}

static void save_as(lv_event_t *e)
{
    const char *text = lv_textarea_get_text(input);
    do_hide_filer(g_mode);
    _file.callback(text);
}
static void close_btn_event_cb(lv_event_t *e)
{
    do_hide_filer(g_mode);
}

static lv_obj_t *get_base(void)
{
    lv_obj_t *b = lv_obj_create(lv_screen_active());
    int w, h;
    w = lv_obj_get_width(lv_screen_active());
    h = lv_obj_get_height(lv_screen_active());
    lv_obj_set_width(b, w);
    lv_obj_set_height(b, h);
    lv_obj_set_flex_flow(b, LV_FLEX_FLOW_COLUMN);
    return b;
}
void open_file_dialog(void)
{
    base = get_base();

    save_area = lv_obj_create(base);
    lv_obj_remove_style_all(save_area);
    lv_obj_set_flex_flow(save_area, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(save_area, 0, 0);
    lv_obj_set_flex_align(save_area, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_size(save_area, lv_pct(100), 64);
    lv_obj_set_style_pad_column(save_area, 8, 0);

    input = lv_textarea_create(save_area);
    lv_textarea_set_placeholder_text(input, "File name here...");
    lv_obj_set_size(input, lv_pct(80), lv_pct(100));

    lv_obj_t *save_btn = lv_button_create(save_area);
    lv_obj_t *save_btn_label = lv_label_create(save_btn);
    lv_obj_add_event_cb(save_btn, save_as, LV_EVENT_SINGLE_CLICKED, NULL);
    lv_label_set_text(save_btn_label, "Save");
    lv_obj_center(save_btn_label);

    lv_obj_t *file_explorer = lv_file_explorer_create(base);
    lv_file_explorer_set_sort(file_explorer, LV_EXPLORER_SORT_KIND);

    /* linux */
    char *envvar = "HOME";
    char home_dir[LV_FS_MAX_PATH_LENGTH];
    strcpy(home_dir, "A:");
    /* get the user's home directory from the HOME environment variable*/
    strcat(home_dir, getenv(envvar));
    LV_LOG_USER("home_dir: %s\n", home_dir);

    lv_file_explorer_open_dir(file_explorer, home_dir);

#if LV_FILE_EXPLORER_QUICK_ACCESS

    lv_file_explorer_set_quick_access_path(file_explorer, LV_EXPLORER_HOME_DIR, home_dir);
    char video_dir[LV_FS_MAX_PATH_LENGTH];
    strcpy(video_dir, home_dir);
    strcat(video_dir, "/Downloads");
    lv_file_explorer_set_quick_access_path(file_explorer, LV_EXPLORER_DOWNLOADS_DIR, video_dir);
    char picture_dir[LV_FS_MAX_PATH_LENGTH];
    strcpy(picture_dir, home_dir);
    strcat(picture_dir, "/Pictures");
    lv_file_explorer_set_quick_access_path(file_explorer, LV_EXPLORER_PICTURES_DIR, picture_dir);
    char music_dir[LV_FS_MAX_PATH_LENGTH];
    strcpy(music_dir, home_dir);
    strcat(music_dir, "/Music");
    lv_file_explorer_set_quick_access_path(file_explorer, LV_EXPLORER_MUSIC_DIR, music_dir);
    char document_dir[LV_FS_MAX_PATH_LENGTH];
    strcpy(document_dir, home_dir);
    strcat(document_dir, "/Documents");
    lv_file_explorer_set_quick_access_path(file_explorer, LV_EXPLORER_DOCS_DIR, document_dir);

    d("");
    lv_file_explorer_set_quick_access_path(file_explorer, LV_EXPLORER_FS_DIR, "A:/");
#endif
    lv_obj_t *header = lv_file_explorer_get_header(file_explorer);
    lv_obj_t *close_btn = lv_image_create(header);
    lv_img_dsc_t *dsc = get_icon_dsc("close_button");
    d("dsc:%p");
    lv_image_set_src(close_btn, dsc);
    lv_obj_add_flag(close_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(close_btn, 64, 64);
    lv_obj_set_align(close_btn, LV_ALIGN_TOP_RIGHT);
    lv_obj_add_event_cb(close_btn, close_btn_event_cb, LV_EVENT_SINGLE_CLICKED, NULL);

    lv_obj_add_event_cb(file_explorer, file_explorer_event_handler, LV_EVENT_ALL, NULL);
}

void show_filer(v_file_callback_t callback, v_file_dialog_mode_t mode)
{
    _file.callback = callback;
    g_mode = mode;

    if (!base)
        open_file_dialog();

    do_show_filer(mode);
}

static void set_filer_visibility(v_file_dialog_mode_t mode, bool visibility)
{
    if (!base)
        return;

    void (*func)(lv_obj_t *, lv_obj_flag_t);
    int32_t height;

    func = visibility ? lv_obj_remove_flag : lv_obj_add_flag;

    func(base, LV_OBJ_FLAG_HIDDEN);

    height = (mode == V_FILE_DIALOG_OPEN) ? 0 : 64;
    lv_obj_set_height(save_area, height);
}
static void do_show_filer(v_file_dialog_mode_t mode)
{
    set_filer_visibility(mode, true);
}
static void do_hide_filer(v_file_dialog_mode_t mode)
{
    set_filer_visibility(mode, false);
}
