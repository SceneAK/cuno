#ifndef SERVER_H
#define SERVER_H
#include "engine/system/network.h"

#define NETMSG_VER 0
#define DEFAULT_RECV_SIZE 400

enum message_type {
    MSG_UNKNOWN = -1,
    MSG_GMSTATE,
    MSG_GMSTART,
    MSG_ACT,
};

struct player_connection {
    int player_id;
    struct network_connection *conn;
    struct network_buffer      sendbuff;
    void (*recvmsg)(short type, const void *data);
};

void server_start(struct network_listener *listener);
void server_start_game();
void server_update();

int server_register_local(void (*recvmsg)(short type, const void *data));
void server_process_act(const char *input);

int server_is_idling(); /* Hacky + spaghetti */

#endif
