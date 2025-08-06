#include "xaero/types/Region.hpp"

#include <utility>

xaero::Region::TileChunk::Chunk::Chunk(const Chunk &other) {
    caveDepth = other.caveDepth;
    caveStart = other.caveStart;
    chunkInterpretationVersion = other.chunkInterpretationVersion;

    if (!other.isPopulated()) {
        return;
    }

    allocateColumns();

    for (std::uint8_t x = 0; x < 16; x++) {
        for (std::uint8_t z = 0; z < 16; z++) {
            (*columns)[x][z] = (*other.columns)[x][z];
        }
    }
}

xaero::Region::TileChunk::Chunk::Chunk(Chunk &&other) noexcept {
    caveDepth = other.caveDepth;
    caveStart = other.caveStart;
    chunkInterpretationVersion = other.chunkInterpretationVersion;

    columns = std::exchange(other.columns, nullptr);
}

xaero::Region::TileChunk::Chunk & xaero::Region::TileChunk::Chunk::operator=(const Chunk &other) {
    if (this == &other) return *this;

    caveDepth = other.caveDepth;
    caveStart = other.caveStart;
    chunkInterpretationVersion = other.chunkInterpretationVersion;

    if (!other.isPopulated()) {
        deallocateColumns();
        return *this;
    }

    allocateColumns();

    for (std::uint8_t x = 0; x < 16; x++) {
        for (std::uint8_t z = 0; z < 16; z++) {
            (*columns)[x][z] = (*other.columns)[x][z];
        }
    }

    return *this;
}

xaero::Region::TileChunk::Chunk & xaero::Region::TileChunk::Chunk::operator=(Chunk &&other) noexcept {
    if (this == &other) return *this;

    deallocateColumns();

    caveDepth = other.caveDepth;
    caveStart = other.caveStart;
    chunkInterpretationVersion = other.chunkInterpretationVersion;

    columns = std::exchange(other.columns, nullptr);

    return *this;
}

bool xaero::Region::TileChunk::Chunk::Pixel::hasOverlays() const {
    return overlays.size() > 0;
}

xaero::Region::TileChunk::Chunk::Pixel * xaero::Region::TileChunk::Chunk::operator[](int x) {
    return (*columns)[x];
}

const xaero::Region::TileChunk::Chunk::Pixel * xaero::Region::TileChunk::Chunk::operator[](int x) const {
    return (*columns)[x];
}

bool xaero::Region::TileChunk::Chunk::isPopulated() const {
    return columns;
}

xaero::Region::TileChunk::Chunk::operator bool() const {
    return isPopulated();
}

void xaero::Region::TileChunk::Chunk::allocateColumns() {
    if (columns) return;
    columns = reinterpret_cast<Pixel(*)[16][16]>(new Pixel[16 * 16]);
}

void xaero::Region::TileChunk::Chunk::deallocateColumns() noexcept {
    delete[] reinterpret_cast<Pixel*>(columns);
    columns = nullptr;
}

xaero::Region::TileChunk::Chunk::Chunk() : chunkInterpretationVersion(1) {
}

xaero::Region::TileChunk::Chunk::~Chunk() {
    delete[] reinterpret_cast<Pixel*>(columns);
}

xaero::Region::TileChunk::Chunk * xaero::Region::TileChunk::operator[](int x) {
    return (*chunks)[x];
}

const xaero::Region::TileChunk::Chunk * xaero::Region::TileChunk::operator[](int x) const {
    return (*chunks)[x];
}

bool xaero::Region::TileChunk::isPopulated() const {
    return chunks;
}

xaero::Region::TileChunk::operator bool() const {
    return isPopulated();
}

void xaero::Region::TileChunk::allocateChunks() {
    if (chunks) return;
    chunks = reinterpret_cast<Chunk(*)[4][4]>(new Chunk[4 * 4]);
}

void xaero::Region::TileChunk::deallocateChunks() noexcept {
    delete[] reinterpret_cast<Chunk*>(chunks);
    chunks = nullptr;
}

xaero::Region::TileChunk::~TileChunk() {
    delete[] reinterpret_cast<Chunk*>(chunks);
}

xaero::Region::TileChunk::TileChunk(const TileChunk &other) {
    if (!other.isPopulated()) {
        return;
    }

    allocateChunks();

    for (std::uint8_t chunkX = 0; chunkX < 4; chunkX++) {
        for (std::uint8_t chunkZ = 0; chunkZ < 4; chunkZ++) {
            (*chunks)[chunkX][chunkZ] = (*other.chunks)[chunkX][chunkZ];
        }
    }
}

