#ifndef SERIALIZE_H
#define SERIALIZE_H

#include "engine/system/network_packer.h"
#include "engine/alias.h"
#include "logic.h"

#ifndef __STDC_IEC_559__
#include <float.h>
#include <limits.h>
#include "engine/utils.h"
STATIC_ASSERT(
    FLT_RADIX == 2
    && FLT_MANT_DIG == 24
    && FLT_MAX_EXP == 128
    && sizeof(float)*CHAR_BIT == 32,
    float_not_ieee754
);
#endif

static inline size_t card_serialize(u8 **cursor, const struct card *card)
{
    size_t total = 0;
    total += network_pack_u16(cursor, card->id);
    total += network_pack_u16(cursor, card->num);
    total += network_pack_u8(cursor, card->color);
    total += network_pack_u8(cursor, card->type);
    return total;
}

static inline void card_deserialize(struct card *card, u8 **cursor)
{
    card->id    = network_unpack_u16(cursor);
    card->num   = network_unpack_u16(cursor);
    card->color = (s8)network_unpack_u8(cursor);
    card->type  = (s8)network_unpack_u8(cursor);
}

static inline size_t card_list_serialize(u8 **cursor, const struct card_list *cardlist)
{
    size_t total = 0;
    int i;

    total += network_pack_u16(cursor, cardlist->len);
    for (i = 0; i < cardlist->len; i++)
        total += card_serialize(cursor, cardlist->elems + i);
    return total;
}

static inline void cardlist_deserialize(struct card_list *cardlist, u8 **cursor)
{
    int i;
    int len;

    len = network_unpack_u16(cursor);

    if (!cardlist->elems)
        card_list_init(cardlist, len);
    card_list_clear(cardlist);
    card_list_emplace(cardlist, len);

    for (i = 0; i < len; i++)
        card_deserialize(cardlist->elems + i, cursor);
}

static inline size_t player_serialize(u8 **cursor, const struct player *player)
{
    size_t total = 0;
    total += network_pack_u8(cursor, player->id);
    total += network_pack_str(cursor, player->name);
    total += card_list_serialize(cursor, &player->hand);
    return total;
}

static inline void player_deserialize(struct player *player, u8 **cursor)
{
    player->id = network_unpack_u8(cursor);
    network_unpack_str(player->name, cursor);
    cardlist_deserialize(&player->hand, cursor);
}

static inline size_t game_state_serialize(u8 **cursor, const struct game_state *state)
{
    size_t total = 0;
    int i;

    total += network_pack_u8(cursor, state->turn);
    total += network_pack_u8(cursor, state->turn_dir);
    total += network_pack_u8(cursor, state->ended);
    total += network_pack_u8(cursor, state->skip_pool);
    total += network_pack_u16(cursor, state->batsu_pool);
    card_serialize(cursor, &state->top_card);

    total += network_pack_u8(cursor, state->player_len);
    for (i = 0; i < state->player_len; i++)
        total += player_serialize(cursor, state->players + i);
    total += network_pack_u8(cursor, state->active_player_index);
    return total;
}

static inline void game_state_deserialize(struct game_state *state, u8 **cursor)
{
    int i;
printf("unpacking regular \n");
    state->turn       = network_unpack_u8(cursor);
    state->turn_dir   = (s8)network_unpack_u8(cursor);
    state->ended      = network_unpack_u8(cursor);
    state->skip_pool  = network_unpack_u8(cursor);
    state->batsu_pool = network_unpack_u16(cursor);printf("deser cafd\n");
    card_deserialize(&state->top_card, cursor);
printf("deser player\n");
    state->player_len = network_unpack_u8(cursor);
    for (i = 0; i < state->player_len; i++)
        player_deserialize(state->players + i, cursor);
    state->active_player_index = network_unpack_u8(cursor);
}

#endif
