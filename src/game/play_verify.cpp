#include "play_verify.h"

#include "play_visitor.h"

#include "effects/base/requests.h"
#include "effect_list_zip.h"

#include "utils/raii_editor.h"
#include "utils/utils.h"

namespace banggame {
    using namespace enums::flag_operators;

    game_string check_player_filter(card *origin_card, player *origin, target_player_filter filter, player *target) {
        if (bool(filter & target_player_filter::dead)) {
            if (target->m_hp > 0) return "ERROR_TARGET_NOT_DEAD";
        } else if (!target->check_player_flags(player_flags::targetable) && !target->alive()) {
            return "ERROR_TARGET_DEAD";
        }

        if (bool(filter & target_player_filter::self) && target != origin)
            return "ERROR_TARGET_NOT_SELF";

        if (bool(filter & target_player_filter::notself) && target == origin)
            return "ERROR_TARGET_SELF";

        if (bool(filter & target_player_filter::notsheriff) && target->m_role == player_role::sheriff)
            return "ERROR_TARGET_SHERIFF";

        if (bool(filter & (target_player_filter::reachable | target_player_filter::range_1 | target_player_filter::range_2))) {
            int distance = origin->m_range_mod;
            if (bool(filter & target_player_filter::reachable)) {
                distance += origin->m_weapon_range;
            } else if (bool(filter & target_player_filter::range_1)) {
                ++distance;
            } else if (bool(filter & target_player_filter::range_2)) {
                distance += 2;
            }
            if (origin->m_game->calc_distance(origin, target) > distance) {
                return "ERROR_TARGET_NOT_IN_RANGE";
            }
        }

        return {};
    }

    game_string check_card_filter(card *origin_card, player *origin, target_card_filter filter, card *target) {
        if (!bool(filter & target_card_filter::can_target_self) && target == origin_card)
            return "ERROR_TARGET_PLAYING_CARD";
        
        if (bool(filter & target_card_filter::cube_slot)) {
            if (target != target->owner->m_characters.front() && target->color != card_color_type::orange)
                return "ERROR_TARGET_NOT_CUBE_SLOT";
        } else if (target->deck == card_deck_type::character) {
            return "ERROR_TARGET_NOT_CARD";
        }

        if (bool(filter & target_card_filter::beer) && !target->has_tag(tag_type::beer))
            return "ERROR_TARGET_NOT_BEER";

        if (bool(filter & target_card_filter::bang) && !origin->is_bangcard(target))
            return "ERROR_TARGET_NOT_BANG";

        if (bool(filter & target_card_filter::bangcard) && !target->has_tag(tag_type::bangcard))
            return "ERROR_TARGET_NOT_BANG";

        if (bool(filter & target_card_filter::missed) && !target->has_tag(tag_type::missedcard))
            return "ERROR_TARGET_NOT_MISSED";

        if (bool(filter & target_card_filter::bronco) && !target->has_tag(tag_type::bronco))
            return "ERROR_TARGET_NOT_BRONCO";

        if (bool(filter & target_card_filter::blue) && target->color != card_color_type::blue)
            return "ERROR_TARGET_NOT_BLUE_CARD";

        if (bool(filter & target_card_filter::clubs) && origin->get_card_sign(target).suit != card_suit::clubs)
            return "ERROR_TARGET_NOT_CLUBS";

        if (bool(filter & target_card_filter::black) != (target->color == card_color_type::black))
            return "ERROR_TARGET_BLACK_CARD";

        if (bool(filter & target_card_filter::table) && target->pocket != pocket_type::player_table)
            return "ERROR_TARGET_NOT_TABLE_CARD";

        if (bool(filter & target_card_filter::hand) && target->pocket != pocket_type::player_hand)
            return "ERROR_TARGET_NOT_HAND_CARD";

        return {};
    }

