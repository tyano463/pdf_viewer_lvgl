#ifndef __V_FILE_H__
#define __V_FILE_H__

typedef struct str_v_file v_file_t;
typedef void (*v_file_callback_t)(char *);

typedef struct str_v_file
{
    char path[512];
    v_file_callback_t callback;
} v_file_t;

void show_filer(v_file_callback_t callback);

#endif