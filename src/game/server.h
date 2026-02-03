#ifndef SERVER_H
#define SERVER_H
#include "logic.h"
#include "engine/system/network.h"

#define NETMSG_VER 0
#define DEFAULT_RECV_SIZE 400

enum message_type {
    MSG_UNKNOWN = -1,
    MSG_GM_START,
    MSG_GM_STATE,
    MSG_GM_ACT,
};

struct player_connection {
    int player_id;
    struct network_connection *conn;
    struct network_buffer      sendbuff;
    void (*recvmsg)(short type, const void *data);
};

void server_init(int port, int max_players);
void server_start_game();
void server_update();

int server_register_local(void (*recvmsg)(short type, const void *data));
int server_handle_act(struct act act);

int server_is_idling(); /* Hacky + spaghetti */

#endif