    game_string play_card_verify::verify_modifiers() const {
        for (card *mod_card : modifiers) {
            if (card *disabler = origin->m_game->get_disabler(mod_card)) {
                return {"ERROR_CARD_DISABLED_BY", mod_card, disabler};
            }
            switch(mod_card->modifier) {
            case card_modifier_type::bangmod:
            case card_modifier_type::bandolier:
                if (origin_card->pocket == pocket_type::player_hand) {
                    if (!origin->is_bangcard(origin_card)) {
                        return "ERROR_INVALID_MODIFIER_CARD";
                    }
                } else if (!origin_card->has_tag(tag_type::play_as_bang)) {
                    return "ERROR_INVALID_MODIFIER_CARD";
                }
                break;
            case card_modifier_type::leevankliff:
                if (origin_card != origin->m_last_played_card)
                    return "ERROR_INVALID_MODIFIER_CARD";
                break;
            case card_modifier_type::shopchoice:
            case card_modifier_type::discount:
                if (origin_card->expansion != card_expansion_type::goldrush)
                    return "ERROR_INVALID_MODIFIER_CARD";
                break;
            case card_modifier_type::belltower:
                switch (origin_card->pocket) {
                case pocket_type::player_hand:
                    if (origin_card->color != card_color_type::brown)
                        return "ERROR_INVALID_MODIFIER_CARD";
                    break;
                case pocket_type::player_table:
                    if (origin_card->effects.empty())
                        return "ERROR_INVALID_MODIFIER_CARD";
                    break;
                default:
                    if (origin_card->color == card_color_type::black)
                        return "ERROR_INVALID_MODIFIER_CARD";
                }
                break;
            default:
                return "ERROR_INVALID_MODIFIER_CARD";
            }
            for (const auto &effect : mod_card->effects) {
                if (game_string e = effect.verify(mod_card, origin)) {
                    return e;
                }
            }
        }
        return {};
    }

    game_string play_card_verify::verify_duplicates() const {
        duplicate_sets selected;

        for (card *mod_card : modifiers) {
            if (!selected.cards.emplace(mod_card).second) {
                return {"ERROR_DUPLICATE_CARD", mod_card};
            }
            for (const auto &effect : mod_card->effects) {
                if (effect.target == target_type::self_cubes) {
                    if (selected.cubes[mod_card] += effect.target_value > mod_card->num_cubes) {
                        return  {"ERROR_NOT_ENOUGH_CUBES_ON", mod_card};
                    }
                }
            }
        }

        auto &effects = is_response ? origin_card->responses : origin_card->effects;
        for (const auto &[target, effect] : zip_card_targets(targets, effects, origin_card->optionals)) {
            if (game_string error = enums::visit_indexed(
                [&]<target_type E>(enums::enum_tag_t<E>, auto && ... args) {
                    return play_visitor<E>{}.verify_duplicates(this, selected, effect, FWD(args) ... );
                }, target))
            {
                return error;
            }
        }

        return {};
    }

    void play_card_verify::play_modifiers() const {
        for (card *mod_card : modifiers) {
            origin->log_played_card(mod_card, false);
            origin->play_card_action(mod_card);
            for (effect_holder &e : mod_card->effects) {
                if (e.target == target_type::none) {
                    e.on_play(mod_card, origin, effect_flags{});
                } else if (e.target == target_type::self_cubes) {
                    origin->pay_cubes(mod_card, e.target_value);
                } else {
                    throw std::runtime_error("Invalid target_type in modifier card");
                }
            }
        }
    }

    game_string play_card_verify::verify_equip_target() const {
        if (card *disabler = origin->m_game->get_disabler(origin_card)) {
            return {"ERROR_CARD_DISABLED_BY", origin_card, disabler};
        }
        if (origin->m_game->check_flags(game_flags::disable_equipping)) {
            return "ERROR_CANT_EQUIP_CARDS";
        }
        if (auto error = origin->m_game->call_event<event_type::verify_play_card>(origin, origin_card, game_string{})) {
            return error;
        }
        player *target = origin;
        if (!origin_card->self_equippable()) {
            if (targets.size() != 1 || !targets.front().is(target_type::player)) {
                return "ERROR_INVALID_EQUIP_TARGET";
            }
            target = targets.front().get<target_type::player>();
            if (game_string error = check_player_filter(origin_card, origin, origin_card->equip_target, target)) {
                return error;
            }
        }
        if (card *equipped = target->find_equipped_card(origin_card)) {
            return {"ERROR_DUPLICATED_CARD", equipped};
        }
        if (origin_card->color == card_color_type::orange && origin->m_game->num_cubes < 3) {
            return "ERROR_NOT_ENOUGH_CUBES";
        }
        return {};
    }

