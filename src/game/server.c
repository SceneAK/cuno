#include "engine/system/network.h"
#include "engine/system/time.h"
#include "serialize.h"
#include "server.h"
#include "logic.h"

struct network_listener      *server_listener;
struct game_state             server_state = {0};
struct player_connection      server_conns[PLAYER_MAX] = {0};
int                           server_conn_len;
char                          server_game_started = 0;
struct network_buffer         server_recvbuff;

int                           server_max_player;

int server_register_local(void (*recvmsg)(short type, const void *data))
{
    if (server_game_started)
        return -1;
    server_conns[server_conn_len].conn = NULL;
    server_conns[server_conn_len].recvmsg = recvmsg;
    server_conn_len++;
    return 0;
}

void server_broadcast_game_start()
{
    struct network_header header = {
        .version = NETMSG_VER,
        .type = MSG_GM_START
    };
    int i;

    for (i = 0; i < server_conn_len; i++) {
        if (!server_conns[i].conn) {
            server_conns[i].recvmsg(header.type, &server_conns[i].player_id);
            continue;
        }
        
        header.len = sizeof(uint8_t);

        network_buffer_make_space(&server_conns[i].sendbuff, header.len + NETHDR_SERIALIZED_SIZE);

        network_header_serialize(&server_conns[i].sendbuff.tail, &header);
        network_pack_u8(&server_conns[i].sendbuff.tail, server_conns[i].player_id);
    }
}

void server_broadcast_state()
{
    struct game_state temp;
    struct network_header header = {
        .version = NETMSG_VER,
        .type = MSG_GM_STATE,
        .len = 0,
    };
    int i;


    game_state_init(&temp);
    for (i = 0; i < server_conn_len; i++) {
        game_state_copy_into(&temp, &server_state);
        game_state_for_player(&temp, server_conns[i].player_id);

        if (!server_conns[i].conn) {
            server_conns[i].recvmsg(header.type, &temp);
            continue;
        }

        header.len = game_state_serialize(NULL, &temp);
        network_buffer_make_space(&server_conns[i].sendbuff, NETHDR_SERIALIZED_SIZE + header.len);

        network_header_serialize(&server_conns[i].sendbuff.tail, &header);
        game_state_serialize(&server_conns[i].sendbuff.tail, &temp);
    }
}

int server_handle_act(struct act act)
{
    int res = game_state_act(&server_state, act);
    if (res == 0)
        server_broadcast_state();
    return res;
}

void server_process_recvbuff()
{
    static int printed = 0;
    struct network_header header;
    struct act act;

    if (!network_buffer_peek_hdrmsg(&server_recvbuff))
        return;

    network_header_deserialize(&header, &server_recvbuff.head);
    switch (header.type) {
        case MSG_GM_ACT:
            act = act_deserialize(&server_recvbuff.head);
            server_handle_act(act);
            return;
        default:
            if (printed)
                break;
            printed = 1;
            printf("Header received: len %d type %d\n", header.len, header.type);
            return;
    }
}

void server_init(int port, int max_players)
{
    network_buffer_init(&server_recvbuff, 64 * max_players);
    game_state_init(&server_state);
    server_listener = network_listener_create(port, max_players);
    server_max_player = max_players;
}

void server_start_game()
{
    const int INITIAL_DEAL = 5;
    int i;

    srand(get_monotonic_time());
    game_state_start(&server_state, server_conn_len, INITIAL_DEAL);
    for (i = 0; i < server_conn_len; i++)
        server_conns[i].player_id = server_state.players[i].id;

    server_game_started = 1;
    server_broadcast_game_start();
    server_broadcast_state();
}

void server_update()
{
    int i;
    uint8_t *temp;

    if (!server_game_started && network_listener_poll(server_listener)) {
        server_conns[server_conn_len].conn = network_listener_accept(server_listener);
        network_buffer_init(&server_conns[server_conn_len].sendbuff, 1024);
        server_conn_len++;

        if (server_conn_len >= server_max_player)
            server_start_game();
    }

    for (i = 0; i < server_conn_len; i++) {
        if (!server_conns[i].conn)
            continue;

        network_connection_sendrecv_nb(server_conns[i].conn, &server_conns[i].sendbuff, &server_recvbuff);

        while (NETWORK_BUFFER_LEN(server_recvbuff))
            server_process_recvbuff();
    }
}

int server_is_idling() /* Hacky + spaghetti */
{
    int i;
    for (i = 0; i < server_conn_len; i++) {
        if (NETWORK_BUFFER_LEN(server_conns[i].sendbuff))
            return 0;
    }
    return 1;
}
