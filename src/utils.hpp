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

		std::string to_string(const std::wstring& ws);

		void init_logging();

		void fini_logging();

		class NoRedraw
		{
		public:
			NoRedraw(HANDLE hdlg);
			~NoRedraw();
		private:
			HANDLE hdlg;
		};

		intptr_t show_far_error_dlg(int error_msg_id, const std::wstring& extra_message = L"");
		intptr_t show_far_error_dlg(int error_msg_id, const std::string& extra_message = "");
	}
}


#endif //UTILS_HPP_64E82CD1_3EFD_41A4_BD43_6FC38FE138A8
