#include <stdlib.h>
#include <string.h>
#include "system/log.h"
#include "utils.h"

/* ARRAY LIST */
void array_list_init(struct array_list *list, size_t initial_alloc_len, size_t elem_size)
{
    list->len           = 0;
    list->allocated_len = initial_alloc_len;
    list->elements      = malloc(elem_size * initial_alloc_len);
}
void array_list_deinit(struct array_list *list)
{
    free(list->elements);
}
void *array_list_emplace(struct array_list *list, size_t len, size_t elem_size)
{
    list->len += len;
    if (list->len > list->allocated_len) {
        list->allocated_len *= 2;
        list->elements = realloc(list->elements, elem_size * list->allocated_len);
    }

    return (char *)list->elements + (list->len - len) * elem_size;
}
void array_list_append(struct array_list *list, const void *items, size_t len, size_t elem_size)
{
    memcpy(array_list_emplace(list, len, elem_size), items, elem_size * len);
}
void array_list_remove_sft(struct array_list *list, const size_t *indices, size_t len, size_t elem_size)
{
    int i, advance = 0;
    char *marked = calloc(list->len, sizeof(char));

    if (!list->len) {
        LOG("ARLST: (warn) Tried to remove_sft an empty list");
        free(marked);
        return;
    }

    for (i = 0; i < len; i++) {
        marked[indices[i]] = 1;
    }
    i = 0;
    while (i + advance < list->len) {
        if(marked[i + advance]) {
            advance++;
            continue;
        }
        memcpy((char *)list->elements + i * elem_size, (char *)list->elements + (i + advance) * elem_size, elem_size);
        i++;
    }
    list->len -= len;
    free(marked);
}
void array_list_remove_swp(struct array_list *list, const size_t *indices, size_t len, size_t elem_size)
{
    int i, swap_index = list->len;
    char *marked = calloc(list->len, sizeof(char));

    if (!list->len) {
        LOG("ARLST: (warn) Tried to remove_swp an empty list");
        free(marked);
        return;
    }

    for (i = 0; i < len; i++) {
        marked[indices[i]] = 1;
    }
    for (i = 0; i < len; i++) {
        if (indices[i] >= list->len - len)
            continue;

        do swap_index--;
        while (marked[swap_index]);
        memcpy((char *)list->elements + indices[i] * elem_size, (char *)list->elements + swap_index * elem_size, elem_size);
    }
    list->len -= len;
    free(marked);
}
