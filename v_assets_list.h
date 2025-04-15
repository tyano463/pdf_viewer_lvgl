#ifndef __V_ASSETS_LIST_H__
#define __V_ASSETS_LIST_H__

#include <stdint.h>

#define ASSETS_LIST \
    ENTRY(hamburger) \
    ENTRY(pen24)

uint8_t *get_asset_ptr(const char *name);

#endif