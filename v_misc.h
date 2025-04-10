#ifndef __V_MISC_H__
#define __V_MISC_H__

#include <dirent.h>
#include <stdint.h>

void dump(const uint8_t *data, size_t size);
int directory_path(char *path, DIR* dir);

float distance(lv_point_t *a, lv_point_t *b);

#endif