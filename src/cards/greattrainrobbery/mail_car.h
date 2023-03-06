#ifndef __GREATTRAINROBBERY_MAIL_CAR_H__
#define __GREATTRAINROBBERY_MAIL_CAR_H__

#include "cards/card_effect.h"

#include "game/bot_suggestion.h"

namespace banggame {

    struct effect_mail_car {
        void on_play(card *origin_card, player *origin);
    };

    struct handler_mail_car {
        bool on_check_target(card *origin_card, player *origin, card *target_card, player *target) {
            return bot_suggestion::target_friend{}.on_check_target(origin_card, origin, target);
        }
        game_string get_error(card *origin_card, player *origin, card *target_card, player *target);
        void on_play(card *origin_card, player *origin, card *target_card, player *target);  
    };
}

#endif