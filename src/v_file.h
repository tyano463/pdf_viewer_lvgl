#ifndef __V_FILE_H__
#define __V_FILE_H__

typedef struct str_v_file v_file_t;
typedef void (*v_file_callback_t)(const char *);

typedef struct str_v_file
{
    char path[512];
    v_file_callback_t callback;
} v_file_t;

typedef enum
{
    V_FILE_DIALOG_OPEN,
    V_FILE_DIALOG_SAVE,
} v_file_dialog_mode_t;

void show_filer(v_file_callback_t callback, v_file_dialog_mode_t mode);

#endif