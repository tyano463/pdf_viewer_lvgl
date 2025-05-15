#ifndef __V_UNDO_H__
#define __V_UNDO_H__

#include <stdint.h>
#include "v_canvas.h"

#define MAX_HISTORY 10

typedef enum
{
    V_UNDO_TYPE_UNDO = 1,
    V_UNDO_TYPE_REDO,
} v_undo_type_t;
typedef enum
{
    UNDO_ACTION_NEW_INK,
    UNDO_ACTION_TRANSFORM_INK,
    UNDO_ACTION_DELETE_INK,
    UNDO_ACTION_MOVE_TEXT,
    UNDO_ACTION_DELETE_TEXT,
    UNDO_ACTION_MAX
} undo_action_t;

typedef struct
{
    v_annot_t *before;
    v_annot_t *after;
    undo_action_t action;
} undo_entry_t;

typedef struct
{
    int8_t head;
    int8_t count;
    int8_t cursor;
    int8_t dummy;
    undo_entry_t entries[MAX_HISTORY];
} undo_buffer_t;

void undo_init(void);
void undo_push(v_annot_t *before, v_annot_t *after, undo_action_t action);
undo_entry_t *undo_do(void);
undo_entry_t *redo_do(void);

#endif