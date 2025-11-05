#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H
#include <stdlib.h>
#include "gmath.h"
#include "utils.h"

#define is_pickable_color(color) ( 0 <= (color) && (color) <= 3 )
#define PLAYER_MAX 5
#define PLAY_ARG_MAX 6

enum card_color {
    CARD_COLOR_BLACK = -1,
    CARD_COLOR_RED,
    CARD_COLOR_GREEN,
    CARD_COLOR_BLUE,
    CARD_COLOR_YELLOW,
    CARD_COLOR_MAX,
};
enum card_type {
    CARD_UNKNOWN = -1,
    CARD_NUMBER,
    CARD_REVERSE,
    CARD_SKIP,
    CARD_PICK_COLOR,
    CARD_PLUS2,
    CARD_PLUS4,
    CARD_SWAP,
    CARD_TYPE_MAX,
};
enum play_arg_type {
    PLAY_ARG_NONE,
    PLAY_ARG_COLOR,
    PLAY_ARG_PLAYER,
};
struct play_arg {
    enum play_arg_type  type;
    union {
        enum card_color color;
        int             player_idx;
    } u;
};
typedef unsigned short card_id_t;
struct card {
    card_id_t           id;
    enum card_type      type;
    enum card_color     color;
    unsigned short      num;
};
DEFINE_ARRAY_LIST_WRAPPER(static, struct card, card_list)
struct player {
    char               *name;
    struct card_list    hand;
};
enum act_type {
    ACT_NONE,
    ACT_PLAY,
    ACT_DRAW,
};
struct act_play {
    card_id_t           card_id;
    struct play_arg     args[PLAY_ARG_MAX];
};
struct game_state {
    card_id_t           card_id_last;

    int                 turn;
    char                ended;
    struct player       players[PLAYER_MAX];
    int                 player_len;
    int                 active_player_index;
    enum act_type       act;

    struct card         top_card;
    int                 turn_dir;
    int                 skip_pool;
    int                 batsu_pool;
};

static const struct play_arg PLAY_ARG_ZERO = {0};
static const struct play_arg PLAY_ARGS_ZERO[PLAY_ARG_MAX] = {0};
static const struct act_play ACT_PLAY_ZERO = {0};

void game_state_copy(struct game_state *dst, const struct game_state *src);
void game_state_init(struct game_state *game, int player_len, int deal);
void game_state_deinit(struct game_state *game);

const struct card *game_state_get_card(const struct game_state *game, card_id_t card_id);
int game_state_can_act_draw(const struct game_state *game);
int game_state_can_act_play(const struct game_state *game, struct act_play play);
int game_state_act_draw(struct game_state *game);
int game_state_act_play(struct game_state *game, struct act_play play);
int game_state_act_auto(struct game_state *game);
int game_state_end_turn(struct game_state *game);

const struct card *active_player_find_card(const struct game_state *game, card_id_t card_id);

void card_get_arg_type_specs(enum play_arg_type arg_specs[PLAY_ARG_MAX], const struct card *card);
vec3 card_color_to_rgb(enum card_color color);
const char *card_type_to_str(enum card_type card_type);
const char *card_color_to_str(enum card_color color);
size_t log_card(char *buffer, size_t buffer_len, const struct card *card);
size_t log_hand(char *buffer, size_t buffer_len, const struct player *player);
size_t log_game_state(char *buffer, size_t buffer_len, const struct game_state *game);

#endif
