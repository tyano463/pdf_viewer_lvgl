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

typedef struct str_v_menu_cb_ops
{
    void (*file_opened)(const char *);
    void (*save_as)(const char *);
    void (*save)(void);
    void (*export_pdf)(const char *);
    void (*show_mode)(v_show_mode_t mode);
    void (*change_page)(uint16_t page);
    void (*menu_opened)(void);
    void (*menu_closed)(void);
} v_menu_cb_ops_t;

typedef struct str_v_menu_ops
{
    v_status_t (*init)(lv_obj_t *parent, v_menu_cb_ops_t *ops);
    void (*show_icon)(void);
    void (*hide_icon)(void);
    void (*set_page)(int16_t page, int16_t page_max);
    int16_t (*get_page)(void);
    void (*periodic_proc)(void);
} v_menu_ops_t;
v_menu_ops_t *v_get_menu_ops(void);

#endif