#include "caches.hpp"
#include "config.hpp"
#include "utils.hpp"
#include "lng.hpp"
#include "spotifar.hpp"
#include "plugin.h"
#include "spotify/cache.hpp"
#include "spotify/auth.hpp"
#include "spotify/api.hpp"
#include "spotify/releases.hpp"

namespace spotifar { namespace ui { namespace settings {

using namespace utils::far3;
namespace fs = std::filesystem;

enum controls : int
{
    no_control = -1,
    dialog_box,

    logs_count_label,
    logs_count_value,
    logs_size_label,
    logs_size_value,
    logs_show_button,
    logs_clear_button,

    caches_separator,
    auth_cache_label,
    auth_clear_button,
    http_cache_label,
    http_clear_button,
    clear_all_button,

    releases_separator,
    releases_status_label,
    releases_status_value,
    releases_resync_button,

    buttons_separator,
    ok_button,
    cancel_button,
};

static const int
    logs_box_y = 2, // y position of a panel with logs settings
    caches_box_y = logs_box_y + 5,
    releases_box_y = caches_box_y + 6,
    buttons_box_y = releases_box_y + 4, // y position of a buttons panel
    width = 52, height = buttons_box_y + 4, // overall dialog height is a summ of all the panels included
    center_x = width / 2,
    box_x1 = 3, box_y1 = 1, box_x2 = width - 4, box_y2 = height - 2,
    view_x1 = box_x1 + 2, view_y1 = box_y1 + 1, view_x2 = box_x2 - 2, view_y2 = box_y2 - 1;

static const std::vector<FarDialogItem> dlg_items_layout{
    ctrl(DI_DOUBLEBOX,   box_x1, box_y1, box_x2, box_y2,                   DIF_NONE),
    
    ctrl(DI_TEXT,        view_x1, logs_box_y, center_x-2, 1,               DIF_RIGHTTEXT),
    ctrl(DI_TEXT,        center_x, logs_box_y, box_x2-center_x, 1,         DIF_NONE),
    ctrl(DI_TEXT,        view_x1, logs_box_y+1, center_x-2, 1,             DIF_RIGHTTEXT),
    ctrl(DI_TEXT,        center_x, logs_box_y+1, box_x2-center_x, 1,       DIF_NONE),
    ctrl(DI_BUTTON,      view_x1, logs_box_y+3, center_x-2, 1,             DIF_CENTERGROUP),
    ctrl(DI_BUTTON,      center_x, logs_box_y+3, box_x2, 1,                DIF_CENTERGROUP),
    
    ctrl(DI_TEXT,        -1, caches_box_y, box_x2, 1,                      DIF_SEPARATOR),
    ctrl(DI_TEXT,        view_x1, caches_box_y+1, box_x2-center_x, 1,      DIF_RIGHTTEXT),
    ctrl(DI_BUTTON,      center_x, caches_box_y+1, box_x2, 1,              DIF_NONE),
    ctrl(DI_TEXT,        view_x1, caches_box_y+2, box_x2-center_x, 1,      DIF_RIGHTTEXT),
    ctrl(DI_BUTTON,      center_x, caches_box_y+2, box_x2, 1,              DIF_NONE),
    ctrl(DI_BUTTON,      center_x, caches_box_y+4, box_x2, 1,              DIF_CENTERGROUP),

    ctrl(DI_TEXT,        -1, releases_box_y, box_x2, 1,                    DIF_SEPARATOR),
    ctrl(DI_TEXT,        view_x1, releases_box_y+1, center_x-2, 1,         DIF_RIGHTTEXT),
    ctrl(DI_TEXT,        center_x, releases_box_y+1, box_x2-center_x, 1,   DIF_NONE),
    ctrl(DI_BUTTON,      view_x1, releases_box_y+3, box_x2, 1,             DIF_CENTERGROUP),
    
    // buttons block
    ctrl(DI_TEXT,        box_x1, buttons_box_y, box_x2, box_y2,            DIF_SEPARATOR),
    ctrl(DI_BUTTON,      box_x1, buttons_box_y+1, box_x2, box_y2,          DIF_CENTERGROUP | DIF_DEFAULTBUTTON),
    ctrl(DI_BUTTON,      box_x1, buttons_box_y+1, box_x2, box_y2,          DIF_CENTERGROUP),
};

static bool remove_http_cache()
{
    const auto &filepath = fs::path(spotify::get_cache_filename());
    
    std::error_code ec;
    if (!fs::remove(filepath, ec))
    {
        // TODO: show dialog?
        return false;
    }

    return true;
}

static bool clear_credentials()
{
    auto ctx = config::lock_settings();
    // recalculate the size
    return ctx->delete_value(spotify::auth_config_key);
}

caches_dialog::caches_dialog():
    modal_dialog(&ConfigCachesDialogGuid, width, height, dlg_items_layout, L"ConfigCaches")
{
    utils::events::start_listening<spotify::releases_observer>(this);
}

caches_dialog::~caches_dialog()
{
    utils::events::stop_listening<spotify::releases_observer>(this);
}

void caches_dialog::set_releases_sync_status(size_t items_left)
{
    static wstring status;

    if (items_left > 0)
        status = std::format(L"{} left", items_left);
    else
        status = L"Finished";

    dialogs::set_text(hdlg, releases_status_value, status.c_str());
    dialogs::enable(hdlg, releases_resync_button, items_left == 0);
}

string format_file_size(uintmax_t size)
{
    std::ostringstream os;

    int o{};
    if (size == 0) return "0B";

    double mantissa = (double)size;
    for (; mantissa >= 1024.; mantissa /= 1024., ++o);
    os << std::ceil(mantissa * 10.) / 10. << "BKMGTPE"[o];
    return os.str();
}

void caches_dialog::init()
{
    int logs_count = 0;
    size_t file_size = 0;

    std::error_code ec;
    for (auto &entry: std::filesystem::directory_iterator(utils::log::get_logs_folder()))
    {
        if (entry.is_regular_file(ec))
        {
            file_size += entry.file_size(ec);
            ++logs_count;
        }
    }

    wstring http_btn_label = L"Clear";

    auto cache_filepath = std::filesystem::path(spotify::get_cache_filename());
    if (auto size = std::filesystem::file_size(cache_filepath, ec); !ec)
        http_btn_label = std::format(L"Clear ({})", utils::to_wstring(format_file_size(size)));

    auto plugin = get_plugin();
    
    dialogs::set_text(hdlg, dialog_box, L"Logs");
    dialogs::set_text(hdlg, logs_count_label, L"Logs count:");
    dialogs::set_text(hdlg, logs_count_value, std::to_wstring(logs_count));
    dialogs::set_text(hdlg, logs_size_label, L"Folder size:");
    dialogs::set_text(hdlg, logs_size_value, utils::to_wstring(format_file_size(file_size)).c_str());
    dialogs::set_text(hdlg, logs_show_button, L"Show logs");
    dialogs::set_text(hdlg, logs_clear_button, L"Clear logs");
    
    dialogs::set_text(hdlg, caches_separator, L"Caches");
    dialogs::set_text(hdlg, auth_cache_label, L"Credentials");
    dialogs::set_text(hdlg, auth_clear_button, L"Clear");
    dialogs::set_text(hdlg, http_cache_label, L"Http reponses");
    dialogs::set_text(hdlg, http_clear_button, http_btn_label.c_str());
    dialogs::set_text(hdlg, clear_all_button, L"Clear All");
    
    dialogs::set_text(hdlg, releases_separator, L"Releases scan");
    dialogs::set_text(hdlg, releases_status_label, L"Sync status:");
    dialogs::set_text(hdlg, releases_status_value, L"Finished");
    dialogs::set_text(hdlg, releases_resync_button, L"Resync");
    dialogs::enable(hdlg, releases_resync_button, plugin != nullptr);

    if (plugin)
    {
        if (auto api = plugin->get_api(); auto releases = api->get_releases())
            set_releases_sync_status(releases->get_sync_tasks_left());
    }

    dialogs::set_text(hdlg, ok_button, get_text(MOk));
    dialogs::set_text(hdlg, cancel_button, get_text(MCancel));
}

intptr_t caches_dialog::handle_result(intptr_t dialog_run_result)
{
    return FALSE;
}

bool caches_dialog::handle_btn_clicked(int ctrl_id)
{
    switch (ctrl_id)
    {
        case logs_show_button:
        {
            panels::set_directory(PANEL_PASSIVE, log::get_logs_folder());
            close();
            return true;
        }
        case logs_clear_button:
        {
            std::error_code ec;
            for (auto &entry: fs::directory_iterator(utils::log::get_logs_folder()))
            {
                if (entry.is_regular_file(ec))
                {
                    if (!fs::remove(entry.path(), ec))
                        log::global->error("Could not remove file '{}': {}", utils::to_string(entry.path()), ec.message());
                }
            }
            return true;
        }
        case auth_clear_button:
        {
            clear_credentials();
            return true;
        }
        case http_clear_button:
        {
            remove_http_cache();
            return true;
        }
        case clear_all_button:
        {
            clear_credentials();
            // clear library & releases
            remove_http_cache();
            return true;
        }
        case releases_resync_button:
        {
            if (auto plugin = get_plugin())
            {
                if (auto api = plugin->get_api(); auto releases = api->get_releases())
                    releases->invalidate();
            }
            return true;
        }
    }
    return false;
}

void caches_dialog::on_sync_progress_changed(size_t items_left)
{
    set_releases_sync_status(items_left);
}

} // namespace settings
} // namespace ui
} // namespace spotifar