#pragma once
#include <array>

#include "Chunk.hpp"

namespace xaero {
    struct TileChunk {
        std::array<std::array<Chunk, 4>, 4>* chunks = nullptr;

        [[nodiscard]] std::array<Chunk, 4>& operator[] (int x);
        [[nodiscard]] const std::array<Chunk, 4>& operator[] (int x) const;
        [[nodiscard]] bool isPopulated() const;
        [[nodiscard]] explicit operator bool() const;

        void allocateChunks();
        void deallocateChunks() noexcept;

        TileChunk()=default;

        ~TileChunk();

        TileChunk(const TileChunk& other);
        TileChunk(TileChunk&& other) noexcept;
        TileChunk& operator=(const TileChunk& other);
        TileChunk& operator=(TileChunk&& other) noexcept;
    };
}
