#ifndef __V_COMMON_H__
#define __V_COMMON_H__

#include <string.h>
#include <lvgl/lvgl.h>

#define min(a, b) (((b) < (a)) ? (b) : (a))
#define max(a, b) (((b) > (a)) ? (b) : (a))

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define d(x, ...)                                                                     \
    do                                                                                \
    {                                                                                 \
        printf("%s(%d) %s " x "\n", __FILENAME__, __LINE__, __func__, ##__VA_ARGS__); \
        fflush(stdout);                                                               \
    } while (0)

#define ERR_RETn(c)            \
    do                         \
    {                          \
        if (c)                 \
            goto error_return; \
    } while (0)

typedef enum
{
    ST_SUCCESS,
    ST_PDF_OPEN_FAILED,
    ST_PDF_CONTEXT_CREAT_FAILED,
    ST_PDF_DOCUMENT_OPEN_FAILED,
    ST_MENU_OPEN_FAIL,
} v_status_t;

#endif