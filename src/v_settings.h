#ifndef __V_SETTINGS_H__
#define __V_SETTINGS_H__

#include <stdio.h>
#include "v_common.h"
#include "v_pen.h"

typedef struct str_v_settings
{
    char last_opened[MAX_PATH];
    uint16_t last_opened_page;
    v_pen_t last_pen;
    v_annot_display_t annot;
    char lang[4];
} v_settings_t;

typedef struct str_v_settings_ops
{
    void (*set_path)(const char *path);
    const char *(*get_path)(void);
    void (*set_page)(uint16_t page);
    uint16_t (*get_page)(void);
    void (*set_annot_mode)(v_annot_display_t mode);
    v_annot_display_t (*get_annot_mode)(void);
    void (*set_pen)(const v_pen_t *pen);
    v_pen_t *(*get_pen)(void);
    void (*set_lang)(const char *lang);
    const char * (*get_lang)(void);
} v_settings_ops_t;

#define V_S_ITEM_LAST_PATH "last_path"
#define V_S_ITEM_LAST_PAGE "last_page"
#define V_S_ITEM_LAST_PEN "last_pen"
#define V_S_ITEM_ANNOT "annot"
#define V_S_ITEM_LANG "lang"

v_status_t v_load_settings(void);
v_status_t v_save_settings(void);
v_settings_ops_t *v_get_settings_ops(void);

#endif