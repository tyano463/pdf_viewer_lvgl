#ifndef __V_MISC_H__
#define __V_MISC_H__

#include <dirent.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

void dump(const uint8_t *data, size_t size);

int mkdir_p(const char *path, mode_t mode);
bool ends_with_ignore_case(const char *str, const char *suffix);
bool file_exists(const char *path);
bool directory_exists(const char *path);
char *execute_command(const char *command, ...);
size_t get_file_size(const char *file);
const char *next_file_name(const char *orig);
char *get_dir_name(const char *path);
char *b2s(uint8_t *data, uint32_t len);
int64_t npow(int64_t a, int64_t n);
#endif
