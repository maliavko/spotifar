#include "player.hpp"

namespace spotifar
{
    namespace spotify
    {
        /*Player::Player():
            is_active(false)
        {
        }

        Player::~Player()
        {
            set_active(false);
        }

        void Player::add_listener(IPlayerListener* l)
        {
            listeners.push_back(l);
        }

        void Player::remove_listener(IPlayerListener* l)
        {
            auto it = std::find(listeners.begin(), listeners.end(), l);
            if (it != listeners.end())
                listeners.erase(it);

            notify_all(&IPlayerListener::on_track_progress_changed);
        }

        void Player::notify_all(EventFunction func)
        {
            for (auto& l: listeners)
            {
                (l->*func)(1, 1);
            }
        }

        void Player::set_active(bool is_active)
        {
            if (this->is_active == is_active)
                return;

            this->is_active = is_active;

            std::packaged_task<void()> task([this]
            {
                while (this->is_active)
                {
                    std::this_thread::sleep_for(SYNC_INTERVAL);
                }
            });

            std::thread(std::move(task)).detach();
        }*/
    }
}