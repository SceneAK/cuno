#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include "engine/system/network_packer.h"
#include "engine/system/network.h"
#include "engine/system/log.h"
#include "engine/alias.h"

struct network_connection {
    int sockfd;
};
struct network_listener {
    int sockfd;
};

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

u16 network_u16_to_host(u16 net) { return ntohs(net); }

u32 network_u32_to_host(u32 net) { return ntohl(net); }

u16 network_u16_to_net(u16 host) { return htons(host); }

u32 network_u32_to_net(u32 host) { return htonl(host); }

void network_buffer_init(struct network_buffer *buff, size_t capacity)
{
    buff->start = malloc(capacity);
    buff->head  = buff->start;
    buff->tail  = buff->start;
    buff->end   = buff->start + capacity;
}

void network_buffer_deinit(struct network_buffer *buff)
{
    free(buff->start);
}

void network_buffer_compact(struct network_buffer *buff)
{
    const size_t LEN = NETWORK_BUFFER_LEN(*buff);

    memmove(buff->start, buff->head, LEN);
    buff->tail = buff->start + LEN;
    buff->head = buff->start;
}

int network_buffer_make_space(struct network_buffer *buff, size_t space)
{
    if (buff->tail + space < buff->end)
        return 0;
    if (NETWORK_BUFFER_LEN(*buff) + space > NETWORK_BUFFER_CAPACITY(*buff))
        return -1;
    
    network_buffer_compact(buff);
    return 0;
}

int network_buffer_peek_hdrmsg(const struct network_buffer *buff)
{
    struct network_header hdr;
    uint8_t *tempcursor = buff->head;
    const size_t LEN = NETWORK_BUFFER_LEN(*buff);

    if (LEN < NETHDR_SERIALIZED_SIZE) 
        return 0;

    network_header_deserialize(&hdr, &tempcursor);
    if (LEN - NETHDR_SERIALIZED_SIZE < hdr.len)
        return 0;
    return 1;
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

enum network_result network_connection_send(struct network_connection *conn, uint8_t **readcursor, const uint8_t *end)
{
    ssize_t res;

    res = send(conn->sockfd, *readcursor, end - *readcursor, 0);
    if (res < 0)
        return netres_from_errno();

    *readcursor += res;
    if (*readcursor != end)
        return NETRES_PARTIAL;
    return NETRES_SUCCESS;
}

enum network_result network_connection_recv(struct network_connection *conn, uint8_t **writecursor, const uint8_t *end)
{
    ssize_t res;

    res = recv(conn->sockfd, *writecursor, end - *writecursor, 0);
    if (res < 0)
        return netres_from_errno();
    if (res == 0)
        return NETRES_ERR_CONN;

    *writecursor += res;
    return NETRES_SUCCESS;
}

void network_connection_sendrecv_nb(struct network_connection *conn, struct network_buffer *sendbuff, struct network_buffer *recvbuff)
{
    char status;

    if (!conn)
        return;

    status = network_connection_poll(conn, NETWORK_POLLIN | NETWORK_POLLOUT);

    if ((status & NETWORK_POLLOUT) && NETWORK_BUFFER_LEN(*sendbuff))
        network_connection_send(conn, &sendbuff->head, sendbuff->tail);

    if (status & NETWORK_POLLIN) {
        while (1) {
            network_connection_recv(conn, &recvbuff->tail, recvbuff->end);
            if (recvbuff->tail != recvbuff->end)
                break;
            network_buffer_compact(recvbuff);
        }
    }
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
