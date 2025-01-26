#ifndef VIEW_HPP_BD8268F0_532D_4A60_9847_B08580783467
#define VIEW_HPP_BD8268F0_532D_4A60_9847_B08580783467
#pragma once

#include "stdafx.h"

namespace spotifar
{
    namespace ui
    {
        using std::string;
        using std::wstring;

        struct ViewItem
        {
            string id;
            wstring name;
            wstring description;
            uintptr_t file_attrs;
            size_t duration;

            ViewItem(const string &id, const wstring &name, const wstring &descr,
                uintptr_t attrs = 0, size_t duration = 0);
        };

		struct ItemFarUserData
		{
			string id;
		};

        class View
        {
        public:
            typedef std::vector<ViewItem> Items;

        public:
            View(const wstring &name): name(name), id(id) {}
            virtual ~View() {}

            inline const wstring& get_name() const { return name; }

            virtual void on_panel_updated(OpenPanelInfo *info) {}
            virtual intptr_t process_input(const ProcessPanelInputInfo *info) { return FALSE; }
            virtual Items get_items() = 0;
            virtual std::shared_ptr<View> select_item(const ItemFarUserData *data) = 0;

        protected:
            wstring name;
            string id;
        };
    }
}
#endif // VIEW_HPP_BD8268F0_532D_4A60_9847_B08580783467