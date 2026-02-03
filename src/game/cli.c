#include <stdio.h>
#include <unistd.h>
#include "engine/system/network.h"
#include "engine/utils.h"
#include "serialize.h"
#include "server.h"
#include "logic.h"

#define PRINTF_RESET() printf("\x1b[2J\x1b[H")

static char  global_char_buff[2048];
static const char  *SPINNER[] = {"⌜", "⌝", "⌟", "⌞"};

void print_spinner()
{
    static int spinner_idx = 0;
    printf("%s\x1b[1D", SPINNER[spinner_idx]);
    fflush(stdout);
    spinner_idx = (spinner_idx + 1) % ARRAY_SIZE(SPINNER);
}

static struct game_state     client_state = {0};
       char                  client_youvegotmail = 0;
static int                   client_playerid = -1;
struct network_connection   *client_serverconn = NULL;
struct network_buffer        client_sendbuff, client_recvbuff;

void client_handle_local_recv(short type, const void *data)
{
    switch(type)
    {
        case MSG_GM_STATE:
            game_state_copy_into(&client_state, data);
            client_youvegotmail = 1;
            break;
        case MSG_GM_START:
            client_playerid = *(unsigned char *)data;
            break;
        default:
            return;
    }
}

void client_start(struct network_connection *conn)
{
    client_serverconn = conn;
    network_buffer_init(&client_sendbuff, 128);
    network_buffer_init(&client_recvbuff, 2048);
    game_state_init(&client_state);

    if (!client_serverconn)
        server_register_local(&client_handle_local_recv);
}

void client_process_recvbuff()
{
    struct network_header header;

    if (!network_buffer_peek_hdrmsg(&client_recvbuff))
        return;

    network_header_deserialize(&header, &client_recvbuff.head);

    switch (header.type) {
        case MSG_GM_START:
            client_playerid = network_unpack_u8(&client_recvbuff.head);
            return;
        case MSG_GM_STATE:
            game_state_deserialize(&client_state, &client_recvbuff.head);
            client_youvegotmail = 1;
            return;
        default:
            printf("Client: unhandled MSG type %d\n", header.type);
            return;
    }
}
void client_update_state()
{
    while (NETWORK_BUFFER_LEN(client_recvbuff))
        client_process_recvbuff();
}

struct act get_act()
{
    struct act act;
    char input[16];

    printf("===============================================\n"
            "draw: d<CR>, play: p[card_id]<CR>, end turn: e<CR>\n"
           ">> ");
    scanf(" %[^\n]s", input);

    switch (input[0]) {
        case 'd':
            act.type = ACT_DRAW;
            break;
        case 'p':
            act.args.play.card_id = atoi(input+1);
            act.type = ACT_PLAY;
            break;
        case 'e':
            act.type = ACT_END_TURN;
            break;
    }

    return act;
}

void client_act()
{
    struct network_header header = {
        .version = NETMSG_VER,
        .type = MSG_GM_ACT,
        .len = 0
    };
    struct act act;
retry:
    act = get_act();

    if (client_serverconn) {
        header.len = act_serialize(NULL, act);

        network_buffer_make_space(&client_sendbuff, header.len + NETHDR_SERIALIZED_SIZE);
        network_header_serialize(&client_sendbuff.tail, &header);
        act_serialize(&client_sendbuff.tail, act);
    } else {
        if (server_handle_act(act) != 0) goto retry;
    }
}

void client_update()
{
    network_connection_sendrecv_nb(client_serverconn, &client_sendbuff, &client_recvbuff);

    client_update_state(); 
    if (client_youvegotmail) {
        client_youvegotmail = 0;

        PRINTF_RESET();
        log_game_state(global_char_buff, sizeof(global_char_buff), &client_state, 1);
        printf("%s\n", global_char_buff);

        if (client_state.active_player_index == client_playerid) {
            client_act();
            return;
        }
        printf("Waiting for update... ");
    }

    print_spinner(); usleep(100 * 1000);
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
    }

    network_connection_destroy(conn);
}

void main_host(short port)
{
    const int MAX_PLAYER = 3;
    server_init(port, MAX_PLAYER); printf("Server listening on port %d...\n", port);
    client_start(NULL);
    while (1) {
        server_update();
        if (server_is_idling())
            client_update();
    }
}

int main(int argc, char *argv[])
{
    PRINTF_RESET();
    printf("CUNO Start.\n");

    if (argc == 3) {
        /* if (strcmp(argv[2], "-s") == 0) {
            main_host(atoi(argv[1]));
            return 0;
        } */
        main_client(argv[1], atoi(argv[2]));
    } else if (argc == 2) {
        main_host(atoi(argv[1]));
    } else {
        return 1;
    }
    return 0;
}
