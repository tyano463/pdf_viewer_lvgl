#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "lvgl/lvgl.h"

#include "v_file.h"
#include "v_common.h"

static void do_hide_filer(void);
static void do_show_filer(void);

static v_file_t _file;
static lv_obj_t *file_explorer;
static void file_explorer_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    if (code == LV_EVENT_VALUE_CHANGED)
    {
        char *p;
        const char *cur_path = lv_file_explorer_get_current_path(obj);
        const char *sel_fn = lv_file_explorer_get_selected_file_name(obj);
        LV_LOG_USER("%s%s", cur_path, sel_fn);
        if (_file.callback)
        {
            if (cur_path[0] == LV_FS_DEFAULT_DRIVER_LETTER && cur_path[1] == ':') {
                p = &cur_path[2];
            } else {
                p = cur_path;
            }
            sprintf(_file.path, "%s%s", p, sel_fn);
            do_hide_filer();
            _file.callback(_file.path);
        }
    }
}

static void close_btn_event_cb(lv_event_t *e)
{
    do_hide_filer();
}

void lv_example_file_explorer_1(void)
{
    file_explorer = lv_file_explorer_create(lv_screen_active());
    lv_file_explorer_set_sort(file_explorer, LV_EXPLORER_SORT_KIND);

#if LV_USE_FS_WIN32
    /* Note to Windows users:  the initial "C:" on these paths corresponds to
     * the value of `LV_FS_WIN32_LETTER` in `lv_conf.h`, and should not be
     * confused with the Windows/DOS drive letter.  It is an identifier that
     * is used to enable LVGL to look up the appropriate driver from a list of
     * registered file-system drivers.  `lv_fs_win32_init()` happens to use the
     * identifier letter 'C' so "C:" is the driver-identifier-prefix used here.
     * The "C:" following that is indeed the Windows/DOS drive letter and is
     * part of the actual path that gets passed to the OS-level functions.
     *
     * See https://docs.lvgl.io/master/details/main-components/fs.html for details.
     * File Explorer uses `lv_fs` internally, thus the required prefix in path strings.
     */
    lv_file_explorer_open_dir(file_explorer, "C:C:/");
#if LV_FILE_EXPLORER_QUICK_ACCESS
    lv_file_explorer_set_quick_access_path(file_explorer, LV_EXPLORER_HOME_DIR, "C:C:/Users/Public/Desktop");
    lv_file_explorer_set_quick_access_path(file_explorer, LV_EXPLORER_VIDEO_DIR, "C:C:/Users/Public/Videos");
    lv_file_explorer_set_quick_access_path(file_explorer, LV_EXPLORER_PICTURES_DIR, "C:C:/Users/Public/Pictures");
    lv_file_explorer_set_quick_access_path(file_explorer, LV_EXPLORER_MUSIC_DIR, "C:C:/Users/Public/Music");
    lv_file_explorer_set_quick_access_path(file_explorer, LV_EXPLORER_DOCS_DIR, "C:C:/Users/Public/Documents");
    lv_file_explorer_set_quick_access_path(file_explorer, LV_EXPLORER_FS_DIR, "C:C:/");
#endif

#else
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
    strcat(video_dir, "/Videos");
    lv_file_explorer_set_quick_access_path(file_explorer, LV_EXPLORER_VIDEO_DIR, video_dir);
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
#endif
    lv_obj_t *header = lv_file_explorer_get_header(file_explorer);
    lv_obj_t *close_btn = lv_btn_create(header);
    lv_obj_set_size(close_btn, 40, 30);
    lv_obj_align(close_btn, LV_ALIGN_TOP_RIGHT, -4, 4);
    lv_obj_add_event_cb(close_btn, close_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label = lv_label_create(close_btn);
    lv_label_set_text(label, "x");
    lv_obj_center(label);

    lv_obj_add_event_cb(file_explorer, file_explorer_event_handler, LV_EVENT_ALL, NULL);
}

void show_filer(v_file_callback_t callback)
{
    _file.callback = callback;

    if (!file_explorer)
        lv_example_file_explorer_1();

    do_show_filer();
}

static void set_filer_visibility(bool visibility)
{
    if (!file_explorer)
        return;

    void (*func)(lv_obj_t *, lv_obj_flag_t);

    lv_obj_flag_t flag = LV_OBJ_FLAG_HIDDEN;
    func = visibility ? lv_obj_remove_flag : lv_obj_add_flag;

    func(file_explorer, flag);
}
static void do_show_filer(void)
{
    set_filer_visibility(true);
}
static void do_hide_filer(void)
{
    set_filer_visibility(false);
}
