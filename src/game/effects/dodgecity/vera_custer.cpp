#include "vera_custer.h"

#include "../../game.h"

namespace banggame {

    static void remove_characters(player *origin) {
        if (origin->m_characters.size() > 1) {
            origin->remove_extra_characters();
            
            while (origin->m_characters.size() > 1) {
                origin->disable_equip(origin->m_characters.back());
                origin->m_game->m_cards.erase(origin->m_characters.back()->id);
                origin->m_characters.pop_back();
            }
        }
    }

    static void copy_characters(player *origin, player *target) {
        remove_characters(origin);

        std::ranges::for_each(
            target->m_characters
                | std::views::reverse
                | std::views::take(2)
                | std::views::reverse,
            [origin](card *target_card) {
                origin->m_game->add_log("LOG_COPY_CHARACTER", origin, target_card);
                
                auto card_copy = std::make_unique<card>(static_cast<const card_data &>(*target_card));
                card_copy->id = static_cast<int>(origin->m_game->m_cards.first_available_id());
                card_copy->pocket = pocket_type::player_character;
                card_copy->owner = origin;

                card *new_card = origin->m_game->m_cards.insert(std::move(card_copy)).get();
                
                origin->m_characters.emplace_back(new_card);
                new_card->on_enable(origin);

                origin->m_game->add_update<game_update_type::add_cards>(
                    make_id_vector(std::views::single(new_card)), pocket_type::player_character, origin);
                origin->m_game->send_card_update(new_card, origin, show_card_flags::instant | show_card_flags::shown);
            });
    }

    struct request_vera_custer : request_base {
        request_vera_custer(card *origin_card, player *target)
            : request_base(origin_card, nullptr, target) {}
        
        bool can_pick(pocket_type pocket, player *target_player, card *target_card) const override {
            return pocket == pocket_type::player_character
                && target_player->alive() && target_player != target;
        }

        void on_pick(pocket_type pocket, player *target_player, card *target_card) override {
            target->m_game->pop_request();
            copy_characters(target, target_player);
            target->m_game->update_request();
        }

        game_string status_text(player *owner) const override {
            if (owner == target) {
                return {"STATUS_VERA_CUSTER", origin_card};
            } else {
                return {"STATUS_VERA_CUSTER_OTHER", target, origin_card};
            }
        }
    };

    void effect_vera_custer::on_enable(card *origin_card, player *origin) {
        origin->m_game->add_listener<event_type::on_turn_start>({origin_card, 1}, [=](player *target) {
            if (origin == target) {
                if (origin->m_game->num_alive() == 2) {
                    copy_characters(origin, std::next(player_iterator(origin)));
                } else {
                    origin->m_game->queue_request<request_vera_custer>(origin_card, target);
                }
            }
        });
        origin->m_game->add_listener<event_type::on_turn_end>(origin_card, [=](player *target, bool skipped) {
            if (skipped && origin == target && origin->m_num_drawn_cards == 0) {
                remove_characters(origin);
            }
        });
    }

}