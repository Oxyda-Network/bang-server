#include "suzy_lafayette.h"

#include "game/game.h"

namespace banggame {

    void equip_suzy_lafayette::on_enable(card *origin_card, player *origin) {
        origin->m_game->add_listener<event_type::on_effect_end>(origin_card, [origin, origin_card](player *, card *) {
            origin->m_game->queue_action([origin, origin_card]{
                if (origin->alive() && origin->empty_hand()) {
                    origin->m_game->flash_card(origin_card);
                    origin->draw_card(1, origin_card);
                }
            });
        });
    }
}