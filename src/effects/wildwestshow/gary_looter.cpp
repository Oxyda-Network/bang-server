#include "gary_looter.h"

#include "game/game.h"

#include "effects/base/vulture_sam.h"
#include "effects/base/requests.h"

namespace banggame {

    static card *get_gary_looter(player *target) {
        card *origin_card = nullptr;
        target->m_game->call_event(event_type::check_card_taker{ target, card_taker_type::discards, origin_card });
        return origin_card;
    }

    void equip_gary_looter::on_enable(card *target_card, player *player_end) {
        player_end->m_game->add_listener<event_type::check_card_taker>(target_card, [=](player *e_target, card_taker_type type, card* &value) {
            if (type == card_taker_type::discards && e_target == player_end) {
                value = target_card;
            }
        });
        player_end->m_game->add_listener<event_type::on_discard_pass>(target_card, [=](player *player_begin, card *discarded_card) {
            if (player_begin != player_end && std::none_of(player_iterator(player_begin), player_iterator(player_end), get_gary_looter)) {
                player_end->m_game->add_log("LOG_DRAWN_CARD", player_end, discarded_card);
                discarded_card->add_short_pause();
                player_end->add_to_hand(discarded_card);
            }
        });
    }
}