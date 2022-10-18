#include "predraw_check.h"

#include "../../game.h"

namespace banggame {

    struct request_predraw : request_base {
        request_predraw(player *target)
            : request_base(nullptr, nullptr, target, effect_flags::auto_pick) {}
        
        bool can_pick(pocket_type pocket, player *target_player, card *target_card) const override {
            if (pocket == pocket_type::player_table && target == target_player) {
                int top_priority = std::ranges::max(target->m_predraw_checks
                    | std::views::values
                    | std::views::filter(std::not_fn(&player::predraw_check::resolved))
                    | std::views::transform(&player::predraw_check::priority));
                auto it = target->m_predraw_checks.find(target_card);
                if (it != target->m_predraw_checks.end()
                    && !it->second.resolved
                    && it->second.priority == top_priority) {
                    return true;
                }
            }
            return false;
        }

        void on_pick(pocket_type pocket, player *target, card *target_card) override {
            target->m_game->pop_request();
            target->m_game->call_event<event_type::on_predraw_check>(target, target_card);
            target->m_game->queue_action([target = target, target_card] {
                auto it = target->m_predraw_checks.find(target_card);
                if (it != target->m_predraw_checks.end()) {
                    it->second.resolved = true;
                }
                target->next_predraw_check();
            });
            target->m_game->update_request();
        }

        game_string status_text(player *owner) const override {
            if (owner == target) {
                return "STATUS_PREDRAW";
            } else {
                return {"STATUS_PREDRAW_OTHER", target};
            }
        }
    };

    void effect_predraw_check::on_enable(card *target_card, player *target) {
        target->m_predraw_checks.try_emplace(target_card, priority, false);
    }

    void effect_predraw_check::on_disable(card *target_card, player *target) {
        target->m_predraw_checks.erase(target_card);
    }

    void effect_predraw_check::queue(player *target) {
        target->m_game->queue_request<request_predraw>(target);
    }
}