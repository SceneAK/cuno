#ifndef UTILS_H
#define UTILS_H
#include <stddef.h>

#define max(a, b) ( a > b ? a : b )
#define min(a, b) ( a < b ? a : b )

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#define STATIC_ASSERT(cond, msg) typedef char static_assert_##msg[ (cond) ? 1 : -1]

#define VLS_UNION(type, size) union { type v; char bytes[size]; }

#endif
