#ifndef __V_ICON_H__
#define __V_ICON_H__

#include <stdint.h>

uint8_t *load_icon_data(uint8_t *ico, uint8_t *w, uint8_t *h);
uint8_t *load_bmp_data(uint8_t *bmp, uint8_t *w, uint8_t *h);
void to_bmp(const char *path, const uint8_t *data, uint16_t width, uint16_t height);
lv_image_dsc_t *get_icon_dsc(const char *name);
#endif