#pragma once
#include <array>

#include "Pixel.hpp"

namespace xaero {
    struct Chunk {
        Chunk(const Chunk &other);
        Chunk(Chunk &&other) noexcept;
        Chunk & operator=(const Chunk &other);
        Chunk & operator=(Chunk &&other) noexcept;

        std::array<std::array<Pixel, 16>, 16> *pixels = nullptr;

        std::int32_t caveStart=0;
        std::int8_t caveDepth=0;

        // will set a default value
        std::int8_t chunkInterpretationVersion;

        [[nodiscard]] std::array<Pixel, 16>& operator[] (int x);
        [[nodiscard]] const std::array<Pixel, 16>& operator[] (int x) const;
        [[nodiscard]] bool isPopulated() const;

        void allocateColumns();
        void deallocateColumns() noexcept;

        Chunk();

        ~Chunk();
    };
}
