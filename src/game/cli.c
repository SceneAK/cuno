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
        case MSG_GMSTATE:
            game_state_copy_into(&client_state, data);
            client_youvegotmail = 1;
            break;
        case MSG_GMSTART:
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
        case MSG_GMSTATE:

            game_state_deserialize(&client_state, &client_recvbuff.head);

            client_youvegotmail = 1;
            return;
        case MSG_GMSTART:
            client_playerid = network_unpack_u8(&client_recvbuff.head);
            return;
        default:
            printf("Unhandled MSG type %d\n", header.type);
            return;
    }
}
void client_update_state()
{
    while (NETWORK_BUFFER_LEN(client_recvbuff))
        client_process_recvbuff();
}

void client_act()
{
    struct network_header header = {
        .version = NETMSG_VER,
        .type = MSG_ACT,
        .len = 0
    };
    char input[16];
    int temp;

    printf("===============================================\n"
           "draw: d<enter>, play: p[card_id]<enter>\n"
           ">> ");
    scanf(" %c", input);

    header.len = 1;
    if (input[0] == 'p') {
        scanf(" %d", &temp);
        header.len += sprintf((char *)(input+1), "%d", temp);
    }

    if (client_serverconn) {
        network_buffer_make_space(&client_sendbuff, header.len + NETHDR_SERIALIZED_SIZE);
        network_header_serialize(&client_sendbuff.tail, &header);
        network_pack_str(&client_sendbuff.tail, input);
    } else {
        server_process_act(input);
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

    print_spinner(); usleep(200 * 1000);
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
    struct network_listener *listener = network_listener_create(port, 4);

    server_start(listener); printf("Server listening on port %d...\n", port);
    client_start(NULL);
    while (1) {
        server_update();
        if (server_is_idling())
            client_update();
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
