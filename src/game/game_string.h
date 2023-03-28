#ifndef __GAME_STRING_H__
#define __GAME_STRING_H__

#include "cards/card_enums.h"

namespace banggame {

    DEFINE_STRUCT(card_format_id,
        (std::string, name)
        (card_sign, sign),

        card_format_id() = default;
        card_format_id(card *value);
    )

    DEFINE_ENUM_TYPES(game_format_arg_type,
        (integer, int)
        (string, std::string)
        (card, card_format_id)
        (player, serial::opt_player)
    )

    using game_format_arg = enums::enum_variant<game_format_arg_type>;
    
    DEFINE_STRUCT(game_string,
        (std::string, format_str)
        (std::vector<game_format_arg>, format_args),

        game_string() = default;
    
        game_string(
                std::convertible_to<std::string> auto &&message,
                auto && ... args)
            : format_str(FWD(message))
            , format_args{game_format_arg(FWD(args)) ...} {}

        explicit operator bool() const {
            return !format_str.empty();
        }
    )
    
    #define MAYBE_RETURN(...) if (auto value_ = __VA_ARGS__) return value_
}

#endif