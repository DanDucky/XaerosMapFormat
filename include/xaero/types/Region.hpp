#pragma once

#include <optional>
#include <variant>
#include <tag_compound.h>
#include <vector>
#include <memory>

#include "BlockState.hpp"
#include "RegionImage.hpp"

namespace xaero {
    struct Region {
        struct TileChunk {
            struct Chunk {
                Chunk(const Chunk &other);
                Chunk(Chunk &&other) noexcept;
                Chunk & operator=(const Chunk &other);
                Chunk & operator=(Chunk &&other) noexcept;

                struct Pixel {
                    std::uint8_t light;
                    std::int16_t height;
                    std::optional<std::uint8_t> topHeight;

                    // the rationale for this being optional instead of having a std::monostate is that xaero "officially" might not have a biome, but it ALWAYS has a state

                    std::optional<std::variant<std::shared_ptr<std::string>, std::string, std::string_view>> biome; // biome ids are so unsupported that I can't even add them here :(
                    std::variant<std::monostate, BlockState, std::shared_ptr<BlockState>, const BlockState*> state;

                    struct Overlay {
                        std::uint8_t light;
                        std::optional<std::int32_t> opacity;
                        std::variant<std::monostate, BlockState, std::shared_ptr<BlockState>, const BlockState*> state;
                    };
                    std::vector<Overlay> overlays;

                    [[nodiscard]] bool hasOverlays() const;
                };

                Pixel (*columns)[16][16] = nullptr;

                std::int32_t caveStart=0;
                std::int8_t caveDepth=0;

                // will set a default value
                std::int8_t chunkInterpretationVersion;

                [[nodiscard]] Pixel* operator[] (int x);
                [[nodiscard]] const Pixel* operator[] (int x) const;
                [[nodiscard]] bool isPopulated() const;
                [[nodiscard]] explicit operator bool() const;

                void allocateColumns();
                void deallocateColumns() noexcept;

                Chunk();

                ~Chunk();
            };
            Chunk (*chunks)[4][4] = nullptr;

            [[nodiscard]] Chunk* operator[] (int x);
            [[nodiscard]] const Chunk* operator[] (int x) const;
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
        TileChunk tileChunks[8][8];

        [[nodiscard]] TileChunk* operator[] (int x);
        [[nodiscard]] const TileChunk* operator[] (int x) const;

        [[nodiscard]] const TileChunk::Chunk::Pixel* operator[](std::uint16_t relX, std::uint16_t relZ) const;
        [[nodiscard]] TileChunk::Chunk::Pixel* operator[](std::uint16_t relX, std::uint16_t relZ);

        [[nodiscard]] bool hasChunk(std::uint8_t relX, std::uint8_t relZ) const;

        void mergeMove(Region& other);
        void mergeCopy(const Region& other);

        Region();

        // will not obey if saving region, will upgrade data to latest xaero
        std::uint16_t majorVersion;
        // will not obey if saving region, will upgrade data to latest xaero
        std::uint16_t minorVersion;
    };
}
