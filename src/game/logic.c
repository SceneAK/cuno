#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "logic.h"
#include "engine/utils.h"

#define DIV_255(val) val/255.0f
#define card_same_color(a, b) ((a).color == (b).color)
#define card_same_type(a, b) ((a).type == (b).type) \
                        && ((a).type != CARD_NUMBER || (a).num == (b).num)

vec3 card_color_to_rgb(enum card_color color)
{
    switch (color) {
        case CARD_COLOR_RED:
            return vec3_create(DIV_255(255), DIV_255(45), DIV_255(45));
        case CARD_COLOR_GREEN:
            return vec3_create(DIV_255(44), DIV_255(255), DIV_255(83));
        case CARD_COLOR_BLUE:
            return vec3_create(DIV_255(44), DIV_255(114), DIV_255(255));
        case CARD_COLOR_YELLOW:
            return vec3_create(DIV_255(255), DIV_255(220), DIV_255(44));
        case CARD_COLOR_BLACK:
            return VEC3_ZERO;
        default:
            return vec3_create(DIV_255(255), DIV_255(0), DIV_255(255));
    }
}

const struct card *active_player_find_card(const struct game_state *game, card_id_t card_id)
{
    int i;
    const struct player *player = game->players + game->active_player_index;

    for (i = 0; i <  player->hand.len; i++) {
        if (player->hand.elems[i].id == card_id)
            return player->hand.elems + i;
    }
    return NULL;
}

static void card_random(struct card *card, card_id_t id)
{
    const float RATIO = RAND_MAX / 110.0;
    int random = rand();

    card->id            = id;
    card->color         = random % 4;

    if ( random <= RATIO * 76 ) {
        card->type = CARD_NUMBER;
        card->num  = random % 10;
    }
    else if ( random <= RATIO * 84 )
        card->type = CARD_REVERSE;

    else if ( random <= RATIO * 92 )
        card->type = CARD_SKIP;

    else if ( random <= RATIO * 100 )
        card->type = CARD_PLUS2;
    
    else if ( random <= RATIO * 104 ) {
        card->color = CARD_COLOR_BLACK;
        card->type  = CARD_PICK_COLOR;
    }
    else if ( random <= RATIO * 108 ) {
        card->color = CARD_COLOR_BLACK;
        card->type  = CARD_PLUS4;
    }
    else if ( random <= RATIO * 110 ) {
        card->color = CARD_COLOR_BLACK;
        /* card->type  = CARD_SWAP; */
        card->type  = CARD_PICK_COLOR;
    }
}

static void append_player_rand_cards(struct game_state *state, int player_index, int amount)
{
    int i;
    struct card *card = card_list_emplace(&state->players[player_index].hand, amount);

    for (i = 0; i < amount; i++)
        card_random(card + i, state->card_id_last++);
}

void card_get_arg_type_specs(enum play_arg_type arg_specs[PLAY_ARG_MAX], const struct card *card)
{
    memset(arg_specs, 0, sizeof(enum play_arg_type) * PLAY_ARG_MAX);
    if (card->color == CARD_COLOR_BLACK)
        arg_specs[0] = PLAY_ARG_COLOR;
    if (card->type == CARD_SWAP)
        arg_specs[1] = PLAY_ARG_PLAYER;
}
static int play_card_effect(struct game_state *game, struct card *card, const struct play_arg args[PLAY_ARG_MAX])
{
    struct card_list temp;

    if (card->color == CARD_COLOR_BLACK && args[0].type == PLAY_ARG_COLOR)
        card->color = args[0].u.color;

    switch (card->type) {
        case CARD_REVERSE:
            game->turn_dir *= -1;
            break;

        case CARD_SKIP:
            game->skip_pool += 1;
            break;

        case CARD_PLUS2:
            game->batsu_pool += 2;
            break;

        case CARD_PLUS4:
            game->batsu_pool += 4;
            break;

        case CARD_SWAP:
            if (args[1].type != PLAY_ARG_PLAYER)
                break;
            temp = game->players[game->active_player_index].hand;
            game->players[game->active_player_index].hand = game->players[args[1].u.player_idx].hand;
            game->players[args[1].u.player_idx].hand = temp;
            break;

        default:
            break;
    }
    return 0;
}

void game_state_init(struct game_state *game)
{
    game->active_player_index   = 0;
    game->turn                  = 0;
    game->ended                 = 0;
    game->turn_dir              = 1;
    game->skip_pool             = 0;
    game->batsu_pool            = 0;
    game->player_len            = 0;
    memset(game->players, 0, sizeof(game->players));
}

