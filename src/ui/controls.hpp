#ifndef CONTROLS_HPP_574B4993_6004_411B_89E4_E49A4A067D54
#define CONTROLS_HPP_574B4993_6004_411B_89E4_E49A4A067D54
#pragma once

#include "utils.hpp"

namespace spotifar { namespace ui {

using utils::clock_t;

/// @brief The class describes the behavior of a delayed changing value
/// @tparam T a holding value data type
template<typename T>
struct descriptor_abstract
{
    using value_t = T;

    /// @brief Sets the given value as a current one 
    virtual void set_value(const T &v) = 0;

    /// @brief Clears currently holding offset
    virtual void clear_offset() = 0;

    /// @brief Get the value without current offset
    virtual const T get_value() const = 0;

    /// @brief Get the value with current offset
    virtual const T get_offset_value() const = 0;

    /// @brief Increments offset 
    virtual const T next() = 0;

    /// @brief Decrement offset
    virtual const T prev() = 0;
    
    /// @brief Whether the descriptor holds changed offset and waiting for it to be applied
    bool is_waiting() const { return get_value() != get_offset_value(); };

    /// @brief Apply currently holding offset to the value, clears it and returns the result value
    value_t apply_offset()
    {
        set_value(get_offset_value());
        clear_offset();
        return get_value();
    }
};

/// @brief The descriptor implements logic of a slider control with
/// min and max values, stepping between them
struct slider_int_descriptor: public descriptor_abstract<int>
{
    int value, offset, step, high, low;

    slider_int_descriptor(int low, int high, int step):
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


/// @brief The descriptor implements logic of a control, which cycles between
/// predefined set of values indefinitely
/// @tparam T a type of a set values
template<typename T>
struct cycled_set_descriptor: public descriptor_abstract<T>
{
    const std::vector<T> values;
    size_t value_idx, offset_idx;

    cycled_set_descriptor(std::initializer_list<T> l): values(l)
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

/// @brief Encapsulates the logic of a value, which can be changed often within short
/// period of time, and to avoid spaming of the request to API, accumulates the value
/// and sends only one request after a short delay of no changes.
/// @tparam descriptor, describes the stored value type and how to work with it
template<typename T>
class delayed_control
{
public:
    using value_t = typename T::value_t;
    using delegate_t = std::function<void(value_t)>;

    inline static const auto delay = 300ms;

public:
    delayed_control(T descr): descr(descr) {}

    const value_t next(int steps = 1);
    const value_t prev(int steps = 1);

    bool is_waiting() const { return descr.is_waiting(); }
    void set_value(const value_t &v) { descr.set_value(v); }
    const value_t get_offset_value() const { return descr.get_offset_value(); }

    /// @brief Checks, whether the offset delay is expired and if so,
    /// applies the offset and calls the `delegate`
    bool check(delegate_t delegate);

protected:
    clock_t::time_point last_change_time{};
    T descr;
};

class slider_control: public delayed_control<slider_int_descriptor>
{
public:
    slider_control(int low, int high, int step): delayed_control({low, high, step}) {}
    int get_higher_boundary() const { return descr.high; }
    void set_higher_boundary(int high) { descr.high = high; }
};

using cycled_bool_control = delayed_control<cycled_set_descriptor<bool>>;
using cycled_string_control = delayed_control<cycled_set_descriptor<string>>;

template<typename T>
auto delayed_control<T>::next(int steps) -> const delayed_control<T>::value_t
{
    last_change_time = clock_t::now();

    while (steps-- > 1)
        descr.next();
    
    return descr.next();
}

template<typename T>
auto delayed_control<T>::prev(int steps) -> const delayed_control<T>::value_t
{
    last_change_time = clock_t::now();

    while (steps-- > 1)
        descr.prev();
    
    return descr.prev();
}

template<typename T>
bool delayed_control<T>::check(delayed_control<T>::delegate_t delegate)
{
    if (descr.is_waiting())
    {
        // if there is an accumulated volume value offset and the last changed of it
        // was more than a threshold, so we apply it
        if (last_change_time + delay < clock_t::now())
        {
            delegate(descr.apply_offset());
            return true;
        }
    }
    return false;
}

} // namespace ui
} // namespace spotifar

#endif // CONTROLS_HPP_574B4993_6004_411B_89E4_E49A4A067D54