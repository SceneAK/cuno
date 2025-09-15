#include <stdlib.h>
#include <string.h>
#include "utils.h"

/* BASIC POOL */
int basic_pool_init(struct basic_pool *pool, size_t num_of_elem, size_t element_size)
{
    if (!pool)
        return -1;
    pool->allocated_len = num_of_elem;
    pool->elements      = malloc(num_of_elem * element_size);
    pool->active        = calloc(num_of_elem, sizeof(char));
    if (!pool->elements || !pool->active)
        return -1;
    return 0;
}
void basic_pool_deinit(struct basic_pool *pool)
{
    free(pool->elements);
    free(pool->active);
}
void *basic_pool_request(struct basic_pool *pool, size_t element_size)
{
    int i;
    for (i = 0; i < pool->allocated_len; i++) {
        if (!pool->active[i]) {
            pool->active[i] = 1;
            return (char *)pool->elements + i * element_size;
        }
    }
    pool->elements  = realloc(pool->elements, 2 * pool->allocated_len * element_size);
    pool->active    = realloc(pool->active, 2 * pool->allocated_len * sizeof(char));
    if (!pool->elements || !pool->active)
        return NULL;

    pool->active[pool->allocated_len] = 1;
    for (i = pool->allocated_len + 1; i < 2 * pool->allocated_len; i++)
        pool->active[i] = 0;
    pool->allocated_len *= 2;

    return (void *)( (char *)pool->elements + pool->allocated_len * element_size );
}
void basic_pool_return(struct basic_pool *pool, void *item)
{
    int index = item - pool->elements;
    pool->active[index] = 0;
}

/* ARRAY LIST */
void array_list_init(struct array_list *list, size_t initial_len, size_t element_size)
{
    list->len           = initial_len;
    list->allocated_len = initial_len;
    list->elements      = malloc(element_size * initial_len);
}
void array_list_deinit(struct array_list *list)
{
    free(list->elements);
}
void *array_list_append_empty(struct array_list *list, size_t len, size_t element_size)
{
    list->len += len;
    if (list->len > list->allocated_len) {
        list->allocated_len *= 2;
        list->elements = realloc(list->elements, element_size * list->allocated_len);
    }

    return (char *)list->elements + (list->len - len) * element_size;
}
void array_list_append(struct array_list *list, const void *items, size_t len, size_t element_size)
{
    memcpy(array_list_append_empty(list, len, element_size), items, element_size * len);
}
void array_list_remove_sft(struct array_list *list, const size_t *indices, size_t len, size_t element_size)
{
    int i, advance = 0;
    char *marked = calloc(list->len, sizeof(char));
    for (i = 0; i < len; i++) {
        marked[indices[i]] = 1;
    }
    for (i = 0; (i + advance) < list->len; i++) {
        while (marked[i + advance]) 
            advance++;
        memcpy((char *)list->elements + i * element_size, (char *)list->elements + (i + advance) * element_size, element_size);
    }
    list->len -= len;
    free(marked);
}
void array_list_remove_swp(struct array_list *list, const size_t *indices, size_t len, size_t element_size)
{
    int i, swap_index = list->len;
    char *marked = calloc(list->len, sizeof(char));
    for (i = 0; i < len; i++) {
        marked[indices[i]] = 1;
    }
    for (i = 0; i < len; i++) {
        if (indices[i] >= list->len - len)
            continue;
        do swap_index--;
        while (marked[swap_index]);
        memcpy((char *)list->elements + i * element_size, (char *)list->elements + swap_index * element_size, element_size);
    }
    list->len -= len;
    free(marked);
}
