#include <stdlib.h>
#include "system/logging.h"

#define IS_PICKABLE_COLOR(color) ( 0 <= (color) && (color) <= 3 )
struct card {
    enum card_type {
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
struct player {
    short        id;
    char        *name;
    struct card *hand;
    size_t       hand_len;
};
struct game_state {
    int             ended;
    struct player  *players;
    int             player_len;
    int             active_player_index;
    enum action {
        NONE,
        PLAY,
        DRAW
    } acted;

    struct card     top_card;
    int             turn_dir;
    int             skip_next;
    int             batsu;
};

void rand_card(struct card *card)
{
    const float RATIO = RAND_MAX / 110;
    int random;

    random = rand();
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
void rand_cards(struct card *cards, int amount)
{
    int i;
    for (i = 0; i < amount; i++) {
        rand_card(cards + i);
    }
}

int add_cards(struct game_state *game, int player_index, int amount)
{
    struct player *player = game->players + player_index;
    int            new_len = player->hand_len + amount;

    player->hand = realloc(player->hand, new_len * sizeof(struct card));
    if (player->hand == NULL)
        return -1;
    rand_cards(player->hand + player->hand_len, amount);
    player->hand_len = new_len;
    return 0;
} /*  曲: 帰る日 & ふたたび - 千と千尋の神隠し */

void remove_cards(struct game_state *game, int player_index, int *hand_indexes_asc, int indexes_len)
{
    struct player *player = game->players + player_index;
    int i, skip = 1;

    for (i = hand_indexes_asc[0]; (i + skip) < player->hand_len; i++) {
        if (skip < indexes_len && (i + skip) == hand_indexes_asc[skip])
            skip++;
        player->hand[i] = player->hand[i + skip];
    }
    player->hand_len -= indexes_len;
}

int is_legal_play(const struct game_state *game, struct card *card)
{
    int same_color = card->color == game->top_card.color;
    int same_type  = card->type  == game->top_card.type;

    /* Card specific */
    if (card->type == NUMBER && same_type)
        same_type = game->top_card.num == card->num;

    if (game->acted == PLAY)
        return same_type;
    if (game->acted)
        return 0;
    if (game->batsu)
        return same_type || card->type == PLUS4;

    if (!same_color && !same_type && card->color != BLACK)
        return 0;


    return 1;
}

int play_card_effect(struct game_state *game, struct card *card, enum card_color arg_color)
{
    switch (card->type) {
        case REVERSE:
            game->turn_dir *= -1;
            break;

        case SKIP:
            game->skip_next = 1;
            break;

        case PLUS2:
            game->batsu += 2;
            break;

        case PLUS4:
            game->batsu += 4;
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
    game->skip_next     = 0;
    game->batsu         = 0;
    srand(1);
    do 
        rand_card(&game->top_card); 
    while (game->top_card.type == PLUS4 || game->top_card.type == PICK_COLOR);
    play_card_effect(game, &game->top_card, -2);
}

void deal_cards(struct game_state *game)
{
    const int deal_per_player = 4;
    int i;

    for (i = 0; i < game->player_len; i++) {
        game->players[i].id = i;
        game->players[i].hand = malloc( deal_per_player * sizeof(struct card) );
        rand_cards(game->players[i].hand, deal_per_player);
        game->players[i].hand_len = deal_per_player;
    }
}

/* ACTS */
int act_take_batsu(struct game_state *game)
{
    if (game->ended || !game->batsu || game->acted)
        return -1;

    add_cards(game, game->active_player_index, game->batsu);
    game->acted = DRAW;
    game->batsu = 0;
    return 0;
}

int act_draw_card(struct game_state *game)
{
    if (game->ended || game->batsu || game->acted)
        return -1;

    add_cards(game, game->active_player_index, 1);
    game->acted = DRAW;

    return 0;
}

int act_play_card(struct game_state *game, int hand_index, enum card_color arg_color)
{
    struct card *card = game->players[game->active_player_index].hand + hand_index;

    if (game->ended || !is_legal_play(game, card))
        return -1;

    if (play_card_effect(game, card, arg_color) != 0)
        return -3;
    game->top_card = *card;

    game->acted = PLAY;
    remove_cards(game, game->active_player_index, &hand_index, 1);
    return 0;
}

int end_turn(struct game_state *game)
{
    int increment;

    if (game->ended || !game->acted)
        return -1;

    if (game->players[game->active_player_index].hand_len == 0) {
        game->ended = 1;
        return 0;
    }

    increment = game->turn_dir;
    if (game->skip_next) {
        increment *= 2;
        game->skip_next = 0;
    }

    game->active_player_index += increment;
    if (game->active_player_index >= game->player_len)
        game->active_player_index -= game->player_len;
    if (game->active_player_index < 0)
        game->active_player_index += game->player_len;

    game->acted = NONE;
    return 0;
}

int supersmartAI_act(struct game_state *game)
{
    struct player *player = game->players + game->active_player_index;
    int i, total_card_expended;

    for (i = 0; i < player->hand_len; i++) {
        if (!is_legal_play(game, player->hand + i))
            continue;

        if (act_play_card(game, i, RED) != 0)
            return -1;
    }

    if (game->acted) 
        return 0;

    if (game->batsu)
        return act_take_batsu(game) == 0 ? 2 : -3;
    else
        return act_draw_card(game) == 0 ? 1 : -4;

    return -5;
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

const char *action_to_str(enum action action)
{
    switch (action) {
        case NONE:
            return "none";
        case DRAW:
            return "draw";
        case PLAY:
            return "play";
        default:
            return "Undefined action";
    };
}

void log_card(struct card *card)
{
    const char *card_type = card_type_to_str(card->type);
    const char *card_color = card_color_to_str(card->color);
    if (card->type == NUMBER)
        LOGF("card [%s] (%s) (%i)", card_type, card_color, card->num);
    else 
        LOGF("card [%s] (%s)", card_type, card_color);
}
void log_hand(struct player *player)
{
    int i;
    
    for (i = 0; i < player->hand_len; i++) {
        log_card(player->hand + i);
    }
    LOGF("Total of %lu cards", player->hand_len);
}
void log_game_state(struct game_state *game)
{
    int i;
    LOGF(" - ended: %i", game->ended);
    LOGF(" - acted: %s", action_to_str(game->acted));
    LOGF(" - turn_dir: %i", game->turn_dir);
    LOGF(" - skip_next: %i", game->skip_next);
    LOGF(" - batsu: %i", game->batsu);
    LOGF(" - active_player_index: %i", game->active_player_index);
    LOG(" - top_card:");
    log_card(&game->top_card);
    for (i = 0; i < game->player_len; i++) {
        LOGF("players[%i]:", i);
        log_hand(game->players + i);
    }
}
