#ifndef CONFIG_HPP_B490198E_23A2_4583_A1B8_80FA1450E83B
#define CONFIG_HPP_B490198E_23A2_4583_A1B8_80FA1450E83B
#pragma once

#include "stdafx.h"

namespace spotifar
{
    namespace config
    {
        extern PluginStartupInfo PsInfo;
        extern FarStandardFunctions FSF;

        enum HotkeyID
        {
            PLAY = 333,
            SKIP_NEXT,
            SKIP_PREV,
            SEEK_FORWARD,
            SEEK_BACKWARD,
            VOLUME_UP,
            VOLUME_DOWN,
            LAST
        };
        
        struct ConfigObserver: public BaseObserverProtocol
        {
            virtual void on_global_hotkeys_setting_changed(bool is_enabled) {};
            virtual void on_global_hotkey_changed(HotkeyID hotkey_id, WORD virtual_key, WORD modifiers) {};
        };

        struct Settings
        {
            bool add_to_disk_menu;
            bool is_global_hotkeys_enabled;
            int localhost_service_port;
            wstring spotify_client_id, spotify_client_secret;
            wstring plugin_startup_folder;
            // pair(key_virtual_code, key_modifiers)
            std::unordered_map<HotkeyID, std::pair<WORD, WORD>> hotkeys;
        };

        class SettingsContext
        {
        public:
            SettingsContext();
            ~SettingsContext();

            bool get_bool(const wstring &name, bool def);
            std::int64_t get_int64(const wstring &name, std::int64_t def);
            int get_int(const wstring &name, int def);
            const wstring get_wstr(const wstring &name, const wstring &def);
            string get_str(const wstring &name, const string &def);
            
            void set_bool(const wstring &name, bool value);
            void set_int64(const wstring &name, std::int64_t value);
            void set_int(const wstring &name, int value);
            void set_wstr(const wstring &name, const wstring &value);
            void set_str(const wstring &name, const string &value);

            bool delete_value(const wstring &name);

            Settings& get_settings();
        private:
            PluginSettings ps;
            Settings settings_copy;
        };

        std::shared_ptr<SettingsContext> lock_settings();

        void read(const PluginStartupInfo *info);
        void write();
        
        bool is_added_to_disk_menu();

        bool is_global_hotkeys_enabled();

        string get_client_id();

        string get_client_secret();

        int get_localhost_port();

        const wstring& get_plugin_launch_folder();

        std::pair<WORD, WORD>* get_hotkey(HotkeyID key);
    }
}

#endif //CONFIG_HPP_B490198E_23A2_4583_A1B8_80FA1450E83B