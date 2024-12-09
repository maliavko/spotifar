#include "ui/config_dialog.hpp"
#include "config.hpp"
#include "lng.hpp"

namespace spotifar
{
    namespace ui
    {
        using namespace config;

        ConfigDialog::ConfigDialog():
            builder(std::make_unique<PluginDialogBuilder>(
                config::PsInfo, MainGuid, ConfigDialogGuid, MPluginUserName, L"Config"
            ))
        {
			builder->AddCheckbox(MConfigAddToDisksMenu, &Opt.AddToDisksMenu);

			builder->AddSeparator(MConfigSpotifySettings);
			builder->AddText(MConfigSpotifyClientID);
			builder->AddEditField(Opt.SpotifyClientID, ARRAYSIZE(Opt.SpotifyClientID), 40, L"", true);
			builder->AddText(MConfigSpotifyClientSecret);
			builder->AddEditField(Opt.SpotifyClientSecret , ARRAYSIZE(Opt.SpotifyClientSecret), 40, L"", true);
			builder->AddText(MConfigLocalhostServicePort);
			builder->AddIntEditField(&Opt.LocalhostServicePort, 10);

			builder->AddOKCancel(MOk, MCancel);
        }
        
        ConfigDialog::~ConfigDialog()
        {
            config::Opt.write();
        }
        
        bool ConfigDialog::show()
        {
            return builder->ShowDialog();
        }
    }
}