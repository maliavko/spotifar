#include "stdafx.h"
#include "config.hpp"
#include "ui/config_dialog.hpp"

namespace spotifar
{
    namespace ui
    {
        bool ConfigDialog::show()
        {
            PluginDialogBuilder builder(
                config::PsInfo, MainGuid, ConfigDialogGuid, MPluginUserName, L"Config"
            );
            {
                auto ctx = config::lock_settings();
                auto &s = ctx->get_settings();

                // dialog's layout
                builder.AddCheckbox(MConfigAddToDisksMenu, &s.add_to_disk_menu);
                builder.AddSeparator(MConfigSpotifySettings);
                builder.AddText(MConfigSpotifyClientID);
                builder.AddEditField(s.spotify_client_id, ARRAYSIZE(s.spotify_client_id), 40, L"", false);
                builder.AddText(MConfigSpotifyClientSecret);
                builder.AddEditField(s.spotify_client_secret, ARRAYSIZE(s.spotify_client_secret), 40, L"", false);
                builder.AddText(MConfigLocalhostServicePort);
                builder.AddIntEditField(&s.localhost_service_port, 10);
                builder.AddOKCancel(MOk, MCancel);
            }
            
            if (builder.ShowDialog())
            {
                config::write();
                return true;
            }
            return false;
        }
    }
}