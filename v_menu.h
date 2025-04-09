#ifndef __V_MENU_H__
#define __V_MENU_H__

#include "v_common.h"

typedef struct
{
    int a;
} v_menu_t;

v_menu_t *v_menu_init(void);
void v_menu_update(void);

#endif