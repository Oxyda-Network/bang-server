#ifndef __BASE_REQUESTS_H__
#define __BASE_REQUESTS_H__

#include "../card_effect.h"

namespace banggame {

    struct request_characterchoice : request_base {
        request_characterchoice(player *target)
            : request_base(nullptr, nullptr, target) {}
        
        bool can_pick(pocket_type pocket, player *target, card *target_card) const override;
        void on_pick(pocket_type pocket, player *target, card *target_card) override;
        game_string status_text(player *owner) const override;
    };
    
    struct request_discard : request_base {
        request_discard(card *origin_card, player *origin, player *target)
            : request_base(origin_card, origin, target, effect_flags::auto_pick) {}

        int ncards = 1;
        
        bool can_pick(pocket_type pocket, player *target, card *target_card) const override;
        void on_pick(pocket_type pocket, player *target, card *target_card) override;
        game_string status_text(player *owner) const override;
    };

    struct request_discard_pass : request_base {
        request_discard_pass(player *target)
            : request_base(nullptr, nullptr, target) {}

        int ndiscarded = 0;

        bool can_pick(pocket_type pocket, player *target, card *target_card) const override;
        void on_pick(pocket_type pocket, player *target, card *target_card) override;
        game_string status_text(player *owner) const override;
    };

    struct request_discard_all : request_base, resolvable_request {
        request_discard_all(player *target)
            : request_base(nullptr, nullptr, target) {}
        
        bool can_pick(pocket_type pocket, player *target_player, card *target_card) const override;
        void on_pick(pocket_type pocket, player *target_player, card *target_card) override;

        static void auto_resolve(player *target);
        void on_resolve() override;
        game_string status_text(player *owner) const override;
    };

    struct request_sheriff_killed_deputy : request_discard_all {
        using request_discard_all::request_discard_all;
        
        game_string status_text(player *owner) const override;
    };

    struct request_force_play_card : request_base {
        request_force_play_card(card *origin_card, player *target, card *target_card)
            : request_base(origin_card, nullptr, target, effect_flags::force_play | effect_flags::auto_respond)
            , target_card(target_card) {}
        
        card *target_card;

        bool can_respond(player *target, card *target_card) const override;

        game_string status_text(player *owner) const override;
    };

}

#endif