#ifndef __V_SETTINGS_H__
#define __V_SETTINGS_H__

#include <stdio.h>
#include "v_common.h"
#include "v_pen.h"

typedef struct str_v_settings
{
    char last_opened[MAX_PATH];
    uint16_t last_opened_page;
} v_settings_t;

#endif