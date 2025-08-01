#pragma once

#include <cstdint>

namespace xaero {
    /**
     * should be able to be directly fed to stb_image and written
     */
    struct RegionImage {
        struct Pixel {
            std::uint8_t red = 0;
            std::uint8_t green = 0;
            std::uint8_t blue = 0;
            std::uint8_t alpha = 0;
        };

        Pixel image[512][512];

        [[nodiscard]] const Pixel* operator[] (const int x) const;
        [[nodiscard]] Pixel* operator[] (const int x);
    };
}
