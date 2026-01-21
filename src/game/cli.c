#include <stdio.h>
#include <unistd.h>
#include "engine/system/network.h"
#include "engine/utils.h"
#include "serialize.h"
#include "logic.h"

#define PRINTF_RESET() printf("\x1b[2J\x1b[H")
#define NETMSG_VER 0
#define DEFAULT_RECV_SIZE 400

enum netmessage_type {
    NETMSG_UNKNOWN = -1,
    NETMSG_GMSTATE,
    NETMSG_ACT,
};

static char  global_char_buff[2048];
static const char  *SPINNER[] = {"⌜", "⌝", "⌟", "⌞"};

void print_spinner()
{
    static int spinner_idx = 0;
    printf("%s\x1b[1D", SPINNER[spinner_idx]);
    fflush(stdout);
    spinner_idx = (spinner_idx + 1) % ARRAY_SIZE(SPINNER);
}

void netupdate_sendrecv(struct network_connection *conn, struct network_buffer *sendbuff, struct network_buffer *recvbuff)
{
    char status;

    if (!conn)
        return;

    status = network_connection_poll(conn, NETWORK_POLLIN | NETWORK_POLLOUT);

    if ((status & NETWORK_POLLOUT) && NETWORK_BUFFER_LEN(*sendbuff))
        network_connection_send(conn, &sendbuff->head, sendbuff->tail);

    if (status & NETWORK_POLLIN) {
        network_buffer_make_space(recvbuff, DEFAULT_RECV_SIZE);
        network_connection_recv(conn, &recvbuff->tail, recvbuff->end);
    }
}

/********* SERVER **********/
static struct game_state             server_state;
static struct network_connection    *server_playerconn[1];
struct network_buffer                server_sendbuff, server_recvbuff;

void server_send_state()
{
    struct network_header header = {
        .version = NETMSG_VER,
        .type = NETMSG_GMSTATE,
        .len = 0,
    };
    size_t total = 0;

    game_state_serialize(NULL, &server_state, &total);
    header.len = total;

    network_buffer_make_space(&server_sendbuff, header.len + NETHDR_SERIALIZED_SIZE);
    network_header_serialize(&server_sendbuff.tail, &header);
    game_state_serialize(&server_sendbuff.tail, &server_state, NULL);

extern char client_youvegotmail;
    client_youvegotmail = 1;
}

void server_process_act(const char *input)
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

void server_try_pop_recvbuff()
{
    static struct network_header header = { .type = NETMSG_UNKNOWN };
    static char input[16];

    switch (header.type) {
        case (uint16_t)NETMSG_UNKNOWN:
            if (NETWORK_BUFFER_LEN(server_recvbuff) < NETHDR_SERIALIZED_SIZE) 
                return;
            network_header_deserialize(&header, &server_recvbuff.head);
            return;
        case NETMSG_ACT:
            if (NETWORK_BUFFER_LEN(server_recvbuff) < header.len) 
                return;
            network_unpack_str(input, &server_recvbuff.head);
            header.type = NETMSG_UNKNOWN;
            server_process_act(input);
            return;
        default:
            printf("Unhandled NETMSG type %d\n", header.type);
            return;
    }
}

void server_start(struct network_listener *listener)
{
    game_state_init(&server_state, 2, 5); 
    network_buffer_init(&server_sendbuff, 512);
    network_buffer_init(&server_recvbuff, 128);

    server_playerconn[0] = network_listener_accept(listener);
    printf("Accepted.\n");
    server_send_state();
}

void server_update()
{
    netupdate_sendrecv(server_playerconn[0], &server_sendbuff, &server_recvbuff);

    while (NETWORK_BUFFER_LEN(server_recvbuff))
        server_try_pop_recvbuff();
}

/********* CLIENT **********/
static struct game_state     client_state = {0};
       char                  client_youvegotmail = 0;
static int                   client_player;
struct network_connection   *client_serverconn = NULL;
struct network_buffer        client_sendbuff, client_recvbuff;

void client_start(struct network_connection *conn)
{
    client_serverconn = conn;
    network_buffer_init(&client_sendbuff, 128);
    network_buffer_init(&client_recvbuff, 1024);
}

void client_try_pop_recvbuff()
{
    static struct network_header header = { .type = NETMSG_UNKNOWN };
    switch (header.type) {
        case (uint16_t)NETMSG_UNKNOWN:
            if (NETWORK_BUFFER_LEN(client_recvbuff) < NETHDR_SERIALIZED_SIZE) 
                return;
            network_header_deserialize(&header, &client_recvbuff.head);
            return;
        case NETMSG_GMSTATE:
            if (NETWORK_BUFFER_LEN(client_recvbuff) < header.len) 
                return;
            game_state_deserialize(&client_state, &client_recvbuff.head);
            header.type = NETMSG_UNKNOWN;
            client_youvegotmail = 1;
            return;
        default:
            printf("Unhandled NETMSG type %d\n", header.type);
            return;
    }
}
void client_update_state()
{
    if (!client_serverconn) {
        client_state = server_state;
        return;
    }

    while (NETWORK_BUFFER_LEN(client_recvbuff))
        client_try_pop_recvbuff();
}

void client_act()
{
    struct network_header header = {
        .version = NETMSG_VER,
        .type = NETMSG_ACT,
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
        network_pack_str(&client_sendbuff.tail, input, NULL);
    } else {
        server_process_act(input);
    }
}

void client_update()
{
    int playerno = client_serverconn ? 1 : 0;

    netupdate_sendrecv(client_serverconn, &client_sendbuff, &client_recvbuff);

    client_update_state(); 
    if (client_youvegotmail) {
        client_youvegotmail = 0;

        PRINTF_RESET();
        log_game_state(global_char_buff, sizeof(global_char_buff), &client_state);
        printf("%s\n", global_char_buff);

        if (client_state.active_player_index == playerno) {
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
    struct network_listener *listener = network_listener_create(port, 3);

    server_start(listener); printf("Server listening on port %d...\n", port);
    client_start(NULL);
    while (1) {
        server_update();
        if (!NETWORK_BUFFER_LEN(server_sendbuff))
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
