#include "don_bell.h"

#include "../../game.h"

namespace banggame {

    void effect_don_bell::on_enable(card *target_card, player *p) {
        p->m_game->add_listener<event_type::on_turn_end>({target_card, -2}, [=](player *target, bool skipped) {
            if (!skipped && p == target && !target->check_player_flags(player_flags::extra_turn)) {
                p->m_game->queue_action([target, target_card] {
                    target->m_game->draw_check_then(target, target_card, [target, target_card](card *drawn_card) {
                        card_suit suit = target->get_card_sign(drawn_card).suit;
                        if (suit == card_suit::diamonds || suit == card_suit::hearts) {
                            target->m_game->add_log("LOG_CARD_HAS_EFFECT", target_card);
                            ++target->m_extra_turns;
                        }
                    });
                });
            }
        });
    }
}