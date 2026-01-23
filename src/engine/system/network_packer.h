#ifndef NETPACKER_H
#define NETPACKER_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define DEFINE_DRYRUN_RETURN(cursor, dryrun_res, len) \

uint16_t network_u16_to_net(uint16_t host);
uint32_t network_u32_to_net(uint32_t host);
uint16_t network_u16_to_host(uint16_t net);
uint32_t network_u32_to_host(uint32_t net);

static inline size_t network_pack_str(uint8_t **cursor, const char* src);
static inline size_t network_pack_u8(uint8_t **cursor, const uint8_t src);
static inline size_t network_pack_u16(uint8_t **cursor, uint16_t src);
static inline size_t network_pack_u32(uint8_t **cursor, uint32_t src);
static inline size_t network_pack_u64(uint8_t **cursor, uint64_t src);

static inline void network_unpack_str(const char* dst, uint8_t **cursor);
static inline uint8_t network_unpack_u8(uint8_t **cursor);
static inline uint16_t network_unpack_u16(uint8_t **cursor);
static inline uint32_t network_unpack_u32(uint8_t **cursor);
static inline uint64_t network_unpack_u64(uint8_t **cursor);

static inline size_t network_pack_str(uint8_t **cursor, const char* src) 
{ 
    if ((cursor) == NULL)
        return strlen(src);

    *cursor += sprintf((char *)*cursor, "%s", src);
    return strlen(src);
}

static inline size_t network_pack_u8(uint8_t **cursor, const uint8_t src) 
{ 
    if ((cursor) == NULL)
        return sizeof(src);

    **cursor = src;
    *cursor += sizeof(src);

    return sizeof(src);
}

static inline size_t network_pack_u16(uint8_t **cursor, uint16_t src)
{
    if ((cursor) == NULL)
        return sizeof(src);

    src = network_u16_to_net(src);
    memcpy(*cursor, &src, sizeof(uint16_t));
    *cursor += sizeof(uint16_t);

    return sizeof(src);
}

static inline size_t network_pack_u32(uint8_t **cursor, uint32_t src)
{
    if ((cursor) == NULL)
        return sizeof(src);

    src = network_u32_to_net(src);
    memcpy(*cursor, &src, sizeof(uint32_t));
    *cursor += sizeof(uint32_t);

    return sizeof(src);
}

static inline void network_unpack_str(const char* dst, uint8_t **cursor) 
{ 
     *cursor += sprintf((char *)dst, "%s", *cursor);
}

static inline uint8_t network_unpack_u8(uint8_t **cursor) 
{ 
    uint8_t temp = **cursor;
    *cursor += sizeof(temp);
    return temp;
}

static inline uint16_t network_unpack_u16(uint8_t **cursor)
{
    uint16_t temp;
    memcpy(&temp, *cursor, sizeof(uint16_t));
    *cursor += sizeof(uint16_t);
    return network_u16_to_host(temp);
}

static inline uint32_t network_unpack_u32(uint8_t **cursor)
{
    uint32_t temp;
    memcpy(&temp, *cursor, sizeof(uint32_t));
    *cursor += sizeof(uint32_t);
    return network_u32_to_host(temp);
}

#endif
