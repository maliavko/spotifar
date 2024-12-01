#ifndef PLAYER_HPP_7971099F_B147_4164_8937_5B72456164FF
#define PLAYER_HPP_7971099F_B147_4164_8937_5B72456164FF
#pragma once

#include "player.hpp"

namespace spotifar
{
    namespace spotify
    {
        class Player
        {
        public:
            std::chrono::duration<float> SYNC_INTERVAL = std::chrono::seconds(1);

        public:
            Player();
            virtual ~Player();

            void set_active(bool is_active);
        private:
            bool is_active;
        };
    }
}

#endif //PLAYER_HPP_7971099F_B147_4164_8937_5B72456164FF