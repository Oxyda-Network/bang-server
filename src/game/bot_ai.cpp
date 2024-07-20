#include "game.h"
#include "play_verify.h"
#include "possible_to_play.h"
#include "net/bot_info.h"

#include "game/filters.h"

namespace banggame {

    static game_action generate_random_play(player *origin, const playable_card_info &args, bool is_response) {
        game_action ret { .card = args.card };
        effect_context ctx{};
        
        for (card *mod_card : args.modifiers) {
            auto &targets = ret.modifiers.emplace_back(mod_card).targets;

            mod_card->get_modifier(is_response).add_context(mod_card, origin, ctx);
            for (const effect_holder &holder : mod_card->get_effect_list(is_response)) {
                const auto &target = targets.emplace_back(play_dispatch::random_target(origin, mod_card, holder, ctx));
                play_dispatch::add_context(origin, mod_card, holder, ctx, target);
            }
        }

        if (filters::is_equip_card(args.card)) {
            if (!args.card->self_equippable()) {
                ret.targets.emplace_back(utils::tag<"player">{},
                    random_element(make_equip_set(origin, args.card, ctx), origin->m_game->bot_rng));
            }
        } else {
            for (const effect_holder &holder : args.card->get_effect_list(is_response)) {
                const auto &target = ret.targets.emplace_back(play_dispatch::random_target(origin, args.card, holder, ctx));
                play_dispatch::add_context(origin, args.card, holder, ctx, target);
            }
        }

        return ret;
    }

    bot_rule rule_filter_by_pocket(pocket_type pocket) {
        return [=](card_node node) {
            if (card *choice_card = node->context.get().card_choice) {
                return choice_card->pocket == pocket;
            }
            return node->card->pocket == pocket;
        };
    }

    bot_rule rule_repeat() {
        return [](card_node node) {
            return node->context.get().repeat_card != nullptr;
        };
    }
    
    static card_node get_selected_node(player *origin, bool is_response, const std::set<card_node> &node_set) {
        auto &rules = is_response ? bot_info.settings.response_rules : bot_info.settings.in_play_rules;
        for (const bot_rule &rule : rules) {
            if (auto filter = rv::filter(node_set, rule)) {
                return random_element(filter, origin->m_game->bot_rng);
            }
        }
        return random_element(node_set, origin->m_game->bot_rng);
    }

    static bool execute_random_play(player *origin, bool is_response, std::optional<timer_id_t> timer_id, const playable_cards_list &play_cards) {
        for (int i=0; i < bot_info.settings.max_random_tries; ++i) {
            std::set<card_node> node_set = play_cards | rv::addressof | rn::to<std::set>;
            
            while (!node_set.empty()) {
                auto selected_node = get_selected_node(origin, is_response, node_set);
                node_set.erase(selected_node);

                // maybe add random variation?
                bool bypass_prompt = node_set.empty() && i >= bot_info.settings.bypass_prompt_after;
                try {
                    auto args = generate_random_play(origin, *selected_node, is_response);
                    args.bypass_prompt = bypass_prompt;
                    args.timer_id = timer_id;
                    if (verify_and_play(origin, args).type == message_type::ok) {
                        return true;
                    }
                } catch (const random_element_error &) {
                    // ignore
                }
            }
        }

        // softlock
        std::cerr << "BOT ERROR: could not find card in execute_random_play()\n";

        return false;
    }

    bool game::request_bot_play() {
        if (pending_requests()) {
            for (player *origin : m_players | rv::filter(&player::is_bot)) {
                playable_cards_list play_cards = generate_playable_cards_list(origin, true);
                
                if (!play_cards.empty()) {
                    std::optional<timer_id_t> timer_id;
                    if (auto *timer = origin->m_game->top_request()->timer()) {
                        timer_id = timer->get_timer_id();
                    }

                    if (execute_random_play(origin, true, timer_id, play_cards)) {
                        return true;
                    }
                }
            }
        } else if (m_playing && m_playing->is_bot()) {
            playable_cards_list play_cards = generate_playable_cards_list(m_playing);
            return execute_random_play(m_playing, false, std::nullopt, play_cards);
        }
        return false;
    }

    
}