void game_state_start(struct game_state *game, int player_len, int deal)
{
    int i;

    game->player_len = min(player_len, PLAYER_MAX);

    for (i = 0; i < game->player_len; i++) {
        game->players[i].id = i;
        card_list_init(&game->players[i].hand, deal);
        append_player_rand_cards(game, i, deal);
    }

    do
        card_random(&game->top_card, game->card_id_last++); 
    while (game->top_card.type == CARD_PICK_COLOR);
    play_card_effect(game, &game->top_card, PLAY_ARGS_ZERO);
}

void game_state_deinit(struct game_state *game)
{
    int i;
    for (i = 0; i < game->player_len; i++)
        card_list_deinit(&game->players[i].hand);
}

void game_state_copy_into(struct game_state *dst, const struct game_state *src)
{
    struct game_state *old_dst = dst;
    struct game_state new_dst;
    int i;

    memcpy(&new_dst, src, sizeof(struct game_state));

    for (i = 0; i < old_dst->player_len && i < src->player_len; i++) {
        card_list_copy_into(&old_dst->players[i].hand, &src->players[i].hand);
        new_dst.players[i].hand = old_dst->players[i].hand;
    }
    for (; i < old_dst->player_len; i++)
        card_list_deinit(&old_dst->players[i].hand);

    for (; i < src->player_len; i++)
        new_dst.players[i].hand = card_list_clone(&src->players[i].hand);

    memcpy(dst, &new_dst, sizeof(struct game_state));
}

void game_state_for_player(struct game_state *state, int player_id)
{
    int i, j;
    for (i = 0; i < state->player_len; i++) {
        if (state->players[i].id == player_id)
            continue;

        for (j = 0; j < state->players[i].hand.len; j++) {
            CARD_HIDE(state->players[i].hand.elems[j]);
        }
    }
}

/* ACTS */
int game_state_can_act_draw(const struct game_state *game)
{
    return !game->ended && !game->act;
}
int game_state_act_draw(struct game_state *game)
{
    int amount = 1;

    if (!game_state_can_act_draw(game))
        return -1;

    if (game->batsu_pool) {
        amount = game->batsu_pool;
        game->batsu_pool = 0;
    }

    append_player_rand_cards(game, game->active_player_index, amount);

    game->act = ACT_DRAW;
    return 0;
}

int game_state_can_act_play(const struct game_state *game, struct act_play play)
{
    int                     i, j, 
                            same_type, same_color;
    const struct card      *card = NULL;
    const struct card_list *hand;
    enum play_arg_type      specs[PLAY_ARG_MAX];

    if (game->ended)
        return 0;

    hand = &game->players[game->active_player_index].hand;
    for (i = 0; i < hand->len; i++) {
        if (hand->elems[i].id == play.card_id)
            break;
    }
    if (i == hand->len)
        return 0;
    card = hand->elems + i;

    card_get_arg_type_specs(specs, card);
    for (i = 0; i < PLAY_ARG_MAX && specs[i] != PLAY_ARG_NONE; i++) {
        if (play.args[i].type != specs[i])
            return 0;
    }

    same_type   = card_same_type(game->top_card, *card);
    same_color  = card_same_color(game->top_card, *card);

    if (game->act == ACT_PLAY)
        return same_type;

    if (game->batsu_pool && !same_type && card->type != CARD_PLUS4)
        return 0;

    return same_color || same_type || card->color == CARD_COLOR_BLACK;
}

int game_state_act_play(struct game_state *game, struct act_play play)
{
    struct player  *player  = game->players + game->active_player_index;
    size_t  index;

    for (index = 0; index < player->hand.len; index++) {
        if (player->hand.elems[index].id == play.card_id)
            break;
    }

    if (index == player->hand.len)
        return -1;

    if (!game_state_can_act_play(game, play))
        return -1;

    game->top_card = player->hand.elems[index];
    play_card_effect(game, &game->top_card, play.args);
    card_list_remove_swp(&player->hand, &index, 1);

    game->act = ACT_PLAY;
    return 0;
}

int game_state_end_turn(struct game_state *game)
{
    int increment;

    if (game->ended || !game->act)
        return -1;

    if (game->players[game->active_player_index].hand.len == 0) {
        game->ended = 1;
        return 0;
    }

    increment = (1 + game->skip_pool) * game->turn_dir;
    game->skip_pool = 0;

    game->active_player_index = (game->active_player_index + increment) % game->player_len;
    if (game->active_player_index < 0)
        game->active_player_index += game->player_len;

    game->act = ACT_NONE;
    game->turn++;
    return 0;
}


