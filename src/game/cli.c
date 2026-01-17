#include <stdio.h>
#include "engine/system/network.h"
#include "engine/utils.h"
#include "serialize.h"
#include "logic.h"

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

#define PRINTF_RESET() printf("\x1b[2J\x1bH")

static char global_char_buff[2048];

/********* SERVER **********/
static struct game_state             server_state;
static struct network_connection    *server_playerconn[1];

void server_send_state()
{
    VLS_UNION(struct network_message, 512) server_msg = { 0 };
    struct network_messagebuff buff = { 0, .message = &server_msg.v };
    uint8_t *cursor;
    int i;

    cursor = server_msg.v.data;
    game_state_serialize(&cursor, &server_state);

    server_msg.v.header.len = cursor - server_msg.v.data;

    cursor = server_msg.v.data;
    game_state_deserialize(&server_state, &cursor);

    while (network_connection_send(server_playerconn[0], &buff) == NETRES_PARTIAL);
}

void server_move(const char *input)
{
    struct act_play play = { 0 };

    if (input[0] == 'p') {
        play.card_id = atoi(input + 1);
        game_state_act_play(&server_state, play);
    } else {
        game_state_act_draw(&server_state);
    }
    game_state_end_turn(&server_state);
    server_send_state();
}

void server_start(struct network_listener *listener)
{
    game_state_init(&server_state, 2, 5); 

    server_playerconn[0] = network_listener_accept(listener); /* Player 1 */
    printf("Accepted.\n");
    server_send_state();
}

void server_update()
{
    VLS_UNION(struct network_message, 256) server_msg = { 0 };
    struct network_messagebuff buff = { 0, .message = &server_msg.v };

    if (server_state.active_player_index == 0)
        return;

    while (network_connection_recv(server_playerconn[0], &buff) == NETRES_PARTIAL);
    server_move((char *)server_msg.v.data);
}

/********* CLIENT **********/
static struct game_state     client_state = {0};
static int                   client_player;
struct network_connection   *client_serverconn = NULL;

void client_start(struct network_connection *conn)
{
    client_serverconn = conn;
}

void client_update()
{
    VLS_UNION(struct network_message, 512)   msg;
    struct network_messagebuff               buff = {0, .message = &msg.v};
    unsigned char                           *cursor;
    int                                      playerno = client_serverconn ? 1 : 0;
    int i, temp;

    if (client_serverconn) {
        while (network_connection_recv(client_serverconn, &buff) == NETRES_PARTIAL);
        cursor = msg.v.data;
        game_state_deserialize(&client_state, &cursor);
    } else
        client_state = server_state;

    log_game_state(global_char_buff, sizeof(global_char_buff), &client_state);
    printf("%s\n", global_char_buff);

    if (client_state.active_player_index != playerno)
        return;

    printf("===============================================\n"
           "draw: d<enter>, play: p[card_id]<enter>\n"
           ">> ");
    scanf(" %c", msg.v.data);
    msg.v.header.len = 1;
    if (msg.v.data[0] == 'p') {
        scanf(" %d", &temp);
        msg.v.header.len += sprintf((char *)(msg.v.data+1), "%d", temp);
    }

    buff.head = 0;
    if (client_serverconn)
        while (network_connection_send(client_serverconn, &buff) == NETRES_PARTIAL);
    else
        server_move((char *)msg.v.data);
}

void main_client(const char *ipv4addr, short port)
{
    struct network_connection *conn;

    printf("Connecting to %s on :%d\n", ipv4addr, port);
    conn = network_connection_create(ipv4addr, port);
    if (!conn)
        return;

    client_start(conn);
    while (1) {
        client_update();
        PRINTF_RESET();
    }

    network_connection_destroy(conn);
}

void main_host(short port)
{
    struct network_listener *listener = network_listener_create(port, 3);

    printf("Server listening on port %d...\n", port);
    server_start(listener);

    client_start(NULL);
    while (1) {
        client_update();
        server_update();
        PRINTF_RESET();
    }

    network_listener_destroy(listener);
}

int main(int argc, char *argv[])
{
    PRINTF_RESET();
    printf("CUNO Start.\n");

    if (argc == 3)
        main_client(argv[1], atoi(argv[2]));
    else if (argc == 2)
        main_host(atoi(argv[1]));
    else
        return 1;

    return 0;
}
