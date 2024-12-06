#ifndef PLAYER_HPP_7971099F_B147_4164_8937_5B72456164FF
#define PLAYER_HPP_7971099F_B147_4164_8937_5B72456164FF
#pragma once

#include "player.hpp"

namespace spotifar
{
    namespace spotify
    {
        /*class Player
        {
        public:
            std::chrono::duration<float> SYNC_INTERVAL = std::chrono::seconds(1);

        public:
            Player();
            virtual ~Player();

            void add_listener(IPlayerListener* l);
            void remove_listener(IPlayerListener* l);

            typedef void (IPlayerListener::*EventFunction)(int, int);
            void notify_all(EventFunction func);

            void set_active(bool is_active);
        private:
            bool is_active;
            std::vector<IPlayerListener*> listeners;
        };*/
    }
}

#endif //PLAYER_HPP_7971099F_B147_4164_8937_5B72456164FF