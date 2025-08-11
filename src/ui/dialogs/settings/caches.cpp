#include <time.h>
#include "caches.hpp"
#include "config.hpp"
#include "utils.hpp"
#include "lng.hpp"
#include "spotifar.hpp"
#include "plugin.h" // IWYU pragma: keep
#include "spotify/cache.hpp"
#include "spotify/auth.hpp"

namespace spotifar { namespace ui { namespace settings {

using no_redraw_caches = no_redraw<caches_dialog>;
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
    releases_next_sync_lbl,
    releases_next_sync_value,
    releases_resync_button,

    buttons_separator,
    ok_button,
    cancel_button,
};

static const int
    logs_box_y = 2, // y position of a panel with logs settings
    caches_box_y = logs_box_y + 5,
    releases_box_y = caches_box_y + 6,
    buttons_box_y = releases_box_y + 6, // y position of a buttons panel
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
    ctrl(DI_TEXT,        view_x1, releases_box_y+2, center_x-2, 1,         DIF_RIGHTTEXT),
    ctrl(DI_TEXT,        center_x, releases_box_y+2, box_x2-center_x, 1,   DIF_NONE),
    ctrl(DI_BUTTON,      view_x1, releases_box_y+4, box_x2, 1,             DIF_CENTERGROUP),
    
