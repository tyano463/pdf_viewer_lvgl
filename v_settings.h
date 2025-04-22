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
} v_settings_t;

v_status_t v_load_settings(void);
const char *v_last_open_file(void);
uint16_t *v_last_open_page(void);
v_status_t v_last_open_pen(v_pen_t* pen);
v_annot_display_t v_last_display_annot(void);
v_status_t v_save_settings(void);
void v_set_pen(v_pen_t* pen);
void v_set_page(uint16_t page);
void v_set_path(const char *path);
#endif