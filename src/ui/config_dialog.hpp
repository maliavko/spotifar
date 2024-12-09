#ifndef CONFIG_DIALOG_HPP_54DC8C7D_6C6A_4A6E_A947_501B698CBB0C
#define CONFIG_DIALOG_HPP_54DC8C7D_6C6A_4A6E_A947_501B698CBB0C
#pragma once

#include <DlgBuilder.hpp>

namespace spotifar
{
    namespace ui
    {
        class ConfigDialog
        {
        public:
            ConfigDialog();
            ~ConfigDialog();

            bool show();
        private:
            std::unique_ptr<PluginDialogBuilder> builder;
        };
    }
}

#endif //CONFIG_DIALOG_HPP_54DC8C7D_6C6A_4A6E_A947_501B698CBB0C