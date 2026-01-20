#ifndef NETWORK_H
#define NETWORK_H
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include "engine/utils.h"
#include "engine/system/network_packer.h"

#define NETWORK_POLLIN  1<<1
#define NETWORK_POLLOUT 1<<2

#define NETWORK_BUFFER_LEN(buff) ((buff).tail - (buff).head)
#define NETWORK_BUFFER_SPACE(buff) ((buff).end - (buff).tail)
#define NETWORK_BUFFER_CAPACITY(buff) ((buff).end - (buff).start)

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
struct network_buffer {
    uint8_t *start,
            *head,
            *tail,
            *end;
};
STATIC_ASSERT(CHAR_BIT == 8, unsupported_byte_width);
STATIC_ASSERT(sizeof(struct network_header) == 8, net_header_size_mismatch);

static const size_t NETHDR_SERIALIZED_SIZE = sizeof(struct network_header); /* might be a bad assumption */

static inline const char *str_network_result(enum network_result res);

static inline void network_header_serialize(uint8_t **cursor, const struct network_header *header);
static inline void network_header_deserialize(struct network_header *header, uint8_t **cursor);

struct network_connection *network_connection_create(const char* ipv4addr, short port);
void network_connection_destroy(struct network_connection *conn);
char network_connection_poll(struct network_connection *conn, char flags);
enum network_result network_connection_send(struct network_connection *conn, uint8_t **readcursor, const uint8_t *end);
enum network_result network_connection_recv(struct network_connection *conn, uint8_t **writecursor, const uint8_t *end);

struct network_listener *network_listener_create(short port, int max_pending);
void network_listener_destroy(struct network_listener *listener);
int network_listener_poll(struct network_listener *conn);
struct network_connection *network_listener_accept(struct network_listener *listener);

void network_buffer_init(struct network_buffer *buff, size_t capacity);
void network_buffer_deinit(struct network_buffer *buff);
int network_buffer_make_space(struct network_buffer *buff, size_t space);


static inline const char *str_network_result(enum network_result res)
{
    switch(res) {
        case NETRES_SUCCESS: return "Success";
        case NETRES_ERR_CONN: return "Connection Error";
        case NETRES_ERR_REFUSED: return "Connection Refused";
        case NETRES_ERR_TIMEDOUT: return "Connection Timed Out";
        case NETRES_ERR_AGAIN: return "NETRES_ERR_AGAIN";
        default: return "Unkonwn network result";
    }
}

static inline void network_header_serialize(uint8_t **cursor, const struct network_header *header)
{
    network_pack_u16(cursor, header->version, NULL);
    network_pack_u16(cursor, header->type,    NULL);
    network_pack_u32(cursor, header->len,     NULL);
}

static inline void network_header_deserialize(struct network_header *header, uint8_t **cursor)
{
    header->version = network_unpack_u16(cursor);
    header->type    = network_unpack_u16(cursor);
    header->len     = network_unpack_u32(cursor);
}

#endif
