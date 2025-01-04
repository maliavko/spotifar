#include "components.hpp"

namespace spotifar
{
    namespace ui
    {
        const int SliderIntDescr::set_offset_value(const int &s)
        {
            if (offset * s < 0)
                clear_offset();

            offset += s;
            
            if (value + offset < low)
                offset = low - value;

            if (value + offset > high)
                offset = high - value;

            return get_offset_value();
        }
    }
}