#include "utils.hpp"
#include "config.hpp"

#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/msvc_sink.h"

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
		namespace far3
		{
			NoRedraw::NoRedraw(HANDLE hdlg):
				hdlg(hdlg)
			{
				assert(hdlg);
				std::lock_guard lock(mutex);
				send_dlg_msg(hdlg, DM_ENABLEREDRAW, FALSE, 0);
			}

			NoRedraw::~NoRedraw()
			{
				std::lock_guard lock(mutex);
				send_dlg_msg(hdlg, DM_ENABLEREDRAW, TRUE, 0);
			}

			int input_record_to_combined_key(const KEY_EVENT_RECORD &kir)
			{
				int key = static_cast<int>(kir.wVirtualKeyCode);
				const auto state = kir.dwControlKeyState;
				
				if (state & RIGHT_CTRL_PRESSED || state & LEFT_CTRL_PRESSED) key |= KEY_CTRL;
				if (state & RIGHT_ALT_PRESSED || state & LEFT_ALT_PRESSED) key |= KEY_ALT;
				if (state & SHIFT_PRESSED) key |= KEY_SHIFT;

				return key;
			}

			wstring get_plugin_launch_folder(const struct PluginStartupInfo *info)
			{
				return std::filesystem::path(info->ModuleName).parent_path().wstring();
			}

			intptr_t show_far_error_dlg(int error_msg_id, const wstring &extra_message)
			{
				auto err_msg = get_msg(error_msg_id);
				const wchar_t* msgs[] = {
					get_msg(MFarMessageErrorTitle),
					err_msg, extra_message.c_str(),
					get_msg(MOk),
				};

				spdlog::error("Far error message dialog is shown, message id {}, {}", error_msg_id,
					utils::to_string(extra_message));
				
				FARMESSAGEFLAGS flags = FMSG_WARNING;
				if (GetLastError())  // if there's no error code, no need to show it in the dialog
					flags |= FMSG_ERRORTYPE;

				return config::PsInfo.Message(&MainGuid, &FarMessageGuid, flags, 0, msgs, ARRAYSIZE(msgs), 1);
			}
			
			intptr_t show_far_error_dlg(int error_msg_id, const string &extra_message)
			{
				return show_far_error_dlg(error_msg_id, utils::to_wstring(extra_message));
			}
			
			intptr_t send_dlg_msg(HANDLE hdlg, intptr_t msg, intptr_t param1, void *param2)
			{
				return config::PsInfo.SendDlgMessage(hdlg, msg, param1, param2);
			}

			const wchar_t* get_msg(int msg_id)
			{
				return config::PsInfo.GetMsg(&MainGuid, msg_id);
			}
		
			static std::unordered_map<intptr_t, SynchroTaskT> synchro_tasks;
			
			void push_synchro_task(SynchroTaskT task)
			{
				static intptr_t task_id = 0;

				synchro_tasks[++task_id] = task;
				config::PsInfo.AdvControl(&MainGuid, ACTL_SYNCHRO, 0, (void*)task_id);
			}
			
			void process_synchro_event(intptr_t task_id)
			{
				auto it = synchro_tasks.find(task_id);
				if (it == synchro_tasks.end())
					return spdlog::error("There is no far synchro task registered with the given id, {}", task_id);

				try
				{
					it->second();
					synchro_tasks.erase(it);
				}
				catch (const std::exception &ex)
				{
					spdlog::error("There is an error while processing far synchro event, {}",
								  ex.what());
				}
			}

			void clear_synchro_events()
			{
				if (synchro_tasks.size() > 0)
					spdlog::error("Unexpected far synchro tasks are stuck in the queue, {}",
								  synchro_tasks.size());

				synchro_tasks.clear();
			}
		}

		namespace log
		{
        	std::shared_ptr<spdlog::logger> global = nullptr, api  = nullptr;

			void init()
			{
				wstring filepath;
				PWSTR app_data_path = NULL;
				HRESULT hres = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &app_data_path);

				// at first, we are trying to create a logs folder in users home directory,
				// if not possible, trying plugins home directory
				if (SUCCEEDED(hres))
					filepath = std::format(L"{}\\spotifar\\spotifar.log", app_data_path);
				else
					filepath = std::format(L"{}\\logs\\spotifar.log", config::get_plugin_launch_folder());

				// a default sink to the file 
				auto daily_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(filepath, 23, 59, false, 3);
				
				auto default_logger = global = std::make_shared<spdlog::logger>(_LOGGER_GLOBAL, daily_sink);
				spdlog::set_default_logger(default_logger);

				// specific logger for spotify api communication
				auto api_logger = api = std::make_shared<spdlog::logger>(_LOGGER_API, daily_sink);
				spdlog::register_logger(api_logger);

				#ifdef _DEBUG
					spdlog::set_level(spdlog::level::debug);
					
					// for debugging in VS Code this sink helps seeing the messages in the Debug Console view
					auto msvc_debug_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
					spdlog::apply_all([&msvc_debug_sink](const auto &logger)
					{
						logger->sinks().push_back(msvc_debug_sink);
					});
				#else
					spdlog::set_level(spdlog::level::info);
				#endif

				spdlog::log(spdlog::level::off, "Plugin logging system is initialized, log level: {}",
					spdlog::level::to_string_view(spdlog::get_level()));
				spdlog::info("Plugin version: {}", PLUGIN_VERSION);
			}

			void fini()
			{
				spdlog::log(spdlog::level::off, "Closing plugin\n\n");
				spdlog::shutdown();
			}
		}

		string generate_random_string(const int length)
		{
			string text = "";
			static const string possible = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

			for (int i = 0; i < length; i++)
			{
				float rand = (float)std::rand() / RAND_MAX * possible.length();
				text += possible[(int)std::floor(rand)];
			}
			return text;
		};

		wstring utf8_decode(const string &s)
		{
			int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), NULL, 0);
			wstring out(len, 0);
			MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &out[0], len);
			return out;
		}
		
		string utf8_encode(const wstring &ws)
		{
			int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), NULL, 0, NULL, NULL);
			string out(len, 0);
			WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), &out[0], len, NULL, NULL);
			return out;
		}

		wstring to_wstring(const string &s)
		{
			return wstring(s.begin(), s.end());
		}
		
		string to_string(const wstring &ws)
		{
			#pragma warning(suppress: 4244)  
			return string(ws.begin(), ws.end());
		}

		wstring strip_invalid_filename_chars(const wstring &filename)
		{
			static auto r = std::wregex(L"[\?\\\\/:*<>|]");
			return std::regex_replace(filename, r, L"_");
		}
	}
}
