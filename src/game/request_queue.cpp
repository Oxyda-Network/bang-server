#include "request_queue.h"

#include "game.h"

namespace banggame {
    
    void request_queue::tick() {
        if (m_game->is_game_over()) return;

        if (m_update_timer && --(*m_update_timer) <= ticks{}) {
            m_update_timer.reset();
            if (auto req = top_request()) {
                req->on_update();

                if (req->state == request_state::dead) {
                    update();
                } else {
                    req->state = request_state::live;
                    if (auto *timer = req->timer()) {
                        timer->start(m_game->get_total_update_time());
                    }
                    m_game->send_request_update();

                    for (player *p : m_game->m_players) {
                        if (p->is_bot() && m_game->request_bot_play(p, true)) {
                            break;
                        }
                    }
                }
            } else if (!m_delayed_actions.empty()) {
                auto fun = std::move(m_delayed_actions.top().first);
                m_delayed_actions.pop();
                std::invoke(std::move(fun));
                update();
            } else if (m_game->m_playing) {
                if (m_game->m_playing->is_bot()) {
                    m_game->request_bot_play(m_game->m_playing, false);
                } else {
                    m_game->send_request_status_ready();
                }
            }
        } else if (auto req = top_request()) {
            if (auto *timer = req->timer()) {
                timer->tick();
                if (timer->finished()) {
                    m_game->send_request_status_clear();
                    pop_request();
                    timer->on_finished();
                    update();
                }
            }
        }
    }
    
    void request_queue::update() {
        m_update_timer = m_game->get_total_update_time();
    }
}