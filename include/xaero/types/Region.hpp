#pragma once

#include <generator>
#include <array>

#include "Overlay.hpp"
#include "TileChunk.hpp"

namespace xaero {
    struct Region {
        std::array<std::array<TileChunk, 8>, 8> tileChunks;

        [[nodiscard]] std::array<TileChunk, 8>& operator[] (std::uint8_t x);
        [[nodiscard]] const std::array<TileChunk, 8>& operator[] (std::uint8_t x) const;

        [[nodiscard]] const Pixel* operator[](std::uint16_t relX, std::uint16_t relZ) const;
        [[nodiscard]] Pixel* operator[](std::uint16_t relX, std::uint16_t relZ);

        [[nodiscard]] bool hasChunk(std::uint8_t relX, std::uint8_t relZ) const;

        void mergeMove(Region& other);
        void mergeCopy(const Region& other);

        [[nodiscard]] std::generator<const Pixel&> everyPixel() const;
        [[nodiscard]] std::generator<Pixel&> everyPixel();

        Region();

        // will not obey if saving region, will upgrade data to latest xaero
        std::uint16_t majorVersion;
        // will not obey if saving region, will upgrade data to latest xaero
        std::uint16_t minorVersion;
    };
}
