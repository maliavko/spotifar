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
        
        template<class T>
        struct DelayedValueDescriptor
        {
            typedef T ValueType;

            virtual void set_value(const T &v) = 0;
            virtual void clear_offset() = 0;
            virtual const T get_value() const = 0;
            virtual const T get_offset_value() const = 0;
            virtual const T next() = 0;
            virtual const T prev() = 0;
            
            bool is_waiting() const { return get_value() != get_offset_value(); };

            ValueType apply_offset()
            {
                set_value(get_offset_value());
                clear_offset();
                return get_value();
            }
        };
        
        // encapsulates the logic of a value, which can be changed often within short
        // period of time, and to avoid spaming of the request to API, accumulates the value
        // and sends only one request after a short delay of no changes. A DescrType
        // describes the stored value type and how to work with it
        template<class DescrType>
        class DelayedValue
        {
        public:
            typedef typename DescrType::ValueType ValueType;
            typedef std::function<void(ValueType)> DelegateType;

            inline static const utils::ms DELAYED_THRESHOLD = 300ms;

        public:
            DelayedValue(DescrType descr): descr(descr) {}

            const ValueType next(int steps = 1);
            const ValueType prev(int steps = 1);

            bool is_waiting() const { return descr.is_waiting(); }
            void set_value(const ValueType &v) { descr.set_value(v); }
            const ValueType get_offset_value() const { return descr.get_offset_value(); }

            bool check(DelegateType delegate);

        protected:
            clock::time_point last_change_time{};
            DescrType descr;
        };

        struct SliderIntDescr: public DelayedValueDescriptor<int>
        {
            int value, offset, step, high, low;

            SliderIntDescr(int low, int high, int step):
                value(0), offset(0), step(step), low(low), high(high)
                {}

            virtual const int get_value() const { return value; }
            virtual const int get_offset_value() const { return value + offset; }
            virtual void clear_offset() { offset = 0; }

            virtual const int next() { return set_offset_value(step); }
            virtual const int prev() { return set_offset_value(-step); }
            
            virtual void set_value(const int &v) { value = v; }
            virtual const int set_offset_value(const int &s);
        };

        template<class T>
        struct CycledSetDescr: public DelayedValueDescriptor<T>
        {
            const std::vector<T> values;
            size_t value_idx, offset_idx;

            CycledSetDescr(std::initializer_list<T> l):
                values(l)
            {
                value_idx = offset_idx = 0;
            }

            virtual const T get_value() const { return values.at(value_idx); }
            virtual const T get_offset_value() const { return values.at(offset_idx); }
            
            virtual void clear_offset() { offset_idx = value_idx; }
            virtual void set_value(const T &v)
            {
                for (int idx = 0; idx < values.size(); idx++)
                    if (values[idx] == v)
                        // if there is offset waiting to be applied, we do not changed it;
                        // otherwise we change both values: offset and value, to avoid creating
                        // an offset
                        if (value_idx == offset_idx)
                            value_idx = offset_idx = idx;
                        else
                            value_idx = idx;
            }

            virtual const T next()
            {
                if (++offset_idx == values.size())
                    offset_idx = 0;
                return values.at(offset_idx);
            }

            virtual const T prev()
            {
                if (offset_idx == 0)
                    offset_idx = values.size();
                return values.at(--offset_idx);
            }
        };

        class SliderValue: public DelayedValue<SliderIntDescr>
        {
        public:
            SliderValue(int low, int high, int step):
                DelayedValue({low, high, step})
                {}

            int get_higher_boundary() const { return descr.high; }
            void set_higher_boundary(int high) { descr.high = high; }
        };

        typedef DelayedValue<CycledSetDescr<bool>> CycledBoolValue;
        typedef DelayedValue<CycledSetDescr<std::string>> CycledStringValue;
        
        template<class DescrType>
        auto DelayedValue<DescrType>::next(int steps) -> const DelayedValue<DescrType>::ValueType
        {
            last_change_time = clock::now();

            while (steps-- > 1)
                descr.next();
            
            return descr.next();
        }
        
        template<class DescrType>
        auto DelayedValue<DescrType>::prev(int steps) -> const DelayedValue<DescrType>::ValueType
        {
            last_change_time = clock::now();

            while (steps-- > 1)
                descr.prev();
            
            return descr.prev();
        }
        
        template<class DescrType>
        bool DelayedValue<DescrType>::check(DelayedValue<DescrType>::DelegateType delegate)
        {
            if (descr.is_waiting())
            {
                // if there is an accumulated volume value offset and the last changed of it
                // was more than a threshold, so we apply it
                if (last_change_time + DELAYED_THRESHOLD < clock::now())
                {
                    delegate(descr.apply_offset());
                    return true;
                }
            }
            return false;
        }
    }
}

#endif // COMPONENTS_HPP_574B4993_6004_411B_89E4_E49A4A067D54