static enum card_color find_frequent_color(const struct card_list *hand)
{
    int             tally[CARD_COLOR_MAX] = {0};
    enum card_color most_frequent = 0;
    int             i;

    for (i = 0; i < hand->len; i++) {
        if (!is_pickable_color(hand->elems[i].color))
            continue;

        tally[hand->elems[i].color]++;
        if (i == 0 || tally[most_frequent] < tally[hand->elems[i].color])
            most_frequent = hand->elems[i].color;
    }
    return most_frequent;
}
int game_state_act_auto(struct game_state *game)
{
    struct player      *player = game->players + game->active_player_index;
    size_t              i;
    enum play_arg_type  arg_specs[PLAY_ARG_MAX];
    struct act_play     act_play = {0};
    
    act_play.args[0].type = PLAY_ARG_COLOR;
    act_play.args[1].type = PLAY_ARG_PLAYER;

    if (game->act) 
        return 0;

    for (i = 0; i < player->hand.len; i++) {
        act_play.card_id = player->hand.elems[i].id;

        card_get_arg_type_specs(arg_specs, player->hand.elems + i);
        if (arg_specs[0] == PLAY_ARG_COLOR)
            act_play.args[0].u.color = find_frequent_color(&player->hand);

        if (game_state_act_play(game, act_play) == 0)
            i = 0;
    } 

    if (game->act)
        return 0;
    else
        return game_state_act_draw(game) == 0 ? 0 : -2;
}

/* LOGGING & DEBUGGING */
const char *card_type_to_str(enum card_type card_type)
{
    switch (card_type) {
        case CARD_UNKNOWN:
            return "unknown";
        case CARD_NUMBER:
            return "number";
        case CARD_REVERSE:
            return "reverse";
        case CARD_SKIP:
            return "skip";
        case CARD_PLUS2:
            return "plus 2";
        case CARD_PICK_COLOR:
            return "pick color";
        case CARD_PLUS4:
            return "plus 4";
        case CARD_SWAP:
            return "swap";
        default:
            return "undefined";
    }
}
const char *card_color_to_str(enum card_color color)
{
    switch (color) {
        case CARD_COLOR_RED:
            return "red";
        case CARD_COLOR_GREEN:
            return "green";
        case CARD_COLOR_BLUE:
            return "blue";
        case CARD_COLOR_YELLOW:
            return "yellow";
        case CARD_COLOR_BLACK:
            return "black";
        default:
            return "undefined";
    }
}

size_t log_card(char *buffer, size_t buffer_len, const struct card *card, char hide_unknowns)
{
    const char *card_type = card_type_to_str(card->type);
    const char *card_color = card_color_to_str(card->color);
    if (card->type == CARD_UNKNOWN && hide_unknowns)
        return 0;
    if (card->type == CARD_NUMBER)
        return snprintf(buffer, buffer_len, "card(%d) [%s] (%s) (%i)\n", card->id, card_type, card_color, card->num);
    else
        return snprintf(buffer, buffer_len, "card(%d) [%s] (%s)\n",card->id, card_type, card_color);
}
size_t log_hand(char *buffer, size_t buffer_len, const struct player *player, char hide_unknowns)
{
    int i;
    size_t total = 0;
    
    for (i = 0; i < player->hand.len; i++) {
        total += log_card(buffer + total, buffer_len - total, player->hand.elems + i, hide_unknowns);
    }
    return total;
}
size_t log_game_state(char *buffer, size_t buffer_len, const struct game_state *game, char hide_unknowns)
{
    int i;
    size_t total;

    total = snprintf(buffer, buffer_len, 
        "GAME_STATE\n"
        "========== TURN %d =========\n"
        " - ended: %i\n"
        " - act: %i\n"
        " - turn_dir: %i\n"
        " - skip_pool: %i\n"
        " - batsu_pool: %i\n"
        " - active_player_index: %i\n"
        " - top_card: ",
        game->turn,
        game->ended,
        game->act,
        game->turn_dir,
        game->skip_pool,
        game->batsu_pool,
        game->active_player_index);
    total += log_card(buffer + total, buffer_len - total, &game->top_card, hide_unknowns);
    for (i = 0; i < game->player_len; i++) {
        total += snprintf(buffer + total, buffer_len - total, "Player %i: (total %lu)\n", i, game->players[i].hand.len);
        total += log_hand(buffer + total, buffer_len - total, game->players + i, hide_unknowns);
    }
    return total;
}
