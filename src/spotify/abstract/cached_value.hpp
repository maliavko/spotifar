#ifndef CACHED_VALUE_HPP_BB3452A8_BAEA_45C5_97A9_14A76DC55BE5
#define CACHED_VALUE_HPP_BB3452A8_BAEA_45C5_97A9_14A76DC55BE5
#pragma once

#include "stdafx.h"
#include "interfaces.hpp"
#include "utils.hpp"

namespace spotifar
{
    namespace spotify
    {
        using namespace std::literals;
        using clock = utils::clock;
        using time_point = clock::time_point;
        using json = nlohmann::json;
        
        template<typename T>
        class CachedValue: public ICachedValue
        {
        public:
            inline static auto PATCH_EXPIRY_TIME = 1500ms;

        public:
            CachedValue(const std::wstring &storage_key, bool is_enabled = true);
            virtual ~CachedValue();

            virtual void read(SettingsCtx &ctx);
            virtual void write(SettingsCtx &ctx);
            virtual void clear(SettingsCtx &ctx);
            virtual bool resync(bool force = false);

            void set_enabled(bool is_enabled) { this->is_enabled = is_enabled; }
            const T& get_data() const { return data; }
            const time_point& get_last_sync_time() const { return last_sync_time; }
            const time_point get_expires_at() const { return last_sync_time + get_sync_interval(); }
            bool is_valid() const { return get_expires_at() > clock::now(); }
            void patch_data(const json &patch);

        protected:
            virtual bool request_data(T &data) = 0;
            virtual utils::ms get_sync_interval() const = 0;
            virtual void on_data_synced(const T &data, const T &prev_data) {}
            virtual void on_data_patched(T &data) {}
            void reset_data(const T &new_data);
            json& apply_patches(json &j);

        private:
            std::mutex patch_mutex;
            const std::wstring storage_data_key, storage_timestamp_key;
            time_point last_sync_time;
            bool is_enabled;
            std::vector<std::pair<time_point, json>> patches;
            T data;
        };
        
        template<typename T>
        CachedValue<T>::CachedValue(const std::wstring &storage_key, bool is_enabled):
            last_sync_time{},
            storage_data_key(storage_key),
            storage_timestamp_key(storage_key + L"Time"),
            is_enabled(is_enabled)
        {
        }

        template<typename T>
        CachedValue<T>::~CachedValue()
        {
        }

        template<typename T>
        void CachedValue<T>::read(SettingsCtx &ctx)
        {
            auto storage_value = ctx.get_str(storage_data_key, "");
            auto storage_timestamp = ctx.get_int64(storage_timestamp_key, 0LL);

            if (storage_value.empty())
                return;

            try
            {
                json::parse(storage_value).get_to(data);
                last_sync_time = clock::time_point{ clock::duration(storage_timestamp) };

                if (is_valid())
                    on_data_synced(data, data);
            }
            catch(const std::exception &e)
            {
                // in case of an error, just discard a stored data and drop an error message to log
                spdlog::error("Cached value \"{}\" is broken, discarding. {}",
                    utils::to_string(storage_data_key), e.what());
                clear(ctx);
            }
        }

        template<typename T>
        void CachedValue<T>::write(SettingsCtx &ctx)
        {
            ctx.set_str(storage_data_key, json(data).dump());
            ctx.set_int64(storage_timestamp_key, last_sync_time.time_since_epoch().count());
        }

        template<typename T>
        void CachedValue<T>::clear(SettingsCtx &ctx)
        {
            ctx.delete_value(storage_data_key);
            ctx.delete_value(storage_timestamp_key);
        }

        template<typename T>
        bool CachedValue<T>::resync(bool force)
        {
            auto sync_time = clock::now();
            // no updates for disabled caches, otherwise only in case the data
            // is invalid or resync is forced
            if (!is_enabled || (!force && is_valid()))
                return false;

            T new_data;
            if (request_data(new_data))
            {
                last_sync_time = sync_time;
                reset_data(new_data);
            }
            return true;
        }

        template<typename T>
        void CachedValue<T>::reset_data(const T &new_data)
        {
            auto old_data = data;
            data = new_data;
            on_data_synced(new_data, old_data);
        }

        template<typename T>
        void CachedValue<T>::patch_data(const json &patch)
        {
            // patches are saved and applied next time data is resynced
            std::lock_guard lock(patch_mutex);
            patches.push_back(std::make_pair(clock::now(), patch));
        }

        template<typename T>
        json& CachedValue<T>::apply_patches(json &j)
        {
            // removing outdated patches first
            auto now = clock::now();
            std::erase_if(patches, [&now](auto &v) { return v.first + PATCH_EXPIRY_TIME < now; });
            
            for (auto& [t, p]: patches)
            {
                j.merge_patch(p);
                // spdlog::get(utils::LOGGER_API)->debug("Applying data patch {}", p.dump());
            }
            return j;
        }
    }
}

#endif //CACHED_VALUE_HPP_BB3452A8_BAEA_45C5_97A9_14A76DC55BE5