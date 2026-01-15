#ifndef NETWORK_H
#define NETWORK_H

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include "engine/utils.h"

#define NETWORK_POLLIN  1<<1
#define NETWORK_POLLOUT 1<<2

STATIC_ASSERT(CHAR_BIT == 8, unsupported_byte_width);

enum network_result {
    NETRES_SUCCESS,
    NETRES_PARTIAL,

    NETRES_ERR_AGAIN,
    NETRES_ERR_TIMEDOUT,
    NETRES_ERR_REFUSED,
    NETRES_ERR_CONN,
    NETRES_ERR,
};

struct network_connection;
struct network_listener;
struct network_header {
    uint16_t version;
    uint16_t type;
    uint32_t len;
};
STATIC_ASSERT(sizeof(struct network_header) == 8, net_header_size_mismatch);
struct network_message {
    struct network_header header;
    uint8_t data[];
};
struct network_messagebuff {
    size_t head;
    size_t capacity;
    struct network_message *message;
};

static inline const char *str_network_result(enum network_result res)
{
    switch(res) {
        case NETRES_SUCCESS:
            return "Success";
        case NETRES_ERR_CONN:
            return "Connection Error";
        case NETRES_ERR_REFUSED:
            return "Connection Refused";
        case NETRES_ERR_TIMEDOUT:
            return "Connection Timed Out";
        case NETRES_ERR_AGAIN:
            return "NETRES_ERR_AGAIN";
        default:
            return "Unkonwn network result";
    }
}

static inline void network_serialize_u8(uint8_t **cursor, const uint8_t src) 
{ 
    **cursor = src;
    *cursor += sizeof(uint8_t);
}
static inline uint8_t network_deserialize_u8(uint8_t **cursor) 
{ 
    *cursor += sizeof(uint8_t);
    return *(*cursor - sizeof(uint8_t));
}

static inline void network_serialize_str(uint8_t **cursor, const char* src) 
{ 
     *cursor += sprintf((char *)*cursor, "%s", src);
}
static inline void network_deserialize_str(const char* dst, uint8_t **cursor) 
{ 
     *cursor += sprintf((char *)dst, "%s", *cursor);
}

void network_serialize_u16(uint8_t **cursor, uint16_t src);
void network_serialize_u32(uint8_t **cursor, uint32_t src);
void network_serialize_u64(uint8_t **cursor, uint64_t src);
uint16_t network_deserialize_u16(uint8_t **cursor);
uint32_t network_deserialize_u32(uint8_t **cursor);
uint64_t network_deserialize_u64(uint8_t **cursor);

static inline void network_header_serialize(uint8_t **cursor, const struct network_header *header)
{
    network_serialize_u16(cursor, header->version);
    network_serialize_u16(cursor, header->type);
    network_serialize_u32(cursor, header->len);
}

static inline void network_header_deserialize(struct network_header *header, uint8_t **cursor)
{
    header->version = network_deserialize_u16(cursor);
    header->type = network_deserialize_u16(cursor);
    header->len = network_deserialize_u32(cursor);
}

struct network_connection *network_connection_create(const char* ipv4addr, short port);
void network_connection_destroy(struct network_connection *conn);
char network_connection_poll(struct network_connection *conn, char flags);
enum network_result network_connection_send(struct network_connection *conn, struct network_messagebuff *buff);
enum network_result network_connection_recv(struct network_connection *conn, struct network_messagebuff *buff);

struct network_listener *network_listener_create(short port, int max_pending);
void network_listener_destroy(struct network_listener *listener);
int network_listener_poll(struct network_listener *conn);
struct network_connection *network_listener_accept(struct network_listener *listener);

#endif
