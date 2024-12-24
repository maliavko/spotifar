#ifndef UTILS_HPP_64E82CD1_3EFD_41A4_BD43_6FC38FE138A8
#define UTILS_HPP_64E82CD1_3EFD_41A4_BD43_6FC38FE138A8
#pragma once

#include <plugin.hpp>
#include <string>

namespace spotifar
{
	namespace utils
	{
		using clock = std::chrono::system_clock;

		namespace far3
		{
			enum Colors16
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
			};

			static const int KEY_CTRL = 0x100000, KEY_ALT = 0x200000, KEY_SHIFT = 0x400000;

			int input_record_to_combined_key(const KEY_EVENT_RECORD &kir);

			std::wstring get_plugin_launch_folder(const PluginStartupInfo *psInfo);

			class _NODISCARD NoRedraw
			{
			public:
				NoRedraw(HANDLE hdlg);
				~NoRedraw();
			private:
				HANDLE hdlg;
			};

			intptr_t show_far_error_dlg(int error_msg_id, const std::wstring &extra_message = L"");
			intptr_t show_far_error_dlg(int error_msg_id, const std::string &extra_message = "");
			
			intptr_t send_dlg_msg(HANDLE hdlg, intptr_t msg, intptr_t param1, void* param2);
			
			const wchar_t* get_msg(int msg_id);
		}

		std::string generate_random_string(const int);

		/// @brief Converts utf8 encoded string into wide-char one
		std::wstring utf8_decode(const std::string &s);
		
		/// @brief Converts wide-char string into utf8 encoded string
		std::string utf8_encode(const std::wstring &ws);

		/// @brief Bluntly converts char string into wide-char string
		std::wstring to_wstring(const std::string &s);
		
		/// @brief Bluntly converts char string into wide-char string
		/// NOTE: The function does not care about string encoding, all the multi-byte
		/// stuff will be broken miserably
		std::string to_string(const std::wstring &ws);

		std::wstring strip_invalid_filename_chars(const std::wstring &filename);

		static const char *LOGGER_GLOBAL = "global", *LOGGER_API = "api";

		void init_logging();
		void fini_logging();
	}
}


#endif //UTILS_HPP_64E82CD1_3EFD_41A4_BD43_6FC38FE138A8
