#include "thirst.h"

#include "../../game.h"

namespace banggame {

    void effect_thirst::on_enable(card *target_card, player *target) {
        target->m_game->add_listener<event_type::count_cards_to_draw>(target_card, [](player *origin, int &value) {
            if (value > 1) {
                value = 1;
            }
        });
    }
}