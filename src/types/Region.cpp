#include "xaero/types/Region.hpp"
#include "lookups/LegacyCompatibility.hpp"

#include <utility>

std::array<xaero::TileChunk, 8>& xaero::Region::operator[](const std::uint8_t x) {
    return tileChunks[x];
}

const std::array<xaero::TileChunk, 8>& xaero::Region::operator[](const std::uint8_t x) const {
    return tileChunks[x];
}

const xaero::Pixel* xaero::Region::operator[
](const std::uint16_t relX, const std::uint16_t relZ) const {
    if (relX >= 512 || relZ >= 512) return nullptr; // out of bounds!

    // can be shifted instead of using modulo, should be faster :smirkcat:

    const auto& tileChunk = tileChunks[relX >> 6][relZ >> 6];

    if (!tileChunk.isPopulated()) return nullptr;

    const auto& chunk = tileChunk[relX >> 4 & 3][relZ >> 4 & 3];

    if (!chunk.isPopulated()) return nullptr;

    return &chunk[relX & 15][relZ & 15];
}

xaero::Pixel * xaero::Region::operator[](const std::uint16_t relX, const std::uint16_t relZ) {
    if (relX >= 512 || relZ >= 512) return nullptr;

    auto& tileChunk = tileChunks[relX >> 6][relZ >> 6];

    if (!tileChunk.isPopulated()) return nullptr;

    auto& chunk = tileChunk[relX >> 4 & 3][relZ >> 4 & 3];

    if (!chunk.isPopulated()) return nullptr;

    return &chunk[relX & 15][relZ & 15];
}

bool xaero::Region::hasChunk(const std::uint8_t relX, const std::uint8_t relZ) const {
    if (relX >= 32 || relZ >= 32) return false;

    const auto& tileChunk = tileChunks[relX >> 6][relZ >> 6];

    if (!tileChunk.isPopulated()) return false;

    const auto& chunk = tileChunk[relX >> 4 & 3][relZ >> 4 & 3];

    return chunk.isPopulated();
}

void xaero::Region::mergeMove(Region &other) {
    for (std::uint16_t tileChunkX = 0; tileChunkX < 8; tileChunkX++) {
        for (std::uint16_t tileChunkZ = 0; tileChunkZ < 8; tileChunkZ++) {
            if (!other[tileChunkX][tileChunkZ].isPopulated()) continue;
            for (std::uint16_t chunkX = 0; chunkX < 4; chunkX++) {
                for (std::uint16_t chunkZ = 0; chunkZ < 4; chunkZ++) {
                    Chunk& chunk = other[tileChunkX][tileChunkZ][chunkX][chunkZ];
                    if (!chunk.isPopulated()) continue;

                    auto& tileChunk = tileChunks[tileChunkX][tileChunkZ];

                    tileChunk.allocateChunks();

                    tileChunk[chunkX][chunkZ].deallocateColumns();

                    tileChunk[chunkX][chunkZ].pixels = chunk.pixels;
                    chunk.pixels = nullptr;
                }
            }
        }
    }
}

void xaero::Region::mergeCopy(const Region &other) {
    for (std::uint16_t tileChunkX = 0; tileChunkX < 8; tileChunkX++) {
        for (std::uint16_t tileChunkZ = 0; tileChunkZ < 8; tileChunkZ++) {
            if (!other[tileChunkX][tileChunkZ].isPopulated()) continue;
            for (std::uint16_t chunkX = 0; chunkX < 4; chunkX++) {
                for (std::uint16_t chunkZ = 0; chunkZ < 4; chunkZ++) {
                    const Chunk& chunk = other[tileChunkX][tileChunkZ][chunkX][chunkZ];
                    if (!chunk.isPopulated()) continue;

                    auto& tileChunk = tileChunks[tileChunkX][tileChunkZ];

                    tileChunk.allocateChunks();

                    auto& thisChunk = tileChunk[chunkX][chunkZ];

                    thisChunk.allocateColumns();

                    for (std::uint8_t x = 0; x < 16; x++) {
                        for (std::uint8_t z = 0; z < 16; z++) {
                            thisChunk[x][z] = chunk[x][z];
                        }
                    }
                }
            }
        }
    }
}

std::generator<const xaero::Pixel&> xaero::Region::everyPixel() const {
    for (std::uint16_t tileChunkX = 0; tileChunkX < 8; tileChunkX++) {
        for (std::uint16_t tileChunkZ = 0; tileChunkZ < 8; tileChunkZ++) {
            if (!tileChunks[tileChunkX][tileChunkZ].isPopulated()) continue;
            for (std::uint16_t chunkX = 0; chunkX < 4; chunkX++) {
                for (std::uint16_t chunkZ = 0; chunkZ < 4; chunkZ++) {
                    const Chunk& chunk = tileChunks[tileChunkX][tileChunkZ][chunkX][chunkZ];
                    if (!chunk.isPopulated()) continue;
                    for (std::uint8_t x = 0; x < 16; x++) {
                        for (std::uint8_t z = 0; z < 16; z++) {
                            co_yield chunk[x][z];
                        }
                    }
                }
            }
        }
    }
}

std::generator<xaero::Pixel&> xaero::Region::everyPixel() {
    for (std::uint16_t tileChunkX = 0; tileChunkX < 8; tileChunkX++) {
        for (std::uint16_t tileChunkZ = 0; tileChunkZ < 8; tileChunkZ++) {
            if (!tileChunks[tileChunkX][tileChunkZ].isPopulated()) continue;
            for (std::uint16_t chunkX = 0; chunkX < 4; chunkX++) {
                for (std::uint16_t chunkZ = 0; chunkZ < 4; chunkZ++) {
                    Chunk& chunk = tileChunks[tileChunkX][tileChunkZ][chunkX][chunkZ];
                    if (!chunk.isPopulated()) continue;
                    for (std::uint8_t x = 0; x < 16; x++) {
                        for (std::uint8_t z = 0; z < 16; z++) {
                            co_yield chunk[x][z];
                        }
                    }
                }
            }
        }
    }
}

xaero::Region::Region() : majorVersion(XAERO_REGION_VERSION_MAJOR),
                          minorVersion(XAERO_REGION_VERSION_MINOR) {
}
