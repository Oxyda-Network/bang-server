#ifndef __DRAW_CHECK_HANDLER_H__
#define __DRAW_CHECK_HANDLER_H__

#include "player.h"

namespace banggame {

    struct draw_check_handler {
        virtual void restart() = 0;
        virtual bool check(card *drawn_card) const = 0;
        virtual void resolve(card *drawn_card) = 0;
    };
    
    struct request_check : request_base, draw_check_handler {
        request_check(game *m_game, card *origin_card, player *target, draw_check_condition &&condition, draw_check_function &&function)
            : request_base(origin_card, nullptr, target)
            , m_game(m_game)
            , m_condition(std::move(condition))
            , m_function(std::move(function)) {}

        game *m_game;
        draw_check_condition m_condition;
        draw_check_function m_function;

        void on_update() override;

        bool can_pick(card *target_card) const override;

        void on_pick(card *target_card) override;

        game_string status_text(player *owner) const override;

        void start();
        void select(card *drawn_card);

        void restart() override;
        bool check(card *drawn_card) const override;
        void resolve(card *drawn_card) override;
    };

}

#endif