#ifndef __GREATTRAINROBBERY_CACTUS_H__
#define __GREATTRAINROBBERY_CACTUS_H__

#include "cards/card_effect.h"

namespace banggame {

    struct effect_cactus {
        void on_play(card *origin_card, player *origin);
    };
}

#endif