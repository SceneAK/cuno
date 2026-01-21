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

static inline void card_serialize(u8 **cursor, const struct card *card, size_t *dryrun_res)
{
    network_pack_u16(cursor, card->id, dryrun_res);
    network_pack_u16(cursor, card->num, dryrun_res);
    network_pack_u8(cursor, card->color, dryrun_res);
    network_pack_u8(cursor, card->type, dryrun_res);
}

static inline void card_deserialzie(struct card *card, u8 **cursor)
{
    card->id    = network_unpack_u16(cursor);
    card->num   = network_unpack_u16(cursor);
    card->color = (s8)network_unpack_u8(cursor);
    card->type  = (s8)network_unpack_u8(cursor);
}

static inline void card_list_serialize(u8 **cursor, const struct card_list *cardlist, size_t *dryrun_res)
{
    int i;
    network_pack_u16(cursor, cardlist->len, dryrun_res);
    for (i = 0; i < cardlist->len; i++)
        card_serialize(cursor, cardlist->elems + i, dryrun_res);
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
        card_deserialzie(cardlist->elems + i, cursor);
}

static inline void player_serialize(u8 **cursor, const struct player *player, size_t *dryrun_res)
{
    network_pack_str(cursor, player->name, dryrun_res);
    card_list_serialize(cursor, &player->hand, dryrun_res);
}

static inline void player_deserialize(struct player *player, u8 **cursor)
{
    network_unpack_str(player->name, cursor);
    cardlist_deserialize(&player->hand, cursor);
}

static inline void game_state_serialize(u8 **cursor, const struct game_state *state, size_t *dryrun_res)
{
    int i;

    network_pack_u8(cursor, state->turn, dryrun_res);
    network_pack_u8(cursor, state->turn_dir, dryrun_res);
    network_pack_u8(cursor, state->ended, dryrun_res);
    network_pack_u8(cursor, state->skip_pool, dryrun_res);
    network_pack_u16(cursor, state->batsu_pool, dryrun_res);
    card_serialize(cursor, &state->top_card, dryrun_res);

    network_pack_u8(cursor, state->player_len, dryrun_res);
    for (i = 0; i < state->player_len; i++)
        player_serialize(cursor, state->players + i, dryrun_res);
    network_pack_u8(cursor, state->active_player_index, dryrun_res);
}

static inline void game_state_deserialize(struct game_state *state, u8 **cursor)
{
    int i;

    state->turn       = network_unpack_u8(cursor);
    state->turn_dir   = (s8)network_unpack_u8(cursor);
    state->ended      = network_unpack_u8(cursor);
    state->skip_pool  = network_unpack_u8(cursor);
    state->batsu_pool = network_unpack_u16(cursor);
    card_deserialzie(&state->top_card, cursor);

    state->player_len = network_unpack_u8(cursor);
    for (i = 0; i < state->player_len; i++)
        player_deserialize(state->players + i, cursor);
    state->active_player_index = network_unpack_u8(cursor);
}
