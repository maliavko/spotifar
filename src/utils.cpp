#include "stdafx.h"
#include "utils.hpp"

#include <filesystem>

namespace spotifar
{
	namespace utils
	{
		extern const int KEY_CTRL = 0x100000, KEY_ALT = 0x200000, KEY_SHIFT = 0x400000;
		
		int input_record_to_combined_key(const KEY_EVENT_RECORD& kir)
		{
			int key = static_cast<int>(kir.wVirtualKeyCode);
			const auto state = kir.dwControlKeyState;
			
			if (state & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)) key |= KEY_CTRL;
			if (state & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED)) key |= KEY_ALT;
			if (state & SHIFT_PRESSED) key |= KEY_SHIFT;

			return key;
		}

		std::wstring get_plugin_launch_folder(const struct PluginStartupInfo* info)
		{
			return std::filesystem::path(info->ModuleName).parent_path().wstring();
		}

		std::string generate_random_string(const int length)
		{
			std::string text = "";
			const std::string possible = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

			for (int i = 0; i < length; i++)
			{
				float rand = (float)std::rand() / RAND_MAX * possible.length();
				text += possible[(int)std::floor(rand)];
			}
			return text;
		};

		std::wstring to_wstring(const std::string& s)
		{
			return std::wstring(s.begin(), s.end());
		}
		

        void Observable::add_observer(IObserver* o)
        {
            observers.push_back(o);
			//on_observers_changed();
        }

        void Observable::remove_observer(IObserver* o)
        {
            auto it = std::find(observers.begin(), observers.end(), o);
            if (it != observers.end())
                observers.erase(it);

			//on_observers_changed();
        }

        void Observable::notify_all(EventHandler handler)
        {
            for (auto& o: observers)
                (o->*handler)();
        }
	}
}
