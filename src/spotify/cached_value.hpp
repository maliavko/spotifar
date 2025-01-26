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
        using utils::far3::StorageValue;
        using SettingsCtx = config::SettingsContext;

        template<class JsonItemType>
        class JsonStorageValue: public StorageValue<JsonItemType>
        {
        public:
            using StorageValue<JsonItemType>::StorageValue; // default ctor

        protected:
            virtual void read_from_settings(SettingsCtx &ctx, const std::wstring &key, JsonItemType &data)
            {
                auto storage_value = ctx.get_str(key, "");
                if (!storage_value.empty())
                    json::parse(storage_value).get_to(data);
            }

            virtual void write_to_settings(SettingsCtx &ctx, const std::wstring &key, JsonItemType &data)
            {
                ctx.set_str(key, json(data).dump());
            }
        };
        
        class TimestampStorageValue: public StorageValue<time_point>
        {
        public:
            using StorageValue<time_point>::StorageValue; // default ctor

        protected:
            virtual void read_from_settings(SettingsCtx &ctx, const std::wstring &key, time_point &data)
            {
                auto storage_timestamp = ctx.get_int64(key, 0LL);
                data = time_point{ clock::duration(storage_timestamp) };
            }

            virtual void write_to_settings(SettingsCtx &ctx, const std::wstring &key, time_point &data)
            {
                ctx.set_int64(key, data.time_since_epoch().count());
            }
        };
        
        class StringStorageValue: public StorageValue<std::string>
        {
        public:
            using StorageValue<std::string>::StorageValue; // default ctor

        protected:
            virtual void read_from_settings(SettingsCtx &ctx, const wstring &key, string &data)
            {
                data = ctx.get_str(key, "");
            }

            virtual void write_to_settings(SettingsCtx &ctx, const wstring &key, string &data)
            {
                ctx.set_str(key, data);
            }
        };
        
        template<class KeyT, class ValueT>
        class MapStorageValue: public StorageValue<std::unordered_map<KeyT, ValueT>>
        {
        public:
            using StorageValue<std::unordered_map<KeyT, ValueT>>::StorageValue; // default ctor

        protected:
            virtual void read_from_settings(SettingsCtx &ctx, const wstring &key, std::unordered_map<KeyT, ValueT> &data)
            {
                auto storage_value = ctx.get_str(key, "");
                if (!storage_value.empty())
                {
                    json j = json::parse(storage_value);
                    for (const auto &[k, v]: j.items())
                        data[k] = v;
                }
            }

            virtual void write_to_settings(SettingsCtx &ctx, const std::wstring &key, std::unordered_map<KeyT, ValueT> &data)
            {
                json j;
                for (const auto &[k, v]: data)
                    j[k] = v;
                ctx.set_str(key, j.dump());
            }
        };

        template<class JsonItemType>
        class CachedItem: public ICachedData
        {
        public:
            inline static auto PATCH_EXPIRY_DELAY = 1500ms;

        public:
            CachedItem(const std::wstring &storage_key);

            // storable data interface
            virtual void read(SettingsCtx &ctx);
            virtual void write(SettingsCtx &ctx);
            virtual void clear(SettingsCtx &ctx);

            // cached data interface
            virtual void resync(bool force = false);

            void patch(const json &patch);

            const JsonItemType& get() const { return data.get(); }
            const time_point& get_last_sync_time() const { return last_sync_time.get(); }
            const time_point get_expires_at() const { return get_last_sync_time() + get_sync_interval(); }
            bool is_valid() const { return get_expires_at() > clock::now(); }
            void invalidate() { last_sync_time.set(time_point{}); }

        protected:
            virtual bool request_data(JsonItemType &json_data) = 0;
            virtual utils::ms get_sync_interval() const = 0;
            void apply_patches(JsonItemType &item);

            virtual void on_data_synced(const JsonItemType &data, const JsonItemType &prev_data) {}
            virtual void on_data_patched(JsonItemType &data) {}

        private:
            JsonStorageValue<JsonItemType> data;
            TimestampStorageValue last_sync_time;

            std::mutex patch_mutex;
            std::vector<std::pair<time_point, json>> patches;
        };
        
        template<typename T>
        CachedItem<T>::CachedItem(const std::wstring &storage_key):
            data(storage_key),
            last_sync_time(storage_key + L"Time")
        {
        }

        template<typename T>
        void CachedItem<T>::read(SettingsCtx &ctx)
        {
            data.read(ctx);
            last_sync_time.read(ctx);
        
            // if the data is still valid, we send notification as
            // it was resynced well from server
            if (is_valid())
                on_data_synced(data.get(), data.get());
        }

        template<typename T>
        void CachedItem<T>::write(SettingsCtx &ctx)
        {
            data.write(ctx);
            last_sync_time.write(ctx);
        }

        template<typename T>
        void CachedItem<T>::clear(SettingsCtx &ctx)
        {
            data.clear(ctx);
            last_sync_time.clear(ctx);
        }

        template<typename T>
        void CachedItem<T>::resync(bool force)
        {
            auto sync_time = clock::now();
            // no updates for disabled caches, otherwise only in case the data
            // is invalid or resync is forced
            if (!is_enabled() || (!force && is_valid()))
                return;

            T new_data;
            if (request_data(new_data))
            {
                auto old_data = data.get();
                apply_patches(new_data);

                data.set(new_data);
                last_sync_time.set(sync_time);

                on_data_synced(new_data, old_data);
            }
        }

        template<typename T>
        void CachedItem<T>::patch(const json &patch)
        {
            // patches are saved and applied next time data is resynced
            std::lock_guard lock(patch_mutex);
            patches.push_back(std::make_pair(clock::now(), patch));

            // instead of calling "resync", we invalidate the cache, so it is updated in
            // a correct order from the right thread
            invalidate();
        }

        template<typename T>
        void CachedItem<T>::apply_patches(T &item)
        {
            // unpacking and packing the item here, definitely not the best solution,
            // did not come up with the better one still
            json j = item; 
            auto now = clock::now();

            // removing outdated patches first
            std::erase_if(patches,
                [&now](auto &v) {
                    return v.first + PATCH_EXPIRY_DELAY < now;
                });
            
            for (auto& [t, p]: patches)
            {
                // spdlog::get(utils::LOGGER_API)->debug("Applygin temp data patch, {}", p.dump());
                j.merge_patch(p);
            }
            j.get_to(item);
        }
    }
}

#endif //CACHED_VALUE_HPP_BB3452A8_BAEA_45C5_97A9_14A76DC55BE5