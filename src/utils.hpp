#ifndef UTILS_HPP_64E82CD1_3EFD_41A4_BD43_6FC38FE138A8
#define UTILS_HPP_64E82CD1_3EFD_41A4_BD43_6FC38FE138A8
#pragma once

#include <plugin.hpp>
#include <string>
#include <format>
#include <thread>

namespace spotifar
{
	namespace utils
	{
		enum
		{
			CLR_BLACK = 0,
			CLR_BLUE,
			CLR_GREEN,
			CLR_CYAN,
			CLR_RED,
			CLR_PURPLE,
			CLR_BROWN,
			CLR_GRAY,
			CLR_DGRAY,
			CLR_LBLUE,
			CLR_LGREEN,
			CLR_LCYAN,
			CLR_LRED,
			CLR_LPURPLE,
			CLR_YELLOW,
			CLR_WHITE
		} Colors16;
		
		static const int KEY_CTRL, KEY_ALT, KEY_SHIFT;

		int input_record_to_combined_key(const KEY_EVENT_RECORD& kir);

		std::wstring get_plugin_launch_folder(const struct PluginStartupInfo* psInfo);

		std::string generate_random_string(const int);

		std::wstring to_wstring(const std::string& s);

		class IObserver
		{
		public:
			virtual ~IObserver() {}
		};

		class Observable
		{
		public:
            typedef void (IObserver::*EventHandler)();

		public:
			virtual ~Observable() {}

            void add_observer(IObserver* o);
            void remove_observer(IObserver* o);
            void notify_all(EventHandler handler);
		protected:
			virtual void on_observers_changed() {};

		protected:
			std::vector<IObserver*> observers;
		};
	}
}


#endif //UTILS_HPP_64E82CD1_3EFD_41A4_BD43_6FC38FE138A8
