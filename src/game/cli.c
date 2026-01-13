#include <stdio.h>
#include "logic.h"
#include "engine/system/network.h"
#include "engine/utils.h"

#ifndef __STDC_IEC_559__
#include <float.h>
STATIC_ASSERT(
    FLT_RADIX == 2
    && FLT_MANT_DIG == 24
    && FLT_MAX_EXP == 128
    && sizeof(float)*CHAR_BIT == 32,

    float_not_ieee754
);
#endif

/* TODO: Factor out game_state and logic.h stuff from gui.c */
static struct game_state *game_state;

void sendstuff(struct network_connection *conn)
{
    int                         len;
    enum network_result         res;
    union {
        unsigned char           buffer[sizeof(struct network_header) + 1024];
        struct network_message  m;
    }                           message;
    struct network_sendbuff     sendbuff = { .head = 0, .message = &message.m };

    printf("Type something: ");
    scanf("%s", message.m.data);

    len = strlen((char *)message.m.data);
    printf("message length: %d\n", len);

    message.m.header.version = 0;
    message.m.header.type = 0;
    message.m.header.len = len+1;
    printf("sending");
    res = network_connection_send(conn, &sendbuff);
    printf("result: %s\n", str_network_result(res));
    if (res != NETRES_SUCCESS)
        exit(EXIT_FAILURE);
}
void recvstuff(struct network_connection *conn)
{
    enum network_result         res;
    union {
        unsigned char           buffer[sizeof(struct network_header) + 1024];
        struct network_message  m;
    }                           message;
    struct network_recvbuff     recvbuff = { .head = 0, .capacity = sizeof(message), .message = &message.m };

    do
        res = network_connection_recv(conn, &recvbuff);
    while (res == NETRES_PARTIAL);
    printf("hdr: len %d\n", message.m.header.len);
    printf("msg: %s\n", (char *)message.m.data);
    printf("res: %s\n", str_network_result(res));
    if (res != NETRES_SUCCESS)
        exit(EXIT_FAILURE);
    printf("press enter to continue..."); getchar();
}
void client_repl(struct network_connection *conn)
{
    int temp;
    while (1) {
        printf("=> Client REPL\n");
        printf("[1] to send, [2] to recv: ");
        scanf("%d", &temp); getchar();
        if (temp == 1)
            sendstuff(conn);
        else
            recvstuff(conn);
    }
}
void main_client(const char *ipv4addr, short port)
{
    struct network_connection *conn;

    printf("Client Test: connecting to %s on :%d\n", ipv4addr, port);
    conn = network_connection_create(ipv4addr, port);
    if (!conn)
        return;

    client_repl(conn);

    network_connection_destroy(conn);
}

void main_server(short port)
{
    struct network_listener *listener = network_listener_create(port, 3);
    struct network_connection *conn;

    printf("Server listening on port %d...\n", port);
    while (1) {
        if (network_listener_poll(listener) & NETWORK_POLLIN) {
            printf("Incoming request...\n");
            break;
        }
    }
    conn = network_listener_accept(listener);
    client_repl(conn);
    
    network_listener_destroy(listener);
}

int main(int argc, char *argv[])
{
    printf("CUNO\n");

    if (argc == 3)
        main_client(argv[1], atoi(argv[2]));
    else if (argc == 2)
        main_server(atoi(argv[1]));
    else
        return 1;

    return 0;
}
