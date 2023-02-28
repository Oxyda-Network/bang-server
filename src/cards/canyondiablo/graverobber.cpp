#include "graverobber.h"

#include "game/game.h"

namespace banggame {

    void effect_graverobber::on_play(card *origin_card, player *origin, player *target) {
        if (origin->m_game->m_discards.empty()) {
            origin->m_game->move_card(origin->m_game->top_of_deck(), pocket_type::selection);
        } else {
            origin->m_game->move_card(origin->m_game->m_discards.back(), pocket_type::selection);
        }
    }
}