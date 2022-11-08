#include "discard_all.h"

#include "game/game.h"

namespace banggame {
    
    static bool is_non_black(card *c) {
        return c->color != card_color_type::black;
    }
    
    static card *get_first_discarded_card(player *target) {
        auto non_black_cards = target->m_table | std::views::filter(is_non_black);

        if (!non_black_cards.empty()) {
            return non_black_cards.front();
        } else if (!target->m_hand.empty()) {
            return target->m_hand.front();
        } else {
            return nullptr;
        }
    }

    static card *get_only_discarded_card(player *target) {
        auto non_black_cards = target->m_table | std::views::filter(is_non_black);
        if (std::ranges::distance(non_black_cards) + target->m_hand.size() == 1) {
            if (target->m_hand.empty()) {
                return non_black_cards.front();
            } else {
                return target->m_hand.front();
            }
        } else {
            return nullptr;
        }
    }

    static void discard_rest(player *target, bool death) {
        std::vector<card *> black_cards;
        for (card *c : target->m_table) {
            if (c->color == card_color_type::black) {
                black_cards.push_back(c);
            }
        }
        for (card *c : black_cards) {
            target->m_game->add_log("LOG_DISCARDED_SELF_CARD", target, c);
            target->discard_card(c);
        }
        if (death) {
            target->add_gold(-target->m_gold);
        }
        target->drop_all_cubes(target->m_characters.front());
    }

    struct request_discard_all : request_base, resolvable_request {
        bool death;

        request_discard_all(player *target, bool death = true)
            : request_base(nullptr, nullptr, target, effect_flags::auto_respond)
            , death(death) {}
        
        bool can_pick(card *target_card) const override {
            return (target_card->pocket == pocket_type::player_hand || target_card->pocket == pocket_type::player_table)
                && target_card->owner == target
                && target_card->color != card_color_type::black;
        }

        void on_pick(card *target_card) override {
            target->m_game->add_log("LOG_DISCARDED_SELF_CARD", target, target_card);
            target->discard_card(target_card);
            
            if (target->only_black_cards_equipped()) {
                discard_rest(target, death);
                target->m_game->pop_request();
            }
            target->m_game->update_request();
        }

        bool auto_resolve() override {
            if (request_base::auto_resolve()) return true;

            if (card *c = (target->m_game->m_options.auto_discard_all ? get_first_discarded_card : get_only_discarded_card)(target)) {
                on_pick(c);
                return true;
            }
            return false;
        }

        void on_resolve() override {
            target->m_game->pop_request();
            while (card *c = get_first_discarded_card(target)) {
                target->m_game->add_log("LOG_DISCARDED_SELF_CARD", target, c);
                target->discard_card(c);
            }
            discard_rest(target, death);
            target->m_game->update_request();
        }

        game_string status_text(player *owner) const override {
            if (death) {
                if (target == owner) {
                    return "STATUS_DISCARD_ALL";
                } else {
                    return {"STATUS_DISCARD_ALL_OTHER", target};
                }
            } else {
                if (target == owner) {
                    return "STATUS_SHERIFF_KILLED_DEPUTY";
                } else {
                    return {"STATUS_SHERIFF_KILLED_DEPUTY_OTHER", target};
                }
            }
        }
    };

    void queue_request_discard_all(player *target, bool death) {
        target->m_game->queue_request<request_discard_all>(target, death);
    }

}