xaero::Region::TileChunk::TileChunk(TileChunk &&other) noexcept {
    if (!other.isPopulated()) return;

    chunks = std::exchange(other.chunks, nullptr);
}

xaero::Region::TileChunk & xaero::Region::TileChunk::operator=(const TileChunk &other) {
    if (this == &other) return *this;

    if (!other.isPopulated()) {
        deallocateChunks();
        return *this;
    }

    allocateChunks();

    for (std::uint8_t chunkX = 0; chunkX < 4; chunkX++) {
        for (std::uint8_t chunkZ = 0; chunkZ < 4; chunkZ++) {
            (*chunks)[chunkX][chunkZ] = (*other.chunks)[chunkX][chunkZ];
        }
    }

    return *this;
}

xaero::Region::TileChunk & xaero::Region::TileChunk::operator=(TileChunk &&other) noexcept {
    if (this == &other) return *this;

    deallocateChunks();

    chunks = std::exchange(other.chunks, nullptr);

    return *this;
}

xaero::Region::TileChunk * xaero::Region::operator[](const int x) {
    return tileChunks[x];
}

const xaero::Region::TileChunk * xaero::Region::operator[](const int x) const {
    return tileChunks[x];
}

const xaero::Region::TileChunk::Chunk::Pixel* xaero::Region::operator[
](const std::uint16_t relX, const std::uint16_t relZ) const {
    if (relX >= 521 || relZ >= 512) return nullptr; // out of bounds!

    // can be shifted instead of using modulo, should be faster :smirkcat:

    const auto& tileChunk = tileChunks[relX >> 6][relZ >> 6];

    if (!tileChunk.isPopulated()) return nullptr;

    const auto& chunk = tileChunk[relX >> 4 & 3][relZ >> 4 & 3];

    if (!chunk.isPopulated()) return nullptr;

    return &chunk[relX & 15][relZ & 15];
}

xaero::Region::TileChunk::Chunk::Pixel * xaero::Region::operator[](const std::uint16_t relX, const std::uint16_t relZ) {
    if (relX >= 512 || relZ >= 512) return nullptr;

    auto& tileChunk = tileChunks[relX >> 6][relZ >> 6];

    if (!tileChunk.isPopulated()) return nullptr;

    auto& chunk = tileChunk[relX >> 4 & 3][relZ >> 4 & 3];

    if (!chunk.isPopulated()) return nullptr;

    return &chunk[relX & 15][relZ & 15];
}

bool xaero::Region::hasPixel(const std::uint16_t relX, const std::uint16_t relZ) const {
    if (relX >= 512 || relZ >= 512) return false;

    const auto& tileChunk = tileChunks[relX >> 6][relZ >> 6];

    if (!tileChunk.isPopulated()) return false;

    const auto& chunk = tileChunk[relX >> 4 & 3][relZ >> 4 & 3];

    if (!chunk.isPopulated()) return false;

    return true;
}

void xaero::Region::mergeMove(Region &other) {
    for (std::uint16_t tileChunkX = 0; tileChunkX < 8; tileChunkX++) {
        for (std::uint16_t tileChunkZ = 0; tileChunkZ < 8; tileChunkZ++) {
            if (!other[tileChunkX][tileChunkZ].isPopulated()) continue;
            for (std::uint16_t chunkX = 0; chunkX < 4; chunkX++) {
                for (std::uint16_t chunkZ = 0; chunkZ < 4; chunkZ++) {
                    TileChunk::Chunk& chunk = other[tileChunkX][tileChunkZ][chunkX][chunkZ];
                    if (!chunk.isPopulated()) continue;

                    auto& tileChunk = tileChunks[tileChunkX][tileChunkZ];

                    tileChunk.allocateChunks();

                    tileChunk[chunkX][chunkZ].deallocateColumns();

                    tileChunk[chunkX][chunkZ].columns = chunk.columns;
                    chunk.columns = nullptr;
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
                    const TileChunk::Chunk& chunk = other[tileChunkX][tileChunkZ][chunkX][chunkZ];
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

xaero::Region::Region() : majorVersion(XAERO_REGION_VERSION_MAJOR),
                          minorVersion(XAERO_REGION_VERSION_MINOR) {
}
