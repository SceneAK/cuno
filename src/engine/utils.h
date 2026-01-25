#ifndef UTILS_H
#define UTILS_H
#include <stddef.h>
#include <limits.h>
#include <stdint.h>

#define max(a, b) ( a > b ? a : b )
#define min(a, b) ( a < b ? a : b )

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#define STATIC_ASSERT(cond, msg) typedef char static_assert_##msg[ (cond) ? 1 : -1]

#define VLS_UNION(type, size) union { type v; char bytes[size]; }

#define BITS_IDX(num_of_bits, type) (num_of_bits) / (sizeof(type)*8)
#define BITS_GET(arr, n) ( (arr)[BITS_IDX(n, (arr)[0])] & ( 1<<(n % (sizeof((arr)[0])*8)) ) )
#define BITS_SET(arr, n) ( (arr)[BITS_IDX(n, (arr)[0])] |= ( 1<<(n % (sizeof((arr)[0])*8)) ) )
#define BITS_UNSET(arr, n) ( (arr)[BITS_IDX(n, (arr)[0])] &= ~( 1<<(n % (sizeof((arr)[0])*8)) ) )

#endif
