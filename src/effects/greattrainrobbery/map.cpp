#include "map.h"

#include "game/game.h"
#include "effects/base/pick.h"
#include "effects/base/resolve.h"
#include "cards/game_enums.h"

namespace banggame {

    struct request_map : selection_picker, interface_resolvable {
        request_map(card *origin_card, player *target)
            : selection_picker(origin_card, nullptr, target) {}
        
        void on_update() override {
            if (!live) {
                for (int i=0; i<2; ++i) {
                    target->m_game->move_card(target->m_game->top_of_deck(), pocket_type::selection, target);
                }
            }
        }

        bool move_card_to_deck() const {
            return target->m_game->check_flags(game_flags::phase_one_override)
                || target->m_game->check_flags(game_flags::phase_one_draw_discard) && !target->m_game->m_discards.empty();
        }

        void on_resolve() override {
            target->m_game->pop_request();
            if (move_card_to_deck()) {
                while (!target->m_game->m_selection.empty()) {
                    target->m_game->move_card(target->m_game->m_selection.front(), pocket_type::main_deck, nullptr, card_visibility::hidden);
                }
            }
        }

        void on_pick(card *target_card) override {
            target->m_game->pop_request();
            if (move_card_to_deck()) {
                target->m_game->move_card(target_card, pocket_type::main_deck, nullptr, card_visibility::hidden);
            }
            while (auto not_target = target->m_game->m_selection | rv::filter([&](card *selection_card) {
                return selection_card != target_card;
            })) {
                card *discarded = *not_target.begin();
                target->m_game->add_log("LOG_DISCARDED_CARD_FOR", origin_card, target, discarded);
                target->m_game->move_card(discarded, pocket_type::discard_pile);
            }
        }

        game_string status_text(player *owner) const override {
            if (owner == target) {
                return {"STATUS_MAP", origin_card};
            } else {
                return {"STATUS_MAP_OTHER", target, origin_card};
            }
        }
    };

    void equip_map::on_enable(card *origin_card, player *origin) {
        origin->m_game->add_listener<event_type::on_turn_start>({origin_card, -2}, [=](player *target) {
            if (origin == target) {
                origin->m_game->queue_request<request_map>(origin_card, origin);
            }
        });
    }
}