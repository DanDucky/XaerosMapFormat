#pragma once

#include <variant>
#include <tag_compound.h>
#include <vector>

#include "RegionImage.hpp"

namespace xaero {
    struct Region {
        struct TileChunk {
            struct Chunk {
                struct Pixel {
                    using BlockState = std::pair<const std::string, const nbt::tag_compound>;

                    std::uint8_t colorType; // todo make enum with descriptive names
                    std::uint8_t light;
                    int height;

                    //not a "param" but should be fetched in same context
                    RegionImage::Pixel customColor;
                    int topHeight;
                    int biome;
                    std::variant<int32_t /* state id */, BlockState, std::shared_ptr<const BlockState>, const BlockState* /* external state management */> state; // keeping support for ids because I hate nbt
                    int numberOfOverlays;
                    std::uint8_t version;
                    struct Overlay {
                        std::uint8_t light;
                        std::int32_t opacity;
                        std::variant<int32_t /* state id */, BlockState, std::shared_ptr<const BlockState>, const BlockState* /* external state management */> state; // keeping support for ids because I hate nbt
                    };
                    std::vector<Overlay> overlays;

                    [[nodiscard]] inline bool hasOverlays() const;
                };
                Pixel (*columns)[16][16] = nullptr;

                [[nodiscard]] Pixel* operator[] (int x);
                [[nodiscard]] const Pixel* operator[] (int x) const;
                [[nodiscard]] inline bool isPopulated() const;
                [[nodiscard]] explicit inline operator bool() const;

                void allocateColumns();

                ~Chunk();
            };
            Chunk (*chunks)[4][4] = nullptr;

            [[nodiscard]] Chunk* operator[] (int x);
            [[nodiscard]] const Chunk* operator[] (int x) const;
            [[nodiscard]] inline bool isPopulated() const;
            [[nodiscard]] explicit inline operator bool() const;

            void allocateChunks();

            ~TileChunk();
        };
        TileChunk tileChunks[8][8];

        [[nodiscard]] TileChunk* operator[] (int x);
        [[nodiscard]] const TileChunk* operator[] (int x) const;

    };
}
