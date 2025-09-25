#ifndef UTILS_H
#define UTILS_H
#include <stddef.h>

struct array_list { \
    void   *elements;
    size_t  len;
    size_t  allocated_len;
};
void array_list_init(struct array_list *list, size_t initial_alloc_len, size_t elem_size);
void array_list_deinit(struct array_list *list);
void *array_list_append_new(struct array_list *list, size_t len, size_t elem_size);
void array_list_append(struct array_list *list, const void *items, size_t len, size_t elem_size);
void array_list_remove_sft(struct array_list *list, const size_t *indices, size_t len, size_t elem_size);
void array_list_remove_swp(struct array_list *list, const size_t *indices, size_t len, size_t elem_size);

#define DEFINE_ARRAY_LIST_WRAPPER(static_inline, type, name) \
    struct name { \
        type   *elements; \
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
    static_inline type *name##_append_new(struct name *list, size_t len) \
    { \
        return (type *)array_list_append_new((struct array_list *)list, len, sizeof(type)); \
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

#endif
