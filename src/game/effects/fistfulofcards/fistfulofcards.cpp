#include "fistfulofcards.h"

#include "../../game.h"
#include "../base/bang.h"

namespace banggame {
    
    void effect_fistfulofcards::on_enable(card *target_card, player *target) {
        target->m_game->add_listener<event_type::pre_turn_start>(target_card, [=](player *p) {
            p->m_game->add_log("LOG_RECEIVED_N_BANGS_FOR", p, target_card, static_cast<int>(p->m_hand.size()));
            for (int i=0; i<p->m_hand.size(); ++i) {
                p->m_game->queue_action([=]{
                    if (p->alive()) {
                        p->m_game->queue_request<request_bang>(target_card, nullptr, p);
                    }
                });
            }
        });
    }
}