#ifndef __WILDWESTSHOW_BIG_SPENCER__
#define __WILDWESTSHOW_BIG_SPENCER__

#include "../card_effect.h"

namespace banggame {

    struct effect_big_spencer {
        void on_enable(card *target_card, player *target);
        void on_disable(card *target_card, player *target);
    };
}

#endif