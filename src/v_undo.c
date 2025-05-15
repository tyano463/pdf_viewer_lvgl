#include <stddef.h>
#include <stdlib.h>
#include "v_undo.h"

#define ASSERT(c)                     \
    do                                \
    {                                 \
        if (!(c))                     \
        {                             \
            printf("#### assert \n"); \
            while (1)                 \
                ;                     \
        }                             \
    } while (0)
static undo_buffer_t ring;
void undo_init(void)
{
    ring.head = 0;
    ring.count = 0;
    ring.cursor = 0;
}

void undo_push(v_annot_t *before, v_annot_t *after, undo_action_t action)
{
    int8_t count = ring.count + ring.cursor;
    int8_t head = (ring.head + ring.cursor + MAX_HISTORY) % MAX_HISTORY;

    ring.entries[head].before = before;
    ring.entries[head].after = after;
    ring.entries[head].action = action;

    ring.cursor = 0;
    ring.head = (head + 1) % MAX_HISTORY;
    ring.count = min(count + 1, MAX_HISTORY);
}

undo_entry_t *undo_do(void)
{
    if ((ring.count + ring.cursor) == 0)
        return NULL;

    ring.cursor--;

    int8_t pos = (ring.head + ring.cursor + MAX_HISTORY) % MAX_HISTORY;
    return &ring.entries[pos];
}

undo_entry_t *redo_do(void)
{
    if (ring.cursor >= 0)
        return NULL;

    int8_t pos = (ring.head + ring.cursor + MAX_HISTORY) % MAX_HISTORY;
    ring.cursor++;
    return &ring.entries[pos];
}

void undo_clear(void)
{
    for (int i = ring.head; ring.count > 0; ring.count--)
    {
        v_annot_t **p;
        p = &ring.entries[i].before;
        if (*p)
            free(*p);
        *p = NULL;

        p = &ring.entries[i].after;
        if (*p)
            free(*p);
        *p = NULL;

        i = (i - 1 + MAX_HISTORY) % MAX_HISTORY;
    }
    undo_init();
}