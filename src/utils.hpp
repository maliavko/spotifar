#ifndef UTILS_HPP_64E82CD1_3EFD_41A4_BD43_6FC38FE138A8
#define UTILS_HPP_64E82CD1_3EFD_41A4_BD43_6FC38FE138A8
#pragma once

#include "stdafx.h"
#include "config.hpp"

namespace spotifar
{
	namespace utils
	{
		using clock = std::chrono::system_clock;
		using ms = std::chrono::milliseconds;
        using SettingsCtx = config::SettingsContext;

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

			static const int
				KEY_A = 0x41,
				KEY_D = 0x44,
				KEY_R = 0x52,
				KEY_S = 0x53,
				KEY_CTRL = 0x100000,
				KEY_ALT = 0x200000,
				KEY_SHIFT = 0x400000;

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
		
			typedef std::function<void(void)> SynchroTaskT;
			void push_synchro_task(SynchroTaskT task);
			void process_synchro_event(intptr_t task_id);
			void clear_synchro_events();


			struct IStorableData
			{
				virtual ~IStorableData() {};

				virtual void read(SettingsCtx &ctx) = 0;
				virtual void write(SettingsCtx &ctx) = 0;
				virtual void clear(SettingsCtx &ctx) = 0;
			};
			
			template<class DataType>
			class StorageValue: public IStorableData
			{
			public:
				StorageValue(const std::wstring &storage_key);

				virtual void read(SettingsCtx &ctx);
				virtual void write(SettingsCtx &ctx);
				virtual void clear(SettingsCtx &ctx);

				const DataType& get() const { return data; }
				void set(const DataType &d) { data = d; }

			protected:
				virtual void read_from_settings(SettingsCtx &ctx, const std::wstring &key, DataType& data) = 0;
				virtual void write_to_settings(SettingsCtx &ctx, const std::wstring &key, DataType& data) = 0;

			private:
				const std::wstring storage_key;
				DataType data;
			};
			
			template<class T>
			StorageValue<T>::StorageValue(const std::wstring &storage_key):
				storage_key(storage_key)
				{}

			template<class T>
			void StorageValue<T>::read(SettingsCtx &ctx)
			{
				try
				{
					read_from_settings(ctx, storage_key, data);
				}
				catch(const std::exception &e)
				{
					// in case of an error, just discard a stored data and drop an error message to log
					spdlog::error("Cached value \"{}\" is broken, discarding. {}",
						utils::to_string(storage_key), e.what());
					clear(ctx);
				}
			}

			template<class T>
			void StorageValue<T>::write(SettingsCtx &ctx)
			{
				write_to_settings(ctx, storage_key, data);
			}

			template<class T>
			void StorageValue<T>::clear(SettingsCtx &ctx)
			{
				ctx.delete_value(storage_key);
			}
		}
	}
}


#endif //UTILS_HPP_64E82CD1_3EFD_41A4_BD43_6FC38FE138A8
