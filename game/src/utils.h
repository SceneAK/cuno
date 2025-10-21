#ifndef UTILS_H
#define UTILS_H
#include <stddef.h>
#include <string.h>

#define max(a, b) ( a > b ? a : b )
#define min(a, b) ( a < b ? a : b )

struct array_list {
    void   *elements;
    size_t  len;
    size_t  allocated_len;
};
void array_list_init(struct array_list *list, size_t initial_alloc_len, size_t elem_size);
void array_list_deinit(struct array_list *list);
void *array_list_emplace(struct array_list *list, size_t len, size_t elem_size);
void array_list_append(struct array_list *list, const void *items, size_t len, size_t elem_size);
void array_list_remove_sft(struct array_list *list, const size_t *indices, size_t len, size_t elem_size);
void array_list_remove_swp(struct array_list *list, const size_t *indices, size_t len, size_t elem_size);
int array_list_index_of_field(const struct array_list *list, size_t elem_size, size_t field_offset, const void *val_ptr, size_t field_size);


#define array_list_index_of(field, list_ptr, val) \
    ((list_ptr)->len < 1 ? -1 : \
        array_list_index_of_field((const struct array_list *)(list_ptr), sizeof(*(list_ptr)->elements), (char *)(&(list_ptr)->elements->field) - (char *)((list_ptr)->elements), &(val), sizeof(val)))

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
        memcpy(dst->elements, src->elements, sizeof(type) * src->len); \
    } \
    static_inline struct name name##_clone(const struct name *src) \
    { \
        struct name cpy; \
        name##_init(&cpy, src->allocated_len); \
        name##_copy(&cpy, src); \
        return cpy; \
    }

#endif
