#ifndef __LOBBY_H__
#define __LOBBY_H__

#include "options.h"
#include "messages.h"

#include "game/game.h"

#include "utils/linked_hash_map.h"

namespace banggame {

class game_manager;
struct game_user;
struct lobby;

using client_handle = std::weak_ptr<void>;
using user_map = std::unordered_map<id_type, game_user>;
using client_to_user_map = std::map<client_handle, game_user *, std::owner_less<>>;

DEFINE_ENUM(lobby_team,
    (game_player)
    (game_spectator)
)

static constexpr ticks lobby_lifetime = 5min;
static constexpr ticks user_lifetime = 10s;

static constexpr ticks ping_interval = 10s;
static constexpr auto pings_until_disconnect = 2min / ping_interval;

using lobby_list = ppstd::linked_hash_map<id_type, lobby>;
using lobby_ptr = lobby_list::iterator;

struct game_user: user_info {
    game_user(const user_info &info, id_type session_id)
        : user_info{info}, session_id{session_id} {}
    
    id_type session_id = 0;
    lobby *in_lobby = nullptr;

    client_handle client;

    ticks ping_timer = ticks{0};
    int ping_count = 0;
    ticks lifetime = user_lifetime;
};

struct lobby_user {
    lobby_team team;
    int user_id;
    game_user *user;
};

struct lobby_bot: user_info {
    lobby_bot(const user_info &info, int user_id)
        : user_info{info}, user_id{user_id} {}

    int user_id;
};

struct lobby : lobby_info {
    lobby(const lobby_info &info, id_type lobby_id)
        : lobby_info{info}, lobby_id{lobby_id} {}

    id_type lobby_id;
    int user_id_count = 0;

    std::vector<lobby_user> users;
    std::vector<lobby_bot> bots;
    std::vector<lobby_chat_args> chat_messages;

    lobby_user &add_user(game_user &user);

    int get_user_id(const game_user &user) const {
        if (auto it = rn::find(users, &user, &lobby_user::user); it != users.end()) {
            return it->user_id;
        }
        return 0;
    }
    
    lobby_state state;
    ticks lifetime = lobby_lifetime;

    std::unique_ptr<banggame::game> m_game;
    void start_game(game_manager &mgr);
    void send_updates(game_manager &mgr);
    lobby_data make_lobby_data() const;
};

}

#endif