    player *play_card_verify::get_equip_target() const {
        if (origin_card->self_equippable()) {
            return origin;
        } else {
            return targets.front().get<target_type::player>();
        }
    }

    game_string play_card_verify::verify_card_targets() const {
        auto &effects = is_response ? origin_card->responses : origin_card->effects;

        if (effects.empty()) {
            return "ERROR_EFFECT_LIST_EMPTY";
        }
        if (card *disabler = origin->m_game->get_disabler(origin_card)) {
            return {"ERROR_CARD_DISABLED_BY", origin_card, disabler};
        }
        if (origin_card->inactive) {
            return {"ERROR_CARD_INACTIVE", origin_card};
        }
        if (auto error = origin->m_game->call_event<event_type::verify_play_card>(origin, origin_card, game_string{})) {
            return error;
        }

        struct {
            raii_editor_stack<int8_t> data;
            void add(int8_t &value, int8_t diff) {
                data.add(value, value + diff);
            }
        } editors;

        for (card *c : modifiers) {
            switch (c->modifier) {
            case card_modifier_type::belltower: editors.add(origin->m_range_mod, 50); break;
            case card_modifier_type::bandolier: editors.add(origin->m_bangs_per_turn, 1); break;
            case card_modifier_type::leevankliff: editors.add(origin->m_bangs_per_turn, 10); break;
            }
        }

        if (game_string error = verify_modifiers()) {
            return error;
        }

        size_t diff = targets.size() - effects.size();
        if (auto repeatable = origin_card->get_tag_value(tag_type::repeatable)) {
            if (diff < 0 || diff % origin_card->optionals.size() != 0
                || (*repeatable > 0 && diff > (origin_card->optionals.size() * *repeatable)))
            {
                return "ERROR_INVALID_TARGETS";
            }
        } else if (diff != 0 && diff != origin_card->optionals.size()) {
            return "ERROR_INVALID_TARGETS";
        }

        target_list mth_targets;
        for (auto [target, effect] : zip_card_targets(targets, effects, origin_card->optionals)) {
            if (!target.is(effect.target)) {
                return "ERROR_INVALID_TARGET_TYPE";
            } else if (effect.type == effect_type::mth_add) {
                mth_targets.push_back(target);
            }
            
            if (game_string error = enums::visit_indexed(
                [&]<target_type E>(enums::enum_tag_t<E>, auto && ... args) {
                    return play_visitor<E>{}.verify(this, effect, FWD(args) ... );
                }, target))
            {
                return error;
            }
        }

        if (game_string error = (is_response ? origin_card->mth_response : origin_card->mth_effect).verify(origin_card, origin, mth_targets)) {
            return error;
        }

        if (game_string error = verify_duplicates()) {
            return error;
        }

        return {};
    }

    game_string play_card_verify::check_prompt() const {
        auto &effects = is_response ? origin_card->responses : origin_card->effects;

        target_list mth_targets;
        for (auto [target, effect] : zip_card_targets(targets, effects, origin_card->optionals)) {
            if (effect.type == effect_type::mth_add) {
                mth_targets.push_back(target);
            } else if (auto prompt_message = enums::visit_indexed(
                [&]<target_type E>(enums::enum_tag_t<E>, auto && ... args) {
                    return play_visitor<E>{}.prompt(this, effect, FWD(args) ... );
                }, target))
            {
                return prompt_message;
            }
        }

        return (is_response ? origin_card->mth_response : origin_card->mth_effect).on_prompt(origin_card, origin, mth_targets);
    }

    game_string play_card_verify::check_prompt_equip() const {
        player *target = get_equip_target();
        for (const auto &e : origin_card->equips) {
            if (auto prompt_message = e.on_prompt(origin, origin_card, target)) {
                return prompt_message;
            }
        }
        return {};
    }

