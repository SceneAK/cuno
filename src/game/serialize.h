#include "engine/system/network.h"
#include "engine/alias.h"
#include "logic.h"

static inline void card_serialize(u8 **cursor, const struct card *card)
{
    network_serialize_u16(cursor, card->id);
    network_serialize_u16(cursor, card->num);
    network_serialize_u8(cursor, card->color);
    network_serialize_u8(cursor, card->type);
}

static inline void card_deserialize(struct card *card, u8 **cursor)
{
    card->id    = network_deserialize_u16(cursor);
    card->num   = network_deserialize_u16(cursor);
    card->color = (s8)network_deserialize_u8(cursor);
    card->type  = (s8)network_deserialize_u8(cursor);
}

static inline void card_list_serialize(u8 **cursor, const struct card_list *cardlist)
{
    int i;
    network_serialize_u16(cursor, cardlist->len);
    for (i = 0; i < cardlist->len; i++)
        card_serialize(cursor, cardlist->elems + i);
}

static inline void card_list_deserialize(struct card_list *cardlist, u8 **cursor)
{
    int i;
    int len;

    len = network_deserialize_u16(cursor);

    if (!cardlist->elems)
        card_list_init(cardlist, len);
    card_list_clear(cardlist);
    card_list_emplace(cardlist, len);

    for (i = 0; i < len; i++)
        card_deserialize(cardlist->elems + i, cursor);
}

static inline void player_serialize(u8 **cursor, const struct player *player)
{
    network_serialize_str(cursor, player->name);
    card_list_serialize(cursor, &player->hand);
}

static inline void player_deserialize(struct player *player, u8 **cursor)
{
    network_deserialize_str(player->name, cursor);
    card_list_deserialize(&player->hand, cursor);
}

static inline void game_state_serialize(u8 **cursor, const struct game_state *state)
{
    int i;

    network_serialize_u8(cursor, state->turn);
    network_serialize_u8(cursor, state->turn_dir);
    network_serialize_u8(cursor, state->ended);
    network_serialize_u8(cursor, state->skip_pool);
    network_serialize_u16(cursor, state->batsu_pool);
    card_serialize(cursor, &state->top_card);

    network_serialize_u8(cursor, state->player_len);
    for (i = 0; i < state->player_len; i++)
        player_serialize(cursor, state->players + i);
    network_serialize_u8(cursor, state->active_player_index);
}

static inline void game_state_deserialize(struct game_state *state, u8 **cursor)
{
    int i;

    state->turn       = network_deserialize_u8(cursor);
    state->turn_dir   = (s8)network_deserialize_u8(cursor);
    state->ended      = network_deserialize_u8(cursor);
    state->skip_pool  = network_deserialize_u8(cursor);
    state->batsu_pool = network_deserialize_u16(cursor);
    card_deserialize(&state->top_card, cursor);

    state->player_len = network_deserialize_u8(cursor);
    for (i = 0; i < state->player_len; i++)
        player_deserialize(state->players + i, cursor);
    state->active_player_index = network_deserialize_u8(cursor);
}
