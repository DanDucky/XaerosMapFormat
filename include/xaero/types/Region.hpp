#pragma once

#include <optional>
#include <variant>
#include <generator>
#include <array>
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
                    std::optional<std::variant<std::shared_ptr<std::string>, std::string, std::string_view>> biome;

                    // std::monostate is default and "nullopt"
                    std::variant<std::monostate, BlockState, std::shared_ptr<BlockState>, const BlockState*> state;

                    struct Overlay {
                        std::uint8_t light;
                        std::optional<std::int32_t> opacity;
                        std::variant<std::monostate, BlockState, std::shared_ptr<BlockState>, const BlockState*> state;

                        [[nodiscard]] const BlockState& getState() const;
                    };
                    std::vector<Overlay> overlays;

                    [[nodiscard]] bool hasOverlays() const;

                    [[nodiscard]] const BlockState& getState() const;
                    [[nodiscard]] std::optional<std::string_view> getBiome() const;
                };

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
        std::array<std::array<TileChunk, 8>, 8> tileChunks;

        [[nodiscard]] std::array<TileChunk, 8>& operator[] (int x);
        [[nodiscard]] const std::array<TileChunk, 8>& operator[] (int x) const;

        [[nodiscard]] const TileChunk::Chunk::Pixel* operator[](std::uint16_t relX, std::uint16_t relZ) const;
        [[nodiscard]] TileChunk::Chunk::Pixel* operator[](std::uint16_t relX, std::uint16_t relZ);

        [[nodiscard]] bool hasChunk(std::uint8_t relX, std::uint8_t relZ) const;

        void mergeMove(Region& other);
        void mergeCopy(const Region& other);

        [[nodiscard]] std::generator<const TileChunk::Chunk::Pixel&> everyPixel() const;
        [[nodiscard]] std::generator<TileChunk::Chunk::Pixel&> everyPixel();

        Region();

        // will not obey if saving region, will upgrade data to latest xaero
        std::uint16_t majorVersion;
        // will not obey if saving region, will upgrade data to latest xaero
        std::uint16_t minorVersion;
    };
}
