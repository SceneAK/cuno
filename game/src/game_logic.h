#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H
#include <stdlib.h>
#include "game_math.h"
#include "utils.h"

#define IS_PICKABLE_COLOR(color) ( 0 <= (color) && (color) <= 3 )

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
    short               id;
    char               *name;
    struct card_list    hand;
};
struct game_state {
    char            ended;
    struct player  *players;
    int             player_len;
    int             active_player_index;
    enum action {
        NONE,
        PLAY,
        DRAW,
    } acted;

    struct card     top_card;
    int             turn_dir;    int             skip_pool;
    int             batsu_pool;
};

vec3 card_color_to_rgb(enum card_color color);
int card_find_index(const struct player *player, card_id_t id);
void cards_find_indices(int *indices, const struct player *player, size_t *ids, int len);

void game_state_init(struct game_state *game, int player_len);
void deal_cards(struct game_state *game, int deal_per_player);
int is_legal_draw(const struct game_state *game);
int is_legal_play(const struct game_state *game, const size_t *indices, int len);
int act_draw_cards(struct game_state *game, size_t *amount_drawn);
int act_play_card(struct game_state *game, const size_t *indices, int len);
int end_turn(struct game_state *game);

int supersmartAI_act(struct game_state *game);

const char *card_type_to_str(enum card_type card_type);
const char *card_color_to_str(enum card_color color);
size_t log_card(char *buffer, size_t buffer_len, const struct card *card);
size_t log_hand(char *buffer, size_t buffer_len, const struct player *player);
size_t log_game_state(char *buffer, size_t buffer_len, const struct game_state *game);

#endif