    struct card_cube_ordering {
        bool operator()(card *lhs, card *rhs) const {
            if (lhs->pocket == pocket_type::player_table && rhs->pocket == pocket_type::player_table) {
                return std::ranges::find(lhs->owner->m_table, lhs) < std::ranges::find(rhs->owner->m_table, rhs);
            } else {
                return lhs->pocket == pocket_type::player_table;
            }
        }
    };

    void play_card_verify::do_play_card() const {
        auto &effects = is_response ? origin_card->responses : origin_card->effects;
        origin->log_played_card(origin_card, is_response);
        if (std::ranges::find(effects, effect_type::play_card_action, &effect_holder::type) == effects.end()) {
            origin->play_card_action(origin_card);
        }

        std::vector<std::pair<const effect_holder *, const play_card_target *>> delay_effects;
        std::map<card *, int, card_cube_ordering> selected_cubes;

        target_list mth_targets;
        for (const auto &[target, effect] : zip_card_targets(targets, effects, origin_card->optionals)) {
            if (effect.type == effect_type::mth_add) {
                mth_targets.push_back(target);
            } else if (effect.type == effect_type::pay_cube) {
                if (auto *cs = target.get_if<target_type::select_cubes>()) {
                    for (card *c : *cs) {
                        ++selected_cubes[c];
                    }
                } else if (target.is(target_type::self_cubes)) {
                    selected_cubes[origin_card] += effect.target_value;
                }
            } else {
                delay_effects.emplace_back(&effect, &target);
            }
        }
        for (const auto &[c, ncubes] : selected_cubes) {
            origin->pay_cubes(c, ncubes);
        }
        for (const auto &[e, t] : delay_effects) {
            enums::visit_indexed([&]<target_type E>(enums::enum_tag_t<E>, auto && ... args) {
                play_visitor<E>{}.play(this, *e, FWD(args) ... );
            }, *t);
        }

        (is_response ? origin_card->mth_response : origin_card->mth_effect).on_play(origin_card, origin, mth_targets);
        origin->m_game->call_event<event_type::on_effect_end>(origin, origin_card);
    }

