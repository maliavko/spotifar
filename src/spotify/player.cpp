#include "player.hpp"

namespace spotifar
{
    namespace spotify
    {
        Player::Player():
            is_active(false)
        {
        }

        Player::~Player()
        {
            set_active(false);
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
        }
    }
}