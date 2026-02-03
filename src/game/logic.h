#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H
#include <stdlib.h>
#include "engine/math.h"
#include "engine/array_list.h"

#define is_pickable_color(color) ( 0 <= (color) && (color) <= 3 )
#define PLAYER_NAME_MAX 64
#define PLAYER_MAX 5
#define PLAY_ARG_MAX 6
#define CARD_HIDE(card) do { (card).type = CARD_UNKNOWN; (card).num = -1; (card).color = CARD_COLOR_MAX; } while(0)

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
typedef unsigned short card_id_t;
struct card {
    card_id_t           id;
    enum card_type      type;
    enum card_color     color;
    unsigned short      num;
};
DEFINE_ARRAY_LIST_WRAPPER(static, struct card, card_list)
struct player {
    unsigned int        id;
    char                name[PLAYER_NAME_MAX];
    struct card_list    hand;
};
enum act_type {
    ACT_NONE,
    ACT_PLAY,
    ACT_DRAW,
    ACT_END_TURN,
};
struct act_args_play {
    card_id_t           card_id;
    enum card_color     color;
};
union act_args {
    struct act_args_play play;
};
struct act {
    enum act_type       type;
    union act_args      args;
};
struct game_state {
    struct player       players[PLAYER_MAX];
    unsigned int        player_len;

    card_id_t           card_id_last;

    int                 turn;
    int                 ended;
    unsigned int        active_player_index;
    enum act_type       curr_act;

    struct card         top_card;
    int                 turn_dir;
    unsigned int        skip_pool;
    unsigned int        batsu_pool;
};


void game_state_init(struct game_state *game);
void game_state_deinit(struct game_state *game);
void game_state_copy_into(struct game_state *dst, const struct game_state *src);
void game_state_start(struct game_state *game, int player_len, int deal);
void game_state_for_player(struct game_state *game, int player_id);

const struct card *game_state_get_card(const struct game_state *game, card_id_t card_id);
int game_state_can_act(const struct game_state *game, struct act act);
int game_state_act(struct game_state *game, struct act act);

int act_auto(const struct game_state *game, struct act *act);

const struct card *active_player_find_card(const struct game_state *game, card_id_t card_id);
static inline int find_player_idx(const struct game_state *game, int player_id);

int card_needs_color_arg(const struct card *card);
vec3 card_color_to_rgb(enum card_color color);
const char *card_type_to_str(enum card_type card_type);
const char *card_color_to_str(enum card_color color);
size_t log_card(char *buffer, size_t buffer_len, const struct card *card, char hide_unknown);
size_t log_hand(char *buffer, size_t buffer_len, const struct player *player, char hide_unknowns);
size_t log_game_state(char *buffer, size_t buffer_len, const struct game_state *game, char hide_unkowns);

static inline int find_player_idx(const struct game_state *state, int player_id)
{
    int i;
    for (i = 0; i < state->player_len; i++) {
        if (state->players[i].id == player_id)
            return i;
    }
    return -1;
}

#endif
