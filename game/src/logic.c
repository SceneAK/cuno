#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "system/log.h"
#include "logic.h"
#include "gmath.h"
#include "utils.h"

#define DIV_255(val) val/255.0f

vec3 card_color_to_rgb(enum card_color color)
{
    switch (color) {
        case RED:
            return vec3_create(DIV_255(255), DIV_255(45), DIV_255(45));
        case GREEN:
            return vec3_create(DIV_255(44), DIV_255(255), DIV_255(83));
        case BLUE:
            return vec3_create(DIV_255(44), DIV_255(114), DIV_255(255));
        case YELLOW:
            return vec3_create(DIV_255(255), DIV_255(220), DIV_255(44));
        case BLACK:
            return VEC3_ZERO;
        default:
            return vec3_create(DIV_255(255), DIV_255(0), DIV_255(255));
    }
}

static void rand_card(struct card *card, card_id_t id)
{
    const float RATIO = RAND_MAX / 110.0;
    int random;

    random = rand();
    card->id    = id;
    card->color = random % 4;

    if ( random <= RATIO * 76 ) {
        card->type = NUMBER;
        card->num  = random % 10;
    }
    else if ( random <= RATIO * 84 )
        card->type = REVERSE;

    else if ( random <= RATIO * 92 )
        card->type = SKIP;

    else if ( random <= RATIO * 100 )
        card->type = PLUS2;

    else if ( random <= RATIO * 104 ) {
        card->type  = PICK_COLOR;
        card->color = BLACK;
    }
    else if ( random <= RATIO * 108 ) {
        card->type  = PLUS4;
        card->color = BLACK;
    }
    else if ( random <= RATIO * 110 ) {
        /*card->type  = SWAP;*/
        card->type  = PICK_COLOR;
        card->color = BLACK;
    }
}

static void append_player_rand_cards(struct game_state *state, int player_index, int amount)
{
    int i;
    struct card *card = card_list_emplace(&state->players[player_index].hand, amount);

    for (i = 0; i < amount; i++)
        rand_card(card + i, state->card_id_last++);
}

static void remove_player_card(struct player *player, card_id_t card)
{
    size_t index = array_list_index_of(id, &player->hand, card);
}

#define card_same_color(a, b) ((a).color == (b).color)
#define card_same_type(a, b) ((a).type == (b).type) \
                        && ((a).type != NUMBER || (a).num == (b).num)

static int play_card_effect(struct game_state *game, struct card *card, enum card_color arg_color)
{
    switch (card->type) {
        case REVERSE:
            game->turn_dir *= -1;
            break;

        case SKIP:
            game->skip_pool += 1;
            break;

        case PLUS2:
            game->batsu_pool += 2;
            break;

        case PLUS4:
            game->batsu_pool += 4;
        case PICK_COLOR:
            if (!is_pickable_color(arg_color))
                return -1;
            card->color = arg_color;
            break;

        default:
            break;
    }
    return 0;
}

void game_state_init(struct game_state *game, int player_len, int deal)
{
    int i;
    game->active_player_index = 0;
    game->turn          = 0;
    game->player_len    = min(player_len, PLAYER_MAX);
    game->ended         = 0;
    game->turn_dir      = 1;
    game->skip_pool     = 0;
    game->batsu_pool    = 0;
    memset(game->players, 0, sizeof(game->players));

    for (i = 0; i < game->player_len; i++) {
        card_list_init(&game->players[i].hand, deal);
        append_player_rand_cards(game, i, deal);
    }

    do
        rand_card(&game->top_card, game->card_id_last++); 
    while (game->top_card.type == PLUS4 || game->top_card.type == PICK_COLOR);
    play_card_effect(game, &game->top_card, -2);
}

void game_state_deinit(struct game_state *game)
{
    int i;
    for (i = 0; i < game->player_len; i++)
        card_list_deinit(&game->players[i].hand);
}

void game_state_copy(struct game_state *dst, const struct game_state *src)
{
    struct game_state *old_dst = dst;
    struct game_state new_dst;
    int i;

    memcpy(&new_dst, src, sizeof(struct game_state));

    for (i = 0; i < old_dst->player_len && i < src->player_len; i++) {
        card_list_copy(&old_dst->players[i].hand, &src->players[i].hand);
        new_dst.players[i].hand = old_dst->players[i].hand;
    }
    for (; i < old_dst->player_len; i++)
        card_list_deinit(&old_dst->players[i].hand);

    for (; i < src->player_len; i++)
        new_dst.players[i].hand = card_list_clone(&src->players[i].hand);

    memcpy(dst, &new_dst, sizeof(struct game_state));
}

