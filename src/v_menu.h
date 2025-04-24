#ifndef __V_MENU_H__
#define __V_MENU_H__

#include "v_common.h"
#include "lvgl/lvgl.h"

typedef struct str_v_menu
{
    lv_obj_t *btn;
    lv_obj_t *menu;
    bool shown;
} v_menu_t;

typedef struct str_v_menu_ops
{
    void (*file_opened)(const char *);
    void (*save_as)(const char *);
    void (*save)(void);
    void (*export_pdf)(const char *);
    void (*show_mode)(v_show_mode_t mode);

} v_menu_ops_t;

v_status_t v_menu_init(lv_obj_t *parent, v_menu_ops_t *ops);
void v_menu_update(void);

#endif