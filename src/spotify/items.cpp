#include "items.hpp" 

namespace spotifar
{
	namespace spotify
	{
		bool operator==(const Device& lhs, const Device& rhs)
		{
			return lhs.id == rhs.id;
		}
	}
}