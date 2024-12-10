#include "stdafx.h"
#include "utils.hpp"
#include "config.hpp"
#include "lng.hpp"

#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/msvc_sink.h"

#include <filesystem>

// specific overload for logging far VersionInfo struct
std::ostream& operator<<(std::ostream& os, const VersionInfo& c)
{ 
	return os << std::format("{}.{}.{}.{}.{}", c.Major, c.Minor, c.Revision, c.Build, (int)c.Stage); 
}

// fmt v10 and above requires `fmt::formatter<T>` extends `fmt::ostream_formatter`.
// See: https://github.com/fmtlib/fmt/issues/3318
template <> struct fmt::formatter<VersionInfo> : fmt::ostream_formatter {};

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
		
		std::string to_string(const std::wstring& ws)
		{
			// NOTE: the conversion is unsafe, use it only in case you know, that the
			// wstring does not have any complicated unicode specific symbols, otherwise
			// it will strip the data
			#pragma warning(suppress: 4244)  
			return std::string(ws.begin(), ws.end());
		}

		void init_logging()
		{
			// TODO: perhaps the plugin folder is not the best for storing logs, clarify
			auto filepath = std::format(L"{}\\logs\\spotifar.log", config::Opt.PluginStartupFolder);
			auto logger = spdlog::daily_logger_mt("plugin", filepath, 0, 0, false, 3);
			spdlog::set_default_logger(logger);

			#ifdef _DEBUG
				spdlog::set_level(spdlog::level::debug);
				
				// separate sink for debuging in VS Code
				logger->sinks().push_back(
					std::make_shared<spdlog::sinks::msvc_sink_mt>()
				);
			#else
				spdlog::set_level(spdlog::level::info);
			#endif

			spdlog::log(spdlog::level::off, "Plugin logging system is initialized, log level: {}",
				spdlog::level::to_string_view(spdlog::get_level()));
			spdlog::info("Plugin version: {}", PLUGIN_VERSION);
		}

		void fini_logging()
		{
			spdlog::log(spdlog::level::off, "Closing plugin\n\n");
			spdlog::shutdown();
		}

		NoRedraw::NoRedraw(HANDLE hdlg):
			hdlg(hdlg)
		{
			assert(hdlg);
			config::PsInfo.SendDlgMessage(hdlg, DM_ENABLEREDRAW, FALSE, 0);
		}

		NoRedraw::~NoRedraw()
		{
			config::PsInfo.SendDlgMessage(hdlg, DM_ENABLEREDRAW, TRUE, 0);
		}


		intptr_t show_far_error_dlg(int error_msg_id, const std::wstring& extra_message)
		{
			auto err_msg = config::get_msg(error_msg_id);
			const wchar_t* msgs[] = {
				config::get_msg(MFarMessageErrorTitle),
				err_msg, extra_message.c_str(),
				config::get_msg(MOk),
			};

			spdlog::error("Far error message dialog is shown, message id {}, {}", error_msg_id,
				utils::to_string(extra_message));
			
			FARMESSAGEFLAGS flags = FMSG_WARNING;
			if (GetLastError())  // if there's no error code, no need to show it in the dialog
				flags |= FMSG_ERRORTYPE;

			return config::PsInfo.Message(&MainGuid, &FarMessageGuid, flags, 0, msgs, ARRAYSIZE(msgs), 1);
		}
		
		intptr_t show_far_error_dlg(int error_msg_id, const std::string& extra_message)
		{
			return show_far_error_dlg(error_msg_id, utils::to_wstring(extra_message));
		}
	}
}
