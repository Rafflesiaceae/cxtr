/*
 * CXTR_COUNTED_BY compiles on flexible array and pointer members.
 * Flexible array members need C99 and do not exist in C++.
 *
 * mode: run
 * std: c99+
 */
#include <stdlib.h>
#include "cxtr.h"

typedef struct Entity { int id; } Entity;

typedef struct EntityList {
    size_t count;
    Entity items[] CXTR_COUNTED_BY(count);
} EntityList;

typedef struct EntitySpan {
    size_t count;
    Entity *items CXTR_COUNTED_BY(count);
} EntitySpan;

int main(void)
{
    size_t i;
    int sum = 0;
    EntitySpan span = { 0, NULL };
    EntityList *list = malloc(sizeof(*list) + 3 * sizeof(Entity));
    if (!list)
        return 1;
    /* The count is set before any element access. */
    list->count = 3;
    for (i = 0; i < list->count; ++i)
        list->items[i].id = (int)i;

    span.items = list->items;
    span.count = list->count;
    for (i = 0; i < span.count; ++i)
        sum += span.items[i].id;
    free(list);
    return sum == 3 ? 0 : 1;
}
