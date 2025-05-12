#ifndef __V_EXT_COMMAND_H__
#define __V_EXT_COMMAND_H__

#include <stdint.h>

typedef uint8_t v_ext_command_type_t;
enum
{
    V_EXT_NONE,
    V_EXT_PAGE_NEXT,
    V_EXT_PAGE_PREV,
    V_EXT_COMMAND_MAX,
};
typedef struct
{
    uint8_t command;
    uint8_t option[511];
} v_ext_command_t;

#endif