    game_string play_card_verify::verify_and_play() {
        if (origin->m_game->pending_requests()) {
            const auto &req = origin->m_game->top_request();
            if (bool(req.flags() & effect_flags::force_play)) {
                if (!req.can_respond(origin, origin_card) && std::ranges::none_of(modifiers, [&](card *c) {
                    return req.can_respond(origin, c);
                })) {
                    return "ERROR_FORCED_CARD";
                }
            } else {
                return "ERROR_MUST_RESPOND_TO_REQUEST";
            }
        } else if (origin->m_game->m_playing != origin) {
            return "ERROR_PLAYER_NOT_IN_TURN";
        }

        switch(origin_card->pocket) {
        case pocket_type::player_hand:
            if (!modifiers.empty() && modifiers.front()->modifier == card_modifier_type::leevankliff) {
                card *bang_card = std::exchange(origin_card, origin->m_last_played_card);
                if (!origin->is_bangcard(bang_card)) {
                    return "ERROR_INVALID_MODIFIER_CARD";
                }
                if (game_string error = verify_card_targets()) {
                    return error;
                }
                origin->prompt_then(check_prompt(), [*this, bang_card]{
                    origin->m_game->move_card(bang_card, pocket_type::discard_pile);
                    origin->m_game->call_event<event_type::on_play_hand_card>(origin, bang_card);
                    do_play_card();
                    origin->set_last_played_card(nullptr);
                });
            } else if (origin_card->color == card_color_type::brown) {
                if (game_string error = verify_card_targets()) {
                    return error;
                }
                origin->prompt_then(check_prompt(), [*this]{
                    play_modifiers();
                    do_play_card();
                    origin->set_last_played_card(origin_card);
                });
            } else {
                if (game_string error = verify_equip_target()) {
                    return error;
                }
                origin->prompt_then(check_prompt_equip(), [*this]{
                    player *target = get_equip_target();
                    origin_card->on_equip(target);
                    if (origin == target) {
                        origin->m_game->add_log("LOG_EQUIPPED_CARD", origin_card, origin);
                    } else {
                        origin->m_game->add_log("LOG_EQUIPPED_CARD_TO", origin_card, origin, target);
                    }
                    target->equip_card(origin_card);
                    switch (origin_card->color) {
                    case card_color_type::blue:
                        if (origin->m_game->has_expansion(card_expansion_type::armedanddangerous)) {
                            origin->queue_request_add_cube(origin_card);
                        }
                        break;
                    case card_color_type::green:
                        origin_card->inactive = true;
                        origin->m_game->add_update<game_update_type::tap_card>(origin_card, true);
                        break;
                    case card_color_type::orange:
                        origin->add_cubes(origin_card, 3);
                        break;
                    }
                    origin->m_game->call_event<event_type::on_equip_card>(origin, target, origin_card);
                    origin->set_last_played_card(nullptr);
                    origin->m_game->call_event<event_type::on_effect_end>(origin, origin_card);
                });
            }
            break;
        case pocket_type::player_character:
        case pocket_type::player_table:
        case pocket_type::scenario_card:
        case pocket_type::button_row:
            if (game_string error = verify_card_targets()) {
                return error;
            }
            origin->prompt_then(check_prompt(), [*this]{
                play_modifiers();
                do_play_card();
                origin->set_last_played_card(nullptr);
            });
            break;
        case pocket_type::hidden_deck:
            if (std::ranges::find(modifiers, card_modifier_type::shopchoice, &card::modifier) == modifiers.end()) {
                return "ERROR_INVALID_MODIFIER_CARD";
            }
            [[fallthrough]];
        case pocket_type::shop_selection: {
            int cost = origin_card->buy_cost();
            for (card *c : modifiers) {
                switch (c->modifier) {
                case card_modifier_type::discount:
                    --cost;
                    break;
                case card_modifier_type::shopchoice:
                    if (c->get_tag_value(tag_type::shopchoice) != origin_card->get_tag_value(tag_type::shopchoice)) {
                        return "ERROR_INVALID_MODIFIER_CARD";
                    }
                    cost += c->buy_cost();
                    break;
                }
            }
            if (origin->m_game->m_shop_selection.size() > 3) {
                cost = 0;
            }
            if (origin->m_gold < cost) {
                return "ERROR_NOT_ENOUGH_GOLD";
            }
            if (origin_card->color == card_color_type::brown) {
                if (game_string error = verify_card_targets()) {
                    return error;
                }
                origin->prompt_then(check_prompt(), [*this, cost]{
                    play_modifiers();
                    origin->add_gold(-cost);
                    do_play_card();
                    origin->set_last_played_card(origin_card);
                    origin->m_game->queue_action([m_game = origin->m_game]{
                        while (m_game->m_shop_selection.size() < 3) {
                            m_game->draw_shop_card();
                        }
                    });
                });
            } else {
                if (game_string error = verify_modifiers()) {
                    return error;
                } else if (game_string error = verify_equip_target()) {
                    return error;
                }
                origin->prompt_then(check_prompt_equip(), [*this, cost]{
                    player *target = get_equip_target();
                    origin_card->on_equip(target);
                    if (origin == target) {
                        origin->m_game->add_log("LOG_BOUGHT_EQUIP", origin_card, origin);
                    } else {
                        origin->m_game->add_log("LOG_BOUGHT_EQUIP_TO", origin_card, origin, target);
                    }
                    play_modifiers();
                    origin->add_gold(-cost);
                    target->equip_card(origin_card);
                    origin->set_last_played_card(nullptr);
                    origin->m_game->queue_action([m_game = origin->m_game]{
                        while (m_game->m_shop_selection.size() < 3) {
                            m_game->draw_shop_card();
                        }
                    });
                });
            }
            break;
        }
        default:
            throw std::runtime_error("play_card: invalid card");
        }
        return {};
    }

    game_string play_card_verify::verify_and_respond() {
        if ((origin_card->pocket == pocket_type::player_hand && origin_card->color != card_color_type::brown)
            || origin_card->pocket == pocket_type::shop_selection
            || !modifiers.empty()
        ) {
            return "ERROR_INVALID_RESPONSE_CARD";
        }
        
        if (game_string error = verify_card_targets()) {
            return error;
        }
        origin->prompt_then(check_prompt(), [*this]{
            do_play_card();
            origin->set_last_played_card(nullptr);
        });
        return {};
    }
}