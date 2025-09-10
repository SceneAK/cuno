#ifndef UTILS_H
#define UTILS_H
#include <stdlib.h>
#include <string.h>

#define DECLARE_BASIC_POOL(type, name) \
    struct name; \
    struct name *name##_create(size_t num_of_elem); \
    void name##_destroy(struct name *pool); \
    type *name##_request(struct name *pool); \
    void name##_return(struct name *pool, type *item);
#define DEFINE_BASIC_POOL(type, name) \
    struct name { \
        type   *elements; \
        char   *active; \
        size_t  allocated_len; \
    };
#define DEFINE_BASIC_POOL_FUNCS(type, name) \
    struct name *name##_create(size_t num_of_elem) \
    { \
        struct name *pool = malloc(sizeof(struct name)); \
        if (!pool)  \
            return NULL; \
        pool->allocated_len = num_of_elem; \
        pool->elements      = malloc(num_of_elem * sizeof(type)); \
        pool->active        = malloc(num_of_elem * sizeof(char)); \
        if (!pool->elements || !pool->active) \
            return NULL; \
        return pool; \
    } \
    void name##_destroy(struct name *pool) \
    { \
        free(pool->elements); \
        free(pool->active); \
        free(pool); \
    } \
    type *name##_request(struct name *pool) \
    { \
        int i; \
        for (i = 0; i < pool->allocated_len; i++) { \
            if (!pool->active[i]) { \
                pool->active[i] = 1; \
                return (pool->elements + i); \
            } \
        } \
        pool->elements  = realloc(pool->elements, 2 * pool->allocated_len * sizeof(type)); \
        pool->active    = realloc(pool->active, 2 * pool->allocated_len * sizeof(char)); \
        if (!pool->elements || !pool->active) \
            return NULL; \
        pool->active[pool->allocated_len] = 1; \
        for (i = pool->allocated_len + 1; i < 2 * pool->allocated_len; i++) \
            pool->active[i] = 0; \
        pool->allocated_len *= 2; \
        return (type *)(pool->elements + pool->allocated_len); \
    } \
    void name##_return(struct name *pool, type *item) \
    { \
        int index = item - pool->elements; \
        pool->active[index] = 0; \
    } \

#define DECLARE_ARRAY_LIST(type, name) \
    struct name; \
    void name##_init(struct name *list, size_t initial_len); \
    void name##_deinit(struct name *list); \
    void name##_append_empty(struct name *list, size_t len); \
    void name##_append(struct name *list, const type *items, size_t len); \
    void name##_remove_sft(struct name *list, const size_t *indices, size_t len); \
    void name##_remove_swp(struct name *list, const size_t *indices, size_t len);
#define DEFINE_ARRAY_LIST(type, name) \
    struct name { \
        type   *elements; \
        size_t  len; \
        size_t  allocated_len; \
    };
#define DEFINE_ARRAY_LIST_FUNCS(type, name) \
    void name##_init(struct name *list, size_t initial_len) \
    { \
        list->len           = initial_len; \
        list->allocated_len = initial_len; \
        list->elements      = malloc(sizeof(type) * initial_len); \
    } \
    void name##_deinit(struct name *list) \
    { \
        free(list->elements); \
    } \
    void name##_append_empty(struct name *list, size_t len) \
    { \
        list->len += len; \
        if (list->len > list->allocated_len) { \
            list->allocated_len *= 2; \
            list->elements = realloc(list->elements, sizeof(type) * list->allocated_len); \
        } \
    } \
    void name##_append(struct name *list, const type *items, size_t len) \
    { \
        name##_append_empty(list, len); \
        memcpy(list->elements + list->len - len, items, sizeof(type) * len); \
    } \
    void name##_remove_sft(struct name *list, const size_t *indices, size_t len) \
    { \
        int i, advance = 0; \
        char *marked = calloc(list->len, sizeof(char)); \
        for (i = 0; i < len; i++) { \
            marked[indices[i]] = 1; \
        } \
        for (i = 0; (i + advance) < list->len; i++) { \
            while (marked[i + advance])  \
                advance++; \
            list->elements[i] = list->elements[i + advance]; \
        } \
        list->len -= len; \
        free(marked); \
    } \
    void name##_remove_swp(struct name *list, const size_t *indices, size_t len) \
    { \
        int i, swap_index = list->len; \
        char *marked = calloc(list->len, sizeof(char)); \
        for (i = 0; i < len; i++) { \
            marked[indices[i]] = 1; \
        } \
        for (i = 0; i < len; i++) { \
            if (indices[i] >= list->len - len)  \
                continue; \
            do swap_index--; \
            while (marked[swap_index]); \
            list->elements[indices[i]] = list->elements[swap_index]; \
        } \
        list->len -= len; \
        free(marked); \
    } \

#endif

/* DEFINE_BASIC_POOL(int, test_int_list); */
/* DEFINE_ARRAY_LIST(int, test_int_list); */

/*  Keeping this in version control, just in case.
 *
 *  Function assumes all valid parameters, no duplicate indices, unsorted.
 *  It's swap-delete, but the concept is 
 *  if delete_index > current swap_index, then delete_index has been swapped, 
 *  and we need to find *what index* it was swapped with. 
 *
 *  We can find it by how far delete_index is from the last hand index (hand_len - 1).
 *  Once it's found, that index could also be > swap_index,
 *  meaning it's swapped AGAIN. So repeat until < swap_index.

void remove_cards(struct player *player, const int *indices, int len)
{
    int i, swap_index, delete_index;

    swap_index = player->hand_len;
    for (i = 0; i < len; i++) {
        swap_index--;
        delete_index = indices[i];
        while (delete_index > swap_index) 
            delete_index = indices[ player->hand_len - 1 - delete_index ];

        player->hand[delete_index] = player->hand[swap_index];
    }
    player->hand_len -= len;
}
*/
