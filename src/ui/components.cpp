#include "components.hpp"

namespace spotifar
{
    namespace ui
    { 
        void SliderIntDescr::set_value(const int &v)
        {
            // if there is no offset, so we change a value and an offset value;
            // in case there is an offset waiting to be applied, we do not touch it
            if (offset_value == value)
                offset_value = value = v;
            else
                value = v;
        }

        const int SliderIntDescr::set_offset_value(const int &s)
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
    }
}