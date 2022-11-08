#include "sacrifice.h"

#include "game/game.h"
#include "cards/base/damage.h"

namespace banggame {

    bool effect_sacrifice::can_respond(card *origin_card, player *origin) {
        if (auto *req = origin->m_game->top_request_if<timer_damaging>()) {
            return req->target != origin;
        }
        return false;
    }

    void effect_sacrifice::on_play(card *origin_card, player *origin) {
        auto &req = origin->m_game->top_request().get<timer_damaging>();
        player *saved = req.target;
        bool fatal = saved->m_hp <= req.damage;
        if (0 == --req.damage) {
            origin->m_game->pop_request();
        }
        origin->damage(origin_card, origin, 1);
        origin->m_game->queue_action_front([=]{
            if (origin->alive()) {
                origin->draw_card(2 + fatal, origin_card);
            }
        });
        origin->m_game->update_request();
    }
}