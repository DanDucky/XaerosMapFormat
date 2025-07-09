#pragma once
#include <concepts>

namespace xaero {
    namespace math {
        template<std::integral T>
        unsigned int addWithRollover(T& value, const int increment, const T lower, const T upper);

        template<std::integral T>
        unsigned int addWithRollover(T &value, const int increment, const T lower, const T upper) {
            const T range = upper - lower;
            const int relative = value - lower + increment;

            // Compute number of rollovers
            int rollovers = static_cast<int>(relative / range);
            if (relative < 0 && relative % range != 0)
                --rollovers;

            // Wrap the value
            T offset = relative % range;
            if (offset < 0) offset += range;

            value = lower + offset;
            return rollovers;
        }
    }
}