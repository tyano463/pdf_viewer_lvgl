#ifndef __V_MISC_H__
#define __V_MISC_H__

#include <dirent.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

void dump(const uint8_t *data, size_t size);

float distance(lv_point_t *a, lv_point_t *b);

int mkdir_p(const char *path, mode_t mode);
bool ends_with_ignore_case(const char *str, const char *suffix);
bool file_exists(const char *path);
void resize_image_bicubic(uint8_t *orig_data, int orig_w, int orig_h, float scale, uint8_t *new_data);
#endif
