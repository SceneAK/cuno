#ifndef ARRAY_LIST_H
#define ARRAY_LIST_H

#include <stddef.h>

struct array_list {
    void   *elems;
    size_t  len;
    size_t  allocated_len;
};

void array_list_init(struct array_list *list, size_t initial_alloc_len, size_t elem_size);
void array_list_deinit(struct array_list *list);
void *array_list_emplace(struct array_list *list, size_t len, size_t elem_size);
void array_list_append(struct array_list *list, const void *items, size_t len, size_t elem_size);
void array_list_remove_sft(struct array_list *list, const size_t *indices, size_t len, size_t elem_size);
void array_list_remove_swp(struct array_list *list, const size_t *indices, size_t len, size_t elem_size);

#define DEFINE_ARRAY_LIST_WRAPPER(static_inline, type, name) \
    struct name { \
        type   *elems; \
        size_t  len; \
        size_t  allocated_len; \
    }; \
    static_inline void name##_init(struct name *list, size_t initial_alloc_len) \
    { \
        array_list_init((struct array_list *)list, initial_alloc_len, sizeof(type)); \
    } \
    static_inline void name##_deinit(struct name *list) \
    { \
        array_list_deinit((struct array_list *)list); \
    } \
    static_inline type *name##_emplace(struct name *list, size_t len) \
    { \
        return (type *)array_list_emplace((struct array_list *)list, len, sizeof(type)); \
    } \
    static_inline void name##_append(struct name *list, const type *items, size_t len) \
    { \
        array_list_append((struct array_list *)list, items, len, sizeof(type)); \
    } \
    static_inline void name##_remove_sft(struct name *list, const size_t *indices, size_t len) \
    { \
        array_list_remove_sft((struct array_list *)list, indices, len, sizeof(type)); \
    } \
    static_inline void name##_remove_swp(struct name *list, const size_t *indices, size_t len) \
    { \
        array_list_remove_swp((struct array_list *)list, indices, len, sizeof(type)); \
    } \
    static_inline void name##_clear(struct name *list) \
    { \
        list->len = 0; \
    } \
    static_inline void name##_copy(struct name *dst, const struct name *src) \
    { \
        name##_clear(dst); \
        name##_emplace(dst, src->len); \
        memcpy(dst->elems, src->elems, sizeof(type) * src->len); \
    } \
    static_inline struct name name##_clone(const struct name *src) \
    { \
        struct name cpy; \
        name##_init(&cpy, src->allocated_len); \
        name##_copy(&cpy, src); \
        return cpy; \
    }

#endif
