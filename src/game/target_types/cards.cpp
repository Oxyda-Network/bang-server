#include "game/play_verify.h"

#include "cards/filter_enums.h"

namespace banggame {

    using visit_cards = play_visitor<target_type::cards>;

    template<> game_string visit_cards::get_error(const effect_context &ctx, const serial::card_list &targets) {
        if (targets.size() != std::max<size_t>(1, effect.target_value)) {
            return "ERROR_INVALID_TARGETS";
        }
        for (card *c : targets) {
            MAYBE_RETURN(play_visitor<target_type::card>{origin, origin_card, effect}.get_error(ctx, c));
        }
        return {};
    }

    template<> duplicate_set visit_cards::duplicates(const serial::card_list &targets) {
        if (!bool(effect.card_filter & target_card_filter::can_repeat)) {
            return {.cards{ targets | ranges::to<std::vector<card *>> }};
        } else {
            return {};
        }
    }

    template<> game_string visit_cards::prompt(const effect_context &ctx, const serial::card_list &targets) {
        for (card *c : targets) {
            MAYBE_RETURN(play_visitor<target_type::card>{origin, origin_card, effect}.prompt(ctx, c));
        }
        return {};
    }

    template<> void visit_cards::play(const effect_context &ctx, const serial::card_list &targets) {
        for (card *c : targets) {
            play_visitor<target_type::card>{origin, origin_card, effect}.play(ctx, c);
        }
    }

}