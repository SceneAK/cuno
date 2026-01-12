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

void main_server(const char *ipv4addr)
{
    struct network_listener *listener;
}
void main_client(const char *ipv4addr, short port)
{
    struct network_connection *conn;

    /* conn = network_connection_create(ipv4addr, port); */

    while (1) {
        getchar();
    };
    network_connection_destroy(conn);
}

int main(int argc, char *argv[])
{
    printf("Hello CUNO\n");

    if (argc == 2)
        main_client(argv[0], atoi(argv[1]));
    else if (argc == 1)
        main_server(argv[0]);
    else
        return 1;

    return 0;
}
