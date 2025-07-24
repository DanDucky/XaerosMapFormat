#pragma once

#include <optional>
#include <variant>
#include <tag_compound.h>
#include <vector>
#include <memory>

#include "RegionImage.hpp"

namespace xaero {
    struct Region {
        struct TileChunk {
            struct Chunk {
                struct Pixel {
                    using BlockState = std::pair<std::string, nbt::tag_compound>;

                    std::uint8_t light;
                    std::optional<std::uint8_t> slope;
                    int16_t height;
                    int16_t topHeight;

                    std::variant<std::shared_ptr<std::string>, std::string, std::string_view> biome; // biome ids are so unsupported that I can't even add them here :(
                    std::variant<int32_t /* state id */, BlockState, std::shared_ptr<BlockState>, const BlockState* /* external state management */> state; // keeping support for ids because I hate nbt

                    int numberOfOverlays;
                    struct Overlay {
                        std::uint8_t light;
                        std::int32_t opacity;
                        std::variant<int32_t /* state id */, BlockState, std::shared_ptr<BlockState>, const BlockState* /* external state management */> state; // keeping support for ids because I hate nbt
                    };
                    std::vector<Overlay> overlays;

                    [[nodiscard]] bool hasOverlays() const;
                };
                Pixel (*columns)[16][16] = nullptr;

                [[nodiscard]] Pixel* operator[] (int x);
                [[nodiscard]] const Pixel* operator[] (int x) const;
                [[nodiscard]] bool isPopulated() const;
                [[nodiscard]] explicit operator bool() const;

                void allocateColumns();

                ~Chunk();
            };
            Chunk (*chunks)[4][4] = nullptr;

            [[nodiscard]] Chunk* operator[] (int x);
            [[nodiscard]] const Chunk* operator[] (int x) const;
            [[nodiscard]] bool isPopulated() const;
            [[nodiscard]] explicit operator bool() const;

            void allocateChunks();

            ~TileChunk();
        };
        TileChunk tileChunks[8][8];

        [[nodiscard]] TileChunk* operator[] (int x);
        [[nodiscard]] const TileChunk* operator[] (int x) const;

    };
}
