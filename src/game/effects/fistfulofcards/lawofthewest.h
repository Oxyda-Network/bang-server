#ifndef __FISTFULOFCARDS_LAWOFTHEWEST_H__
#define __FISTFULOFCARDS_LAWOFTHEWEST_H__

#include "../card_effect.h"

namespace banggame {

    struct effect_lawofthewest : event_based_effect {
        void on_enable(card *target_card, player *target);
    };
}

#endif