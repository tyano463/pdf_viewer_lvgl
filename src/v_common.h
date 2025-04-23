#ifndef __V_COMMON_H__
#define __V_COMMON_H__

#include <string.h>
#include <lvgl/lvgl.h>

#define APP_NAME "pdf_viewer_lvgl"
#define WIDTH 1024
#define HEIGHT 768

#ifndef min
#define min(a, b) (((b) < (a)) ? (b) : (a))
#define max(a, b) (((b) > (a)) ? (b) : (a))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define d(x, ...)                                                                          \
    do                                                                                     \
    {                                                                                      \
        v_log_write("%s(%d) %s " x "\n", __FILENAME__, __LINE__, __func__, ##__VA_ARGS__); \
    } while (0)

#define ERR_RETn(c)            \
    do                         \
    {                          \
        if (c)                 \
            goto error_return; \
    } while (0)

#define ERR_RET(c, s, ...)                                                                     \
    do                                                                                         \
    {                                                                                          \
        if (c)                                                                                 \
        {                                                                                      \
            v_log_write("%s(%d) %s " s "\n", __FILENAME__, __LINE__, __func__, ##__VA_ARGS__); \
            goto error_return;                                                                 \
        }                                                                                      \
    } while (0)

typedef enum
{
    MODE_NORMAL,
    MODE_PEN,
} v_mode_t;

typedef uint8_t v_annot_display_t;
enum
{
    V_ANNOT_SHOW,
    V_ANNOT_HIDE,
};

typedef uint8_t v_format_t;
enum
{
    V_FORMAT_PDF,
    V_FORMAT_SVG,
    V_FORMAT_PNG,
    V_FORMAT_JPEG,
    V_FORMAT_MUSICXML,
    V_FORMAT_MIDI,
    V_FORMAT_MAX,
};

typedef struct
{
    float sx;
    float sy;
} v_scale_t;

typedef enum
{
    ST_SUCCESS,
    ST_SETTING_LOAD_FAILED,
    ST_SETTING_SAVE_FAILED,
    ST_SETTING_PARSE_FAILED,
    ST_SETTING_SERIALIZE_FAILED,
    ST_CREATE_CANVAS_FAILED,
    ST_LOG_INIT_FAILED,
    ST_LOG_WRITE_FAILED,
    ST_LOG_ROTATE_FAILED,
    ST_DISPLAY_INIT_FAILED,
    ST_PDF_OPEN_FAILED,
    ST_PDF_CONTEXT_CREAT_FAILED,
    ST_PDF_DOCUMENT_OPEN_FAILED,
    ST_PNG_OPEN_FAILED,
    ST_MXL_OPEN_FAILED,
    ST_JPEG_OPEN_FAILED,
    ST_MENU_OPEN_FAIL,
} v_status_t;

#include "v_log.h"

#endif