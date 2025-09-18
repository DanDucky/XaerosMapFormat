#include "Endianness.hpp"

#include <cstdint>

static std::endian getEndianness() {
    if constexpr (std::endian::native == std::endian::little) {
        return std::endian::little;
    } else if constexpr (std::endian::native == std::endian::big) {
        return std::endian::big;
    } else {
        // test endianness at runtime
        union {
            std::uint16_t value = 1;
            std::uint8_t array[2];
        };
        if (array[0] == 1) {
            return std::endian::little;
        } else {
            return std::endian::big;
        }
    }
}

const std::endian xaero::endianness = getEndianness();
