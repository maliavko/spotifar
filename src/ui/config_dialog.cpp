#include "ui/config_dialog.hpp"
#include "config.hpp"
#include "lng.hpp"

namespace spotifar
{
    namespace ui
    {
        ConfigDialog::ConfigDialog():
            builder(std::make_unique<PluginDialogBuilder>(
                config::PsInfo, MainGuid, ConfigDialogGuid, MPluginUserName, L"Config"
            ))
        {
            auto ctx = config::lock_settings();
            auto& s = ctx->get_settings();

			builder->AddCheckbox(MConfigAddToDisksMenu, &s.add_to_disk_menu);

			builder->AddSeparator(MConfigSpotifySettings);
			builder->AddText(MConfigSpotifyClientID);
			builder->AddEditField(&s.spotify_client_id[0], (int)s.spotify_client_id.size() + 1, 40, L"", true);
			builder->AddText(MConfigSpotifyClientSecret);
			builder->AddEditField(&s.spotify_client_secret[0], (int)s.spotify_client_secret.size() + 1, 40, L"", true);
			builder->AddText(MConfigLocalhostServicePort);
			builder->AddIntEditField(&s.localhost_service_port, 10);

			builder->AddOKCancel(MOk, MCancel);
        }
        
        ConfigDialog::~ConfigDialog()
        {
            config::write();
        }
        
        bool ConfigDialog::show()
        {
            return builder->ShowDialog();
        }
    }
}