#ifndef COMPONENTS_HPP_574B4993_6004_411B_89E4_E49A4A067D54
#define COMPONENTS_HPP_574B4993_6004_411B_89E4_E49A4A067D54
#pragma once

#include "utils.hpp"

namespace spotifar
{
    namespace ui
    {
        using utils::clock;
        using namespace std::literals;
        
        template<class DescrT>
        class DelayedValue
        {
        public:
            inline static const std::chrono::milliseconds DELAYED_THRESHOLD = 300ms;

        public:
            DescrT& get_descr() { return descr; }
            
            const typename DescrT::ValueType& next(int steps = 1)
            {
                last_change_time = clock::now();

                while (steps-- > 1)
                    descr.next();
                
                return descr.next();
            }

            const typename DescrT::ValueType& prev(int steps = 1)
            {
                last_change_time = clock::now();

                while (steps-- > 1)
                    descr.prev();
                
                return descr.prev();
            }

            void set_value(const typename DescrT::ValueType &v) { descr.set_value(v); }
            
            bool check(std::function<void(typename DescrT::ValueType)> delegate)
            {
                if (descr.is_waiting())
                {
                    auto now = clock::now();
                    // if there is an accumulated volume value offset and the last changed of it
                    // was more than a threshold, so we apply it
                    if (last_change_time + DELAYED_THRESHOLD < now)
                    {
                        auto new_value = descr.get_offset_value();

                        spdlog::debug("Setting a new value {}, a current value {}", new_value, descr.get_value());
                        descr.clear_offset();

                        // TODO: what if delegate finishes with error?
                        delegate(new_value);
                        return true;
                    }
                }
                return false;
            }
        private:
            DescrT descr;
            clock::time_point last_change_time{};
            std::mutex access_mutex{};
        };

        template<class ValueT>
        struct DelayedValueDescriptor
        {
            typedef ValueT ValueType;

            virtual const ValueT& get_value() const = 0;
            virtual const ValueT& get_offset_value() const = 0;
            virtual void set_value(const ValueT &v) = 0;
            virtual const ValueT& next() = 0;
            virtual const ValueT& prev() = 0;
            virtual void clear_offset() = 0;
            
            bool is_waiting() const { return get_value() != get_offset_value(); };
        };

        // encapsulates logic of a value, which can be changed often within short
        // period of time, and to avoid spam of the request to API, accumulates the value
        // and sends only one request after a short delay of no changes
        struct SliderIntDescr: public DelayedValueDescriptor<int>
        {
            int value, offset_value, step, high, low;

            SliderIntDescr(int low, int high, int step):
                value(0), offset_value(value), step(step), low(low), high(high)
                {}

            virtual const int& get_value() const { return value; }

            virtual const int& get_offset_value() const { return offset_value; }

            virtual void clear_offset() { offset_value = value; }

            virtual const int& next() { return set_offset_value(step); }

            virtual const int& prev() { return set_offset_value(-step); }
            
            virtual void set_value(const int &v)
            {
                if (offset_value == value)
                    offset_value = value = v;
                else
                    value = v;
            }

            virtual const int& set_offset_value(const int &s)
            {
                if ((offset_value - value) * s < 0)
                    clear_offset();

                offset_value += s;
                
                if (offset_value <= low)
                    offset_value = low;

                if (offset_value >= high)
                    offset_value = high;

                return offset_value;
            }
        };

        template<class T>
        struct CycledSetDescr: public DelayedValueDescriptor<T>
        {
            std::list<T> values;
            std::list<T>::iterator value_ptr, offset_ptr;

            CycledSetDescr(const std::initializer_list<T> &init):
                values(init)
            {
                value_ptr = offset_ptr = values.begin();
            }

            virtual const T& get_value() const { return *value_ptr; }

            virtual const T& get_offset_value() const { return *offset_ptr; }
            
            virtual void set_value(const T &v)
            {
                for (auto it = values.begin(); it != values.end(); it++)
                    if (*it == v)
                        value_ptr = it;
            }

            virtual void clear_offset() { offset_ptr = value_ptr; }

            virtual const T& next()
            {
                if (++offset_ptr == values.end())
                    offset_ptr = values.begin();
                return *offset_ptr;
            }

            virtual const T& prev()
            {
                if (offset_ptr == values.begin())
                    offset_ptr = values.end();
                return *--offset_ptr;
            }
        };
    }
}

#endif // COMPONENTS_HPP_574B4993_6004_411B_89E4_E49A4A067D54