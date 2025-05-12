#ifndef __V_ASSETS_LIST_H__
#define __V_ASSETS_LIST_H__

#include <stdint.h>

#define ASSETS_LIST  \
    ENTRY(hamburger) \
    ENTRY(pen)       \
    ENTRY(open_book) \
    ENTRY(select)    \
    ENTRY(eraser)    \
    ENTRY(resize)    \
    ENTRY(circle)    \
    ENTRY(prev_page) \
    ENTRY(next_page) \
    ENTRY(close_button)

#define JSON_LIST \
    JENTRY(menu)  \
    JENTRY(ja)    \
    JENTRY(en)

uint8_t *get_asset_ptr(const char *name);
uint8_t *get_json_ptr(const char *name);

#endif