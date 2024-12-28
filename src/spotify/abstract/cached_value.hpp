#ifndef CACHED_VALUE_HPP_BB3452A8_BAEA_45C5_97A9_14A76DC55BE5
#define CACHED_VALUE_HPP_BB3452A8_BAEA_45C5_97A9_14A76DC55BE5
#pragma once

#include "stdafx.h"

namespace spotifar
{
    namespace spotify
    {
        using clock = utils::clock;
        using time_point = clock::time_point;
        using json = nlohmann::json;
        using SettingsContext = config::SettingsContext;

        class ICachedValue
        {
        public:
            virtual ~ICachedValue() {};

            virtual void read(SettingsContext &ctx) = 0;
            virtual void write(SettingsContext &ctx) = 0;
            virtual void clear(SettingsContext &ctx) = 0;
            virtual bool resync(bool force = false) = 0;
            virtual void set_enabled(bool is_enabled) = 0;
        };
        
        template<typename T>
        class CachedValue: public ICachedValue
        {
        public:
            CachedValue(httplib::Client *endpoint, const std::wstring &storage_key,
                bool is_enabled = true);
            virtual ~CachedValue();

            virtual void read(SettingsContext &ctx);
            virtual void write(SettingsContext &ctx);
            virtual void clear(SettingsContext &ctx);
            virtual bool resync(bool force = false);

            void set_enabled(bool is_enabled) { this->is_enabled = is_enabled; }
            const T& get_data() const { return data; }
            const time_point& get_last_sync_time() const { return last_sync_time; }
            const time_point get_expires_at() const { return last_sync_time + get_sync_interval(); }
            bool is_valid() const { return get_expires_at() > clock::now(); }
            void patch_data(json patch);

        protected:
            virtual bool request_data(T &data) = 0;
            virtual std::chrono::milliseconds get_sync_interval() const = 0;
            virtual void on_data_synced(const T &data, const T &prev_data) {}
            virtual void on_data_patched(T &data) {}
            void reset_data(const T &new_data);

        protected:
            httplib::Client *endpoint;

        private:
            std::mutex access_mutext;
            const std::wstring storage_data_key, storage_timestamp_key;
            time_point last_sync_time;
            bool is_enabled;
            T data;
        };
        
        template<typename T>
        CachedValue<T>::CachedValue(httplib::Client* endpoint, const std::wstring &storage_key,
                                    bool is_enabled):
            endpoint(endpoint),
            last_sync_time{},
            storage_data_key(storage_key),
            storage_timestamp_key(storage_key + L"Time"),
            is_enabled(is_enabled)
        {
        }

        template<typename T>
        CachedValue<T>::~CachedValue()
        {
            endpoint = nullptr;
        }

        template<typename T>
        void CachedValue<T>::read(SettingsContext &ctx)
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
        void CachedValue<T>::write(SettingsContext &ctx)
        {
            ctx.set_str(storage_data_key, json(data).dump());
            ctx.set_int64(storage_timestamp_key, last_sync_time.time_since_epoch().count());
        }

        template<typename T>
        void CachedValue<T>::clear(SettingsContext &ctx)
        {
            ctx.delete_value(storage_data_key);
            ctx.delete_value(storage_timestamp_key);
        }

        template<typename T>
        bool CachedValue<T>::resync(bool force)
        {
            auto sync_time = clock::now();
            std::lock_guard lk(access_mutext);
            // no updates for disabled caches, otherwise only in case the data
            // is invalid or resync is forced
            if (!is_enabled || (!force && is_valid()))
                return false;

            T new_data;
            if (request_data(new_data))
            {
                reset_data(new_data);
                last_sync_time = sync_time;
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
        void CachedValue<T>::patch_data(json patch)
        {
            auto sync_time = clock::now();
            std::lock_guard lk(access_mutext);
            try
            {
                json j(data);
                j.merge_patch(patch);

                auto data = j.get<T>();
                on_data_patched(data);
                reset_data(data);
                last_sync_time = sync_time;
            }
            catch (const json::exception& ex)
            {
                spdlog::error("An error occured while patching cached: {}, patch {}",
                              ex.what(), patch.dump());
            }
        }
    }
}

#endif //CACHED_VALUE_HPP_BB3452A8_BAEA_45C5_97A9_14A76DC55BE5