    // buttons block
    ctrl(DI_TEXT,        box_x1, buttons_box_y, box_x2, box_y2,            DIF_SEPARATOR),
    ctrl(DI_BUTTON,      box_x1, buttons_box_y+1, box_x2, box_y2,          DIF_CENTERGROUP | DIF_DEFAULTBUTTON),
    ctrl(DI_BUTTON,      box_x1, buttons_box_y+1, box_x2, box_y2,          DIF_CENTERGROUP),
};

static string format_size(uintmax_t size)
{
    return utils::format_number(size, 1024, "BKMGTPE");
}

/// @brief Simple helper to calculate files and its total size in the given `folder_path`.
/// In case of errors of getting stats on the individual files, just skip entries and
/// count only valid ones
/// @return std::pair<files_count, files_total_size>
static std::pair<size_t, std::uintmax_t> get_folder_stats(const wstring &folder_path)
{
    size_t logs_count = 0;
    std::uintmax_t total_size = 0;

    std::error_code ec;
    for (auto &entry: fs::directory_iterator(folder_path))
    {
        if (entry.is_regular_file(ec))
        {
            total_size += entry.file_size(ec);
            ++logs_count;
        }
    }
    return std::make_pair(logs_count, total_size);
}

static void update_logs_block(HANDLE hdlg)
{
    no_redraw_caches nr(hdlg);

    static wstring logs_count, logs_size;
    
    auto stats = get_folder_stats(utils::log::get_logs_folder());

    logs_count = std::to_wstring(stats.first);
    logs_size = utils::to_wstring(format_size(stats.second));
    
    dialogs::set_text(hdlg, dialog_box, get_text(MCfgLogs));
    dialogs::set_text(hdlg, logs_count_label, get_text(MCfgLogsCount));
    dialogs::set_text(hdlg, logs_count_value, logs_count.c_str());
    dialogs::set_text(hdlg, logs_size_label, get_text(MCfgLogsFolderSize));
    dialogs::set_text(hdlg, logs_size_value, logs_size.c_str());
    dialogs::set_text(hdlg, logs_show_button, get_text(MCfgShowLogsBtn));
    dialogs::set_text(hdlg, logs_clear_button, get_text(MCfgClearLogsBtn));
}

static void update_caches_block(HANDLE hdlg)
{
    no_redraw_caches nr(hdlg);

    static wstring http_btn_label;

    std::error_code ec;
    auto size = fs::file_size(spotify::get_cache_filename(), ec);

    // ignore errors, just show 0B size on the buttong
    http_btn_label = get_vtext(MCfgHttpCacheClearBtn,
        utils::to_wstring(format_size(ec ? 0 : size)));
    
    dialogs::set_text(hdlg, caches_separator, get_text(MCfgCaches));
    dialogs::set_text(hdlg, auth_cache_label, get_text(MCfgCredentials));
    dialogs::set_text(hdlg, auth_clear_button, get_text(MCfgCredentialsClearBtn));
    dialogs::set_text(hdlg, http_cache_label, get_text(MCfgHttpCache));
    dialogs::set_text(hdlg, http_clear_button, http_btn_label.c_str());
    dialogs::set_text(hdlg, clear_all_button, get_text(MCfgClearAllCaches));
}

void set_releases_sync_status(HANDLE hdlg)
{
    no_redraw_caches nr(hdlg);

    static wstring status_lbl, next_sync_time_lbl;

    size_t items_left = 0;
    bool is_resync_button_enabled = false;
    utils::clock_t::time_point next_sync_time{};

    if (auto plugin = get_plugin())
        if (auto api = plugin->get_api())
            if (auto releases = api->get_releases())
            {
                items_left = releases->get_sync_tasks_left();
                next_sync_time = releases->get_next_sync_time();
                is_resync_button_enabled = items_left == 0;
            }

    if (items_left > 0)
    {
        status_lbl = get_text(MCfgReleasesStatusRunning);
        next_sync_time_lbl = get_vtext(MCfgReleasesStatusLeft, items_left);
    }
    else
    {
        status_lbl = get_text(MCfgReleasesStatusFinished);

        if (next_sync_time > utils::clock_t::now())
        {
            auto time = utils::clock_t::to_time_t(next_sync_time);
            next_sync_time_lbl = utils::format_localtime(time, L"%d %b, %H:%M");
        }
        else
        {
            next_sync_time_lbl = L"---------";
        }
    }

    dialogs::set_text(hdlg, releases_status_value, status_lbl.c_str());
    dialogs::set_text(hdlg, releases_next_sync_value, next_sync_time_lbl.c_str());

    dialogs::enable(hdlg, releases_resync_button, is_resync_button_enabled);
}

static void update_releases_scan_block(HANDLE hdlg, plugin_ptr_t plugin)
{
    no_redraw_caches nr(hdlg);

    static wstring status;

    dialogs::set_text(hdlg, releases_separator, get_text(MCfgReleases));
    dialogs::set_text(hdlg, releases_status_label, get_text(MCfgReleasesSyncStatus));
    dialogs::set_text(hdlg, releases_status_value, get_text(MCfgReleasesStatusFinished));
    dialogs::set_text(hdlg, releases_next_sync_lbl, get_text(MCfgReleasesNext));
    dialogs::set_text(hdlg, releases_next_sync_value, L"------");
    dialogs::set_text(hdlg, releases_resync_button, get_text(MCfgReleasesResyncBtn));

    set_releases_sync_status(hdlg);
}

static bool remove_http_cache()
{
    const auto &filepath = spotify::get_cache_filename();
    
    std::error_code ec;
    if (!fs::remove(filepath, ec))
    {
        show_far_error_dlg(MFarMessageWarningTitle,
            get_vtext(MErrorRemoveFile, filepath.c_str(), utils::to_wstring(ec.message())));
        return false;
    }

    return true;
}

static void clear_credentials()
{
    auto ctx = config::lock_settings();
    ctx->delete_value(spotify::auth_config_key);
    ctx->delete_value(spotify::auth_config_key + L"Time");

    if (auto plugin = get_plugin())
    {
        if (auto api = plugin->get_api())
            api->get_auth_cache()->clear_credentials();
    }
}

caches_dialog::caches_dialog():
    modal_dialog(&guids::ConfigCachesDialogGuid, width, height, dlg_items_layout, L"ConfigCaches")
{
    utils::events::start_listening<spotify::releases_observer>(this);
}

caches_dialog::~caches_dialog()
{
    utils::events::stop_listening<spotify::releases_observer>(this);
}

void caches_dialog::init()
{
    update_logs_block(hdlg);
    update_caches_block(hdlg);
    update_releases_scan_block(hdlg, get_plugin());

    dialogs::set_text(hdlg, ok_button, get_text(MOk));
    dialogs::set_text(hdlg, cancel_button, get_text(MCancel));
}

bool caches_dialog::handle_btn_clicked(int ctrl_id, std::uintptr_t param)
{
    switch (ctrl_id)
    {
        case logs_show_button:
        {
            panels::set_directory(PANEL_PASSIVE, log::get_logs_folder());
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
            update_logs_block(hdlg);
            return true;
        }
        case auth_clear_button:
        {
            clear_credentials();
            update_caches_block(hdlg);
            return true;
        }
        case http_clear_button:
        {
            remove_http_cache();
            update_caches_block(hdlg);
            return true;
        }
        case clear_all_button:
        {
            clear_credentials();
            remove_http_cache();
            update_caches_block(hdlg);
            return true;
        }
        case releases_resync_button:
        {
            if (auto plugin = get_plugin())
            {
                if (auto api = plugin->get_api())
                    api->get_releases()->invalidate();
            }
            else
            {
                log::global->warn("No plugin is launched, while received a request to "
                    "resync releases cache");
            }
            return true;
        }
    }
    return false;
}

void caches_dialog::on_sync_progress_changed(size_t items_left)
{
    set_releases_sync_status(hdlg);
}

} // namespace settings
} // namespace ui
} // namespace spotifar