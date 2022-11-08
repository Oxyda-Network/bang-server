#include "draw.h"

#include "game/game.h"

namespace banggame {

    void effect_draw::on_play(card *origin_card, player *origin, player *target) {
        target->draw_card(ncards, origin_card);
    }

    void handler_draw_multi::on_play(card *origin_card, player *origin, int amount) {
        if (amount > 0) {
            effect_draw{amount}.on_play(origin_card, origin);
        }
    }

    game_string effect_draw_discard::verify(card *origin_card, player *origin, player *target) {
        if (target->m_game->m_discards.empty()) {
            return "ERROR_DISCARD_PILE_EMPTY";
        }
        return {};
    }
    
    void effect_draw_discard::on_play(card *origin_card, player *origin, player *target) {
        card *drawn_card = target->m_game->m_discards.back();
        target->m_game->add_log("LOG_DRAWN_FROM_DISCARD", target, drawn_card);
        target->add_to_hand(drawn_card);
    }

    void effect_draw_to_discard::on_play(card *origin_card, player *origin) {
        for (int i=0; i<ncards; ++i) {
            origin->m_game->draw_card_to(pocket_type::discard_pile);
        }
    }

    void effect_draw_one_less::on_play(card *origin_card, player *target) {
        target->m_game->queue_action([=]{
            if (target->alive()) {
                ++target->m_num_drawn_cards;
                int ncards = target->get_cards_to_draw();
                while (target->m_num_drawn_cards < ncards) {
                    target->add_to_hand_phase_one(target->m_game->phase_one_drawn_card());
                }
            }
        });
    }
    
    bool request_draw::can_pick(card *target_card) const {
        return target_card->pocket == target->m_game->phase_one_drawn_card()->pocket;
    }

    void request_draw::on_pick(card *target_card) {
        target->draw_from_deck();
    }

    game_string request_draw::status_text(player *owner) const {
        if (owner == target) {
            return "STATUS_YOUR_TURN";
        } else {
            return {"STATUS_YOUR_TURN_OTHER", target};
        }
    }
    
    bool effect_while_drawing::can_respond(card *origin_card, player *origin) {
        return origin->m_game->top_request_is<request_draw>(origin);
    }

    void effect_end_drawing::on_play(card *origin_card, player *origin) {
        if (origin->m_game->top_request_is<request_draw>()) {
            origin->m_game->pop_request();
            origin->m_game->add_listener<event_type::on_effect_end>(origin_card, [=](player *p, card *c) {
                if (p == origin && c == origin_card) {
                    origin->m_game->queue_action([=]{
                        origin->m_game->call_event<event_type::post_draw_cards>(origin);
                    });
                    origin->m_game->remove_listeners(origin_card);
                }
            });
            origin->m_game->update_request();
        }
    }
}