#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include "engine/system/network.h"
#include "engine/system/log.h"
#include "engine/alias.h"

#define IS_LITTLE_ENDIAN ( *((u8 *)&ENDIAN_TEST) == 1 )
#define bswap64(x) ((u64)htonl( (x) & 0xFFFFFFFF) << 32) | htonl( (x) >> 32);

struct network_connection {
    int sockfd;
};
struct network_listener {
    int sockfd;
};

static const u16 ENDIAN_TEST = 1;

enum network_result netres_from_errno() 
{
    switch(errno) {
        case EAGAIN:
            return NETRES_ERR_AGAIN;
        case ETIMEDOUT:
            return NETRES_ERR_TIMEDOUT;
        case ECONNREFUSED:
            return NETRES_ERR_REFUSED;
        case ECONNRESET:
            return NETRES_ERR_CONN;
        default:
            return NETRES_ERR;
    }
}

size_t network_serialize_u16(u8 dst[], u16 src)
{
    src = htons(src);
    memcpy(dst, &src, sizeof(u16));
    return sizeof(u16);
}

size_t network_serialize_u32(u8 dst[], u32 src)
{
    src = htonl(src);
    memcpy(dst, &src, sizeof(u32));
    return sizeof(u32);
}

size_t network_serialize_u64(u8 dst[], u64 src)
{
    if (IS_LITTLE_ENDIAN)
        src = bswap64(src);
    memcpy(dst, &src, sizeof(u64));
    return sizeof(u64);
}

size_t network_deserialize_u16(u16 *dst, const u8 src[])
{
    memcpy(dst, src, sizeof(u16));
    *dst = ntohs(*dst);
    return sizeof(u16);
}

size_t network_deserialize_u32(u32 *dst, const u8 src[])
{
    memcpy(dst, src, sizeof(u32));
    *dst = ntohl(*dst);
    return sizeof(u32);
}

size_t network_deserialize_u64(u64 *dst, const u8 src[])
{
    memcpy(dst, src, sizeof(u64));
    if (IS_LITTLE_ENDIAN)
        *dst = bswap64(*dst);
    return sizeof(u64);
}

struct network_connection *network_connection_create(const char* ipv4addr, short port)
{
    struct network_connection *conn;
    int                 sockfd;
    struct sockaddr_in  sockaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
        return NULL;

    sockaddr.sin_family      = AF_INET;
    sockaddr.sin_port        = htons(port);
    sockaddr.sin_addr.s_addr = inet_addr(ipv4addr);
    if (connect(sockfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) == -1)
        goto err;

    conn = malloc(sizeof(struct network_connection));
    if (!conn)
        goto err;

    memset(conn, 0, sizeof(struct network_connection));
    conn->sockfd = sockfd;
    cuno_logf(LOG_INFO, "Connected\n");
    return conn;
err:
    cuno_logf(LOG_ERR, "Connection Error: %s\n", strerror(errno));
    close(sockfd);
    return NULL;
}

void network_connection_destroy(struct network_connection *conn)
{
    close(conn->sockfd);
    free(conn);
}

char network_connection_poll(struct network_connection *conn, char event)
{
    struct pollfd pollfd = { 0 };

    pollfd.fd = conn->sockfd;
    pollfd.events |= event & NETWORK_POLLIN ? POLLIN : 0;
    pollfd.events |= event & NETWORK_POLLOUT ? POLLOUT : 0;

    poll(&pollfd, 1, 0);

    return (pollfd.revents & POLLIN)  ? NETWORK_POLLIN  : 0 | 
           (pollfd.revents & POLLOUT) ? NETWORK_POLLOUT : 0;
}

enum network_result network_connection_send(struct network_connection *conn, struct network_sendbuff *sendbuff)
{
    const size_t                TOTAL       = sizeof(sendbuff->message->header) + sendbuff->message->header.len;
    const struct network_header HEADER_COPY = sendbuff->message->header;

    network_header_serialize((u8 *)&sendbuff->message->header, &HEADER_COPY);
    ssize_t res = send(conn->sockfd,
            (char *)sendbuff->message + sendbuff->head,
            TOTAL - sendbuff->head, 0);
    sendbuff->message->header = HEADER_COPY;

    if (res < 0)
        return netres_from_errno();

    sendbuff->head += res;
    if (sendbuff->head != TOTAL)
        return NETRES_PARTIAL;

    return NETRES_SUCCESS;
}

enum network_result network_connection_recv(struct network_connection *conn, struct network_recvbuff *recvbuff)
{
    size_t total = sizeof(struct network_header);
    struct network_header header_temp;
    ssize_t res;

    if (recvbuff->head >= sizeof(struct network_header))
        total += recvbuff->message->header.len;

    res = recv(conn->sockfd, 
        (char *)recvbuff->message + recvbuff->head,
        total - recvbuff->head, 0);

    if (res < 0)
        return netres_from_errno();
    if (res == 0)
        return NETRES_ERR_CONN;

    recvbuff->head += res;

    if (recvbuff->head == sizeof(struct network_header)) {
        network_header_deserialize(&header_temp, (u8 *)&recvbuff->message->header);
        recvbuff->message->header = header_temp;
        return NETRES_PARTIAL;
    }

    if (recvbuff->head != total)
        return NETRES_PARTIAL;

    return NETRES_SUCCESS;
}

struct network_listener *network_listener_create(short port, int max_pending)
{
    int                     sockfd;
    struct sockaddr_in      sockaddr;
    struct network_listener *listener;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
        return NULL;

    sockaddr.sin_family      = AF_INET;
    sockaddr.sin_port        = htons(port);
    sockaddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0)
        goto err;

    if (listen(sockfd, max_pending) < 0)
        goto err;

    listener = malloc(sizeof(struct network_listener));
    listener->sockfd = sockfd;

    return listener;
err:
    cuno_logf(LOG_ERR, "Connection Error: %s\n", strerror(errno));
    close(sockfd);
    return NULL;
}
void network_listener_destroy(struct network_listener *listener)
{
    close(listener->sockfd);
    free(listener);
}
int network_listener_poll(struct network_listener *listener)
{
    return network_connection_poll((struct network_connection *)listener, NETWORK_POLLIN);
}
struct network_connection *network_listener_accept(struct network_listener *listener)
{
    struct network_connection *conn = malloc(sizeof(struct network_connection));
    if (!conn)
        return NULL;

    conn->sockfd = accept(listener->sockfd, NULL, NULL);
    return conn;
}
