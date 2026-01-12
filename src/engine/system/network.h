#ifndef NETWORK_H
#define NETWORK_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include "engine/utils.h"

#define NETWORK_SERIALIZE_AUTO(dst, src) network_serialize_auto(dst, src, sizeof(*src))
#define NETWORK_DESERIALIZE_AUTO(dst, src) network_deserialize_auto(dst, src, sizeof(*dst))

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
struct network_sendbuff {
    size_t head;
    struct network_message *message;
};
struct network_recvbuff {
    size_t capacity;
    size_t head;
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


size_t network_serialize_u16(uint8_t dst[], uint16_t src);
size_t network_serialize_u32(uint8_t dst[], uint32_t src);
size_t network_serialize_u64(uint8_t dst[], uint64_t src);
size_t network_deserialize_u16(uint16_t *dst, const uint8_t src[]);
size_t network_deserialize_u32(uint32_t *dst, const uint8_t src[]);
size_t network_deserialize_u64(uint64_t *dst, const uint8_t src[]);

static inline size_t network_serialize_auto(uint8_t dst[], const void *src, uint64_t bytes)
{
    union {
        uint16_t u16;
        uint32_t u32;
        uint64_t u64;
    } tmp = { 0 };

    switch(bytes*CHAR_BIT) {
        case 16:
            memcpy(&tmp.u16, src, bytes);
            return network_serialize_u16(dst, tmp.u16);
        case 32:
            memcpy(&tmp.u32, src, bytes);
            return network_serialize_u32(dst, tmp.u32);
        case 64:
            memcpy(&tmp.u64, src, bytes);
            return network_serialize_u64(dst, tmp.u64);
        default:
            return 0;
    }
}
static inline uint64_t network_deserialize_auto(void *dst, uint8_t src[], uint64_t bytes)
{
    switch(bytes*CHAR_BIT) {
        case 16:
            return network_deserialize_u16(dst, src);
        case 32:
            return network_deserialize_u32(dst, src);
        case 64:
            return network_deserialize_u64(dst, src);
        default:
            return 0;
    }
}

struct network_connection *network_connection_create(const char* ipv4addr, short port, double timeout_ms);
void network_connection_destroy(struct network_connection *conn);
char network_connection_poll(struct network_connection *conn, char flags);
enum network_result network_connection_send(struct network_connection *conn, struct network_sendbuff *buff);
enum network_result network_connection_recv(struct network_connection *conn, struct network_recvbuff *buff);

struct network_listener *network_listener_create(short port);
void network_listener_destroy(struct network_listener *listener);
int network_listener_poll(struct network_listener *conn);
struct network_connection *network_listener_accept(struct network_listener *listener);

#endif
