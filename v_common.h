#ifndef __V_COMMON_H__
#define __V_COMMON_H__

#include <string.h>
#include <lvgl/lvgl.h>


#define min(a, b) (((b) < (a)) ? (b) : (a))
#define max(a, b) (((b) > (a)) ? (b) : (a))

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

typedef enum
{
    MODE_NORMAL,
    MODE_PEN,
} v_mode_t;

typedef enum
{
    ST_SUCCESS,
    ST_LOG_INIT_FAILED,
    ST_LOG_WRITE_FAILED,
    ST_LOG_ROTATE_FAILED,
    ST_DISPLAY_INIT_FAILED,
    ST_PDF_OPEN_FAILED,
    ST_PDF_CONTEXT_CREAT_FAILED,
    ST_PDF_DOCUMENT_OPEN_FAILED,
    ST_MENU_OPEN_FAIL,
} v_status_t;

#include "v_log.h"

#endif