#include "prompts.h"

#include "game/game.h"
#include "game/bot_suggestion.h"

#include "cards/filter_enums.h"

namespace banggame {

    game_string prompt_target_self(card_ptr origin_card, player_ptr origin, player_ptr target) {
        if (origin == target) {
            return {"PROMPT_TARGET_SELF", origin_card};
        }
        return {};
    }

    game_string prompt_target_ghost(card_ptr origin_card, player_ptr origin, player_ptr target) {
        if (target->is_ghost()) {
            return {"PROMPT_TARGET_GHOST", origin_card, target};
        } else {
            return {};
        }
    }

    game_string bot_check_target_enemy(player_ptr origin, player_ptr target) {
        if (origin->is_bot() && !bot_suggestion::is_target_enemy(origin, target)) {
            return "BOT_BAD_PLAY";
        }
        return {};
    }

    game_string bot_check_target_friend(player_ptr origin, player_ptr target) {
        if (origin->is_bot() && !bot_suggestion::is_target_friend(origin, target)) {
            return "BOT_BAD_PLAY";
        }
        return {};
    }

    game_string bot_check_target_enemy_card(player_ptr origin, card_ptr target) {
        if (target->pocket == pocket_type::player_table && !target->self_equippable() && !target->has_tag(tag_type::ghost_card)) {
            return bot_check_target_friend(origin, target->owner);
        } else {
            return bot_check_target_enemy(origin, target->owner);
        }
    }
}