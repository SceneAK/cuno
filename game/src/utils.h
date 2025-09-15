#ifndef UTILS_H
#define UTILS_H
#include <stddef.h>

struct basic_pool {
    void   *elements;
    char   *active;
    size_t  allocated_len;
};
int basic_pool_init(struct basic_pool *pool, size_t num_of_elem, size_t element_size);
void basic_pool_deinit(struct basic_pool *pool);
void *basic_pool_request(struct basic_pool *pool, size_t element_size);
void basic_pool_return(struct basic_pool *pool, void *item);

#define DEFINE_BASIC_POOL_WRAPPER(static_inline, type, name) \
    struct name { \
        type   *elements; \
        char   *active; \
        size_t  allocated_len; \
    }; \
    static_inline void name##_init(struct name *pool, size_t num_of_elem) \
    { \
        basic_pool_init((struct basic_pool *)pool, num_of_elem, sizeof(type)); \
    } \
    static_inline void name##_deinit(struct name *pool) \
    { \
        basic_pool_deinit((struct basic_pool *)pool); \
    } \
    static_inline type *name##_request(struct name *pool) \
    { \
        return (type *)basic_pool_request((struct basic_pool *)pool, sizeof(type)); \
    } \
    static_inline void name##_return(struct name *pool, type *item) \
    { \
        basic_pool_return((struct basic_pool *)pool, item); \
    } \

struct array_list { \
    void   *elements;
    size_t  len;
    size_t  allocated_len;
};
void array_list_init(struct array_list *list, size_t initial_len, size_t element_size);
void array_list_deinit(struct array_list *list);
void *array_list_append_empty(struct array_list *list, size_t len, size_t element_size);
void array_list_append(struct array_list *list, const void *items, size_t len, size_t element_size);
void array_list_remove_sft(struct array_list *list, const size_t *indices, size_t len, size_t element_size);
void array_list_remove_swp(struct array_list *list, const size_t *indices, size_t len, size_t element_size);

#define DEFINE_ARRAY_LIST_WRAPPER(static_inline, type, name) \
    struct name { \
        type   *elements; \
        size_t  len; \
        size_t  allocated_len; \
    }; \
    static_inline void name##_init(struct name *list, size_t initial_len) \
    { \
        array_list_init((struct array_list *)list, initial_len, sizeof(type)); \
    } \
    static_inline void name##_deinit(struct name *list) \
    { \
        array_list_deinit((struct array_list *)list); \
    } \
    static_inline type *name##_append_empty(struct name *list, size_t len) \
    { \
        return (type *)array_list_append_empty((struct array_list *)list, len, sizeof(type)); \
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
