#ifndef __CARD_SERIAL_H__
#define __CARD_SERIAL_H__

#include "utils/small_pod.h"
#include "utils/utils.h"

namespace banggame {
    struct game_context;
    struct game;
    struct card;
    struct player;
    struct card_format;
}

template<typename T, typename ... Ts>
static constexpr bool is_one_of = (std::is_same_v<T, Ts> || ...);

#ifdef BUILD_BANG_CLIENT

namespace banggame {
    
    class card_view;
    class player_view;
    class game_context_view;

    namespace serial {
        using context = banggame::game_context_view;
        using card = banggame::card_view *;
        using opt_card = card;
        using player = banggame::player_view *;
        using opt_player = player;
        using card_format = banggame::card_format;
        using int_list = std::vector<int>;

        template<typename T>
        concept serializable = is_one_of<T, card, player>;
    }
}

#else

namespace banggame::serial {
    using context = banggame::game_context;
    using opt_card = banggame::card *;
    using card = not_null<opt_card>;
    using opt_player = banggame::player *;
    using player = not_null<opt_player>;
    using int_list = small_int_set;
    
    struct card_format {
        banggame::card *card;
        card_format() = default;
        card_format(banggame::card *card) : card(card) {}
    };

    template<typename T>
    concept serializable = is_one_of<T, opt_card, card, opt_player, player, card_format>;
}

#ifndef BUILD_BANG_SERVER
    #define NO_DEFINE_SERIALIZERS
#endif

#endif

namespace banggame::serial {
    using player_list = std::vector<player>;
    using card_list = std::vector<card>;
}

#ifndef NO_DEFINE_SERIALIZERS

namespace json {
    template<banggame::serial::serializable T> struct serializer<T, banggame::serial::context> {
        const banggame::serial::context &context;
        json operator()(same_if_trivial_t<T> value) const;
    };

    template<banggame::serial::serializable T> struct deserializer<T, banggame::serial::context> {
        const banggame::serial::context &context;
        T operator()(const json &value) const;
    };

}

#endif

#endif