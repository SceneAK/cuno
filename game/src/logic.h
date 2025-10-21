#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H
#include <stdlib.h>
#include "gmath.h"
#include "utils.h"

#define is_pickable_color(color) ( 0 <= (color) && (color) <= 3 )
#define PLAYER_MAX 5

typedef unsigned short card_id_t;
struct card {
    card_id_t id;
    enum card_type {
        UNKNOWN = -1,
        NUMBER,
        REVERSE,
        SKIP,
        PICK_COLOR,
        PLUS2,
        PLUS4,
        SWAP
    } type;
    enum card_color {
        BLACK = -1,
        RED,
        GREEN,
        BLUE,
        YELLOW
    } color;
    short num;
};
DEFINE_ARRAY_LIST_WRAPPER(static, struct card, card_list)
struct player {
    char               *name;
    struct card_list    hand;
};
struct game_state {
    card_id_t       card_id_last;

    int             turn;
    char            ended;
    struct player   players[PLAYER_MAX];
    int             player_len;
    int             active_player_index;
    enum action {
        ACT_NONE,
        ACT_PLAY,
        ACT_DRAW,
    } act;

    struct card     top_card;
    int             turn_dir;
    int             skip_pool;
    int             batsu_pool;
};

vec3 card_color_to_rgb(enum card_color color);

void game_state_copy(struct game_state *dst, const struct game_state *src);
void game_state_init(struct game_state *game, int player_len, int deal);
void game_state_deinit(struct game_state *game);

int game_state_can_act_draw(const struct game_state *game);
int game_state_can_act_play(const struct game_state *game, card_id_t card_id);
int game_state_act_draw(struct game_state *game);
int game_state_act_play(struct game_state *game, card_id_t card_id);
int game_state_act_auto(struct game_state *game);
int game_state_end_turn(struct game_state *game);

const char *card_type_to_str(enum card_type card_type);
const char *card_color_to_str(enum card_color color);
size_t log_card(char *buffer, size_t buffer_len, const struct card *card);
size_t log_hand(char *buffer, size_t buffer_len, const struct player *player);
size_t log_game_state(char *buffer, size_t buffer_len, const struct game_state *game);

#endif
