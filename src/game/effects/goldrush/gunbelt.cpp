#include "gunbelt.h"

#include "../../game.h"

namespace banggame {

    void effect_gunbelt::on_enable(card *target_card, player *target) {
        target->m_game->add_listener<event_type::apply_maxcards_modifier>({target_card, 20 - ncards}, [=, ncards=ncards](player *p, int &value) {
            if (p == target) {
                value = ncards;
            }
        });
    }
}