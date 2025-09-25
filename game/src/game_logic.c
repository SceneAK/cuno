#include <stdlib.h>
#include "system/logging.h"
#include "game_logic.h"
#include "game_math.h"

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

static card_id_t card_count = 0;
static void rand_card(struct card *card)
{
    const float RATIO = RAND_MAX / 110;
    int random;

    random = rand();
    card->id    = card_count++;
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
        card->type  = SWAP;
        card->color = BLACK;
    }
}
static void rand_cards(struct card *cards, int amount)
{
    int i;
    for (i = 0; i < amount; i++) {
        rand_card(cards + i);
    }
}

int find_index(const struct player *player, card_id_t id)
{
    int i;

    for (i = 0; i < player->hand.len; i++) {
        if (player->hand.elements[i].id == id)
            return i;
    }
    return -1;
}

void find_indices(int *indices, const struct player *player, size_t *ids, int len)
{
    int i, j, indices_index = 0;

    for (i = 0; i < player->hand.len; i++) {
        for (j = 0; j < len; j++)
            if (player->hand.elements[i].id == ids[j])
                indices[indices_index++] = i;
    }
}

static void append_cards(struct player *player, int amount)
{
    card_list_append_new(&player->hand, amount);
    rand_cards(player->hand.elements + player->hand.len - amount, amount);
}

static void remove_cards(struct player *player, const size_t *indices, size_t len)
{
    card_list_remove_swp(&player->hand, indices, len);
}

#define IS_SAME_COLOR(a, b) ((a).color == (b).color)
#define IS_SAME_TYPE(a, b) ((a).type == (b).type) \
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
            if (!IS_PICKABLE_COLOR(arg_color))
                return -1;
            card->color = arg_color;
            break;

        default:
            break;
    }
    return 0;
}

void game_state_init(struct game_state *game, int player_len)
{
    game->active_player_index = 0;
    game->player_len    = player_len;
    game->players       = malloc( game->player_len*sizeof(struct player) );
    game->ended         = 0;
    game->turn_dir      = 1;
    game->skip_pool     = 0;
    game->batsu_pool    = 0;
    srand(1);
    do 
        rand_card(&game->top_card); 
    while (game->top_card.type == PLUS4 || game->top_card.type == PICK_COLOR);
    play_card_effect(game, &game->top_card, -2);
}

void deal_cards(struct game_state *game, int deal_per_player)
{
    int i;

    for (i = 0; i < game->player_len; i++) {
        game->players[i].id = i;
        card_list_init(&game->players[i].hand, deal_per_player);

        append_cards(game->players + i, deal_per_player);
    }
}

int is_legal_draw(const struct game_state *game)
{
    return !game->ended && !game->acted;
}
/* ACTS */
int act_draw_cards(struct game_state *game, size_t *amount_drawn)
{
    struct player *player = game->players + game->active_player_index;
    *amount_drawn = 1;

    if (!is_legal_draw(game))
        return -1;

    if (game->batsu_pool) {
        *amount_drawn = game->batsu_pool;
        game->batsu_pool = 0;
    }

    append_cards(player, *amount_drawn);

    game->acted = DRAW;
    return 0;
}


int is_legal_play(const struct game_state *game, const size_t *indices, int len)
{
    int i, j, same_type, same_color;
    const struct card *active_hand, *card, *previous_card;

    if (game->ended || !len)
        return 0;

    previous_card = &game->top_card;
    active_hand = game->players[game->active_player_index].hand.elements;

    for (i = 0; i < len; i++) {
        card = active_hand + indices[i];
        same_type = IS_SAME_TYPE(*previous_card, *card);
        same_color = IS_SAME_COLOR(*previous_card, *card);

        if (i == 0) {
            if (game->batsu_pool && !same_type && card->type != PLUS4)
                return 0;
            if (!same_color && !same_type && card->color != BLACK)
                return 0;
        }
        else if (!same_type)
            return 0;

        /* check duplicates */
        for (j = 0; j < i; j++)
            if (indices[i] == indices[j])
                return 0;

        previous_card = card;
    }
    return 1;
}

/* If shifting noticably impacts, combine follow-ups into one play. */
int act_play_card(struct game_state *game, const size_t *indices, int len)
{
    struct player *player = game->players + game->active_player_index;
    struct card *last_card;
    int i;

    if (!is_legal_play(game, indices, len))
        return -1;

    for (i = 0; i < len; i++) {
        last_card = (struct card *)player->hand.elements + indices[i];
        play_card_effect(game, last_card, RED);
    }

    game->top_card = *last_card;
    game->acted = PLAY;
    remove_cards(player, indices, len);
    return 0;
}

int end_turn(struct game_state *game)
{
    int increment;

    if (game->ended || !game->acted)
        return -1;

    if (game->players[game->active_player_index].hand.len == 0) {
        game->ended = 1;
        return 0;
    }

    increment = (1 + game->skip_pool) * game->turn_dir;
    game->skip_pool = 0;

    game->active_player_index += increment;
    if (game->active_player_index >= game->player_len)
        game->active_player_index -= game->player_len;
    if (game->active_player_index < 0)
        game->active_player_index += game->player_len;

    game->acted = 0;
    return 0;
}


int supersmartAI_act(struct game_state *game)
{
    struct player *player = game->players + game->active_player_index;
    size_t i, cards_drawn;
    size_t *indices, indices_len = 0;

    if (game->acted) 
        return 0;

    indices = malloc( sizeof(int) * player->hand.len );
    if (indices == NULL)
        return -1;

    for (i = 0; i < player->hand.len; i++) {
        indices[indices_len] = i;
        if (is_legal_play(game, indices, indices_len + 1)) {
            indices_len++;
            i = 0;
        }
    } 
    if (indices_len != 0 && act_play_card(game, indices, indices_len) == 0) {
        free(indices); 
        return 0;
    }
    else {
        free(indices);
        return act_draw_cards(game, &cards_drawn) == 0 ? 0 : -2;
    }
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

void log_card(const struct card *card)
{
    const char *card_type = card_type_to_str(card->type);
    const char *card_color = card_color_to_str(card->color);
    if (card->type == NUMBER)
        LOGF("card [%s] (%s) (%i)", card_type, card_color, card->num);
    else 
        LOGF("card [%s] (%s)", card_type, card_color);
}
void log_hand(const struct player *player)
{
    int i;
    
    for (i = 0; i < player->hand.len; i++) {
        log_card(player->hand.elements + i);
    }
    LOGF("Total of %lu cards", player->hand.len);
}
void log_game_state(const struct game_state *game)
{
    int i;
    LOGF(" - ended: %i", game->ended);
    LOGF(" - acted: %i", game->acted);
    LOGF(" - turn_dir: %i", game->turn_dir);
    LOGF(" - skip_pool: %i", game->skip_pool);
    LOGF(" - batsu_pool: %i", game->batsu_pool);
    LOGF(" - active_player_index: %i", game->active_player_index);
    LOG(" - top_card:");
    log_card(&game->top_card);
    for (i = 0; i < game->player_len; i++) {
        LOGF("players[%i]:", i);
        log_hand(game->players + i);
    }
}
