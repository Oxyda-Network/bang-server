#include "sacrifice.h"

#include "game/game.h"
#include "cards/base/damage.h"

namespace banggame {

    bool effect_sacrifice::can_respond(card *origin_card, player *origin) {
        if (auto *req = origin->m_game->top_request_if<request_damage>()) {
            return req->target != origin;
        }
        return false;
    }

    void effect_sacrifice::on_play(card *origin_card, player *origin) {
        auto &req = origin->m_game->top_request().get<request_damage>();
        player *saved = req.target;
        bool fatal = saved->m_hp <= req.damage;

        auto lock = origin->m_game->lock_updates(--req.damage == 0);
        
        origin->damage(origin_card, origin, 1);
        origin->m_game->queue_action([=]{
            if (origin->alive()) {
                origin->draw_card(2 + fatal, origin_card);
            }
        }, 1);
    }
}