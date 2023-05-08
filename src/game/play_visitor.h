#ifndef __PLAY_VISITOR_H__
#define __PLAY_VISITOR_H__

#include "player.h"
#include "game.h"

namespace banggame {

    struct card_cube_ordering {
        bool operator()(card *lhs, card *rhs) const {
            if (lhs->pocket == pocket_type::player_table && rhs->pocket == pocket_type::player_table) {
                return std::ranges::find(lhs->owner->m_table, lhs) < std::ranges::find(rhs->owner->m_table, rhs);
            } else {
                return lhs->pocket == pocket_type::player_table;
            }
        }
    };

    using card_cube_count = std::map<card *, int, card_cube_ordering>;

    struct duplicate_set {
        std::vector<player *> players;
        std::vector<card *> cards;
        card_cube_count cubes;
    };

    template<target_type E> struct play_visitor {
        player *origin;
        card *origin_card;
        const effect_holder &effect;

        game_string get_error(const effect_context &ctx);
        duplicate_set duplicates();
        game_string prompt(const effect_context &ctx);
        void play(const effect_context &ctx);
    };

    template<target_type E> requires (play_card_target::has_type<E>)
    struct play_visitor<E> {
        using arg_type = same_if_trivial_t<unwrap_not_null_t<typename play_card_target::value_type<E>>>;

        player *origin;
        card *origin_card;
        const effect_holder &effect;

        game_string get_error(const effect_context &ctx, arg_type arg);
        duplicate_set duplicates(arg_type arg);
        game_string prompt(const effect_context &ctx, arg_type arg);
        void play(const effect_context &ctx, arg_type arg);
    };

}

#endif