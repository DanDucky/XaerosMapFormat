#pragma once

#include <variant>
#include <tag_compound.h>

#include "RegionImage.hpp"

namespace xaero {
    struct Region {
        struct TileChunk {
            struct Chunk {
                struct Pixel {
                    bool isNotGrass;
                    bool hasOverlays;
                    bool newStatePalette;
                    bool newBiomePalette;
                    bool biomeAsInt;
                    std::uint8_t colorType; // todo make enum with descriptive names
                    std::uint8_t light;
                    int height;
                    bool hasBiome;
                    bool topHeightAndHeightDontMatch;
                    bool hasSlope;

                    //not a "param" but should be fetched in same context
                    RegionImage::Pixel customColor;
                    int topHeight;
                    int biome;
                    std::variant<int32_t, nbt::tag_compound> state; // keeping support for ids because I hate nbt
                    int numberOfOverlays;
                    std::uint8_t version;
                    struct Overlay {
                        bool isNotWater;
                        bool hasOpacity;
                        bool legacyHasOpacity;
                        bool hasCustomColor;
                        int light;
                        int savedColorType;

                        // not a "overlay" but should be fetched in same context
                        int state;
                        RegionImage::Pixel customColor;
                        int opacity;

                    }* overlays = nullptr;

                    void allocateOverlays();

                    ~Pixel();
                } (*columns)[16][16] = nullptr;

                bool isVoid = true;

                [[nodiscard]] Pixel* operator[] (int x);
                [[nodiscard]] const Pixel* operator[] (int x) const;

                void allocateColumns();

                ~Chunk();
            } (*chunks)[4][4] = nullptr;

            [[nodiscard]] Chunk* operator[] (int x);
            [[nodiscard]] const Chunk* operator[] (int x) const;

            void allocateChunks();

            ~TileChunk();
        } tileChunks[8][8];

        [[nodiscard]] TileChunk* operator[] (int x);
        [[nodiscard]] const TileChunk* operator[] (int x) const;

    };
}