int game_state_can_act_draw(const struct game_state *game)
{
    return !game->ended && !game->act;
}

/* ACTS */
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

int game_state_can_act_play(const struct game_state *game, card_id_t card_id)
{
    int                     i, j, 
                            same_type, same_color;
    const struct card      *card;
    int                     card_index;
    const struct card_list *hand;

    if (game->ended)
        return 0;

    hand = &game->players[game->active_player_index].hand;

    card_index = array_list_index_of(id, hand, card_id);
    if (card_index == -1)
        return 0;

    card = hand->elements + card_index;

    same_type = card_same_type(game->top_card, *card);
    same_color = card_same_color(game->top_card, *card);

    if (game->act == ACT_PLAY)
        return same_type;

    if (game->batsu_pool && !same_type && card->type != PLUS4)
        return 0;

    return same_color || same_type || card->color == BLACK;
}

int game_state_act_play(struct game_state *game, card_id_t card_id)
{
    struct player  *player  = game->players + game->active_player_index;
    size_t          index   = array_list_index_of(id, &player->hand, card_id);

    if (!game_state_can_act_play(game, card_id))
        return -1;

    game->top_card = player->hand.elements[index];
    play_card_effect(game, &game->top_card, RED);
    card_list_remove_swp(&player->hand, &index, 1);

    game->act = ACT_PLAY;
    return 0;
}
card_id_t *game_state_act_play_mult(struct game_state *game, card_id_t *card_ids, size_t len)
{
    int i = 0;
    for (i = 0; i < len; i++) {
        if (game_state_act_play(game, card_ids[i]) == 0)
            continue;

        return card_ids + i;
    }
    return NULL;
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


int game_state_act_auto(struct game_state *game)
{
    struct player      *player = game->players + game->active_player_index;
    size_t              i;

    if (game->act) 
        return 0;

    for (i = 0; i < player->hand.len; i++) {
        if (game_state_act_play(game, player->hand.elements[i].id) == 0)
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
        case NUMBER:
            return "number";
        case REVERSE:
            return "reverse";
        case SKIP:
            return "skip";
        case PLUS2:
            return "plus 2";
        case PICK_COLOR:
            return "pick color";
        case PLUS4:
            return "plus 4";
        case SWAP:
            return "swap";
        default:
            return "Undefined card_type";
    }
}
const char *card_color_to_str(enum card_color color)
{
    switch (color) {
        case RED:
            return "red";
        case GREEN:
            return "green";
        case BLUE:
            return "blue";
        case YELLOW:
            return "yellow";
        case BLACK:
            return "black";
        default:
            return "Undefined card_color";
    }
}

size_t log_card(char *buffer, size_t buffer_len, const struct card *card)
{
    const char *card_type = card_type_to_str(card->type);
    const char *card_color = card_color_to_str(card->color);
    if (card->type == NUMBER)
        return snprintf(buffer, buffer_len, "card(%d) [%s] (%s) (%i)\n", card->id, card_type, card_color, card->num);
    else 
        return snprintf(buffer, buffer_len, "card(%d) [%s] (%s)\n",card->id, card_type, card_color);
}
size_t log_hand(char *buffer, size_t buffer_len, const struct player *player)
{
    int i;
    size_t total = 0;
    
    for (i = 0; i < player->hand.len; i++) {
        total += log_card(buffer + total, buffer_len - total, player->hand.elements + i);
    }
    total += snprintf(buffer + total, buffer_len - total, "Total of %lu cards\n", player->hand.len);
    return total;
}
size_t log_game_state(char *buffer, size_t buffer_len, const struct game_state *game)
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
        " - active_player_index: %i\n",
        game->turn,
        game->ended,
        game->act,
        game->turn_dir,
        game->skip_pool,
        game->batsu_pool,
        game->active_player_index);
    total += log_card(buffer + total, buffer_len - total, &game->top_card);
    for (i = 0; i < game->player_len; i++) {
        total += snprintf(buffer + total, buffer_len - total, "player [%i]:\n", i);
        total += log_hand(buffer + total, buffer_len - total, game->players + i);
    }
    return total;
}
