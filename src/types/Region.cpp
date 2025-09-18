#include "xaero/types/Region.hpp"

#include "lookups/LegacyCompatibility.hpp"
#include "util/StringUtils.hpp"

#include <utility>

inline const xaero::BlockState* getStateForUnion(const std::variant<std::monostate, xaero::BlockState, std::shared_ptr<xaero::BlockState>, const xaero::BlockState*>& state) {
    if (std::holds_alternative<std::monostate>(state)) {
        return xaero::getStateFromID(0); // air
    }
    if (std::holds_alternative<const xaero::BlockState*>(state)) {
        return std::get<const xaero::BlockState*>(state);
    }
    if (std::holds_alternative<xaero::BlockState>(state)) {
        return &std::get<xaero::BlockState>(state);
    }
    return std::get<std::shared_ptr<xaero::BlockState>>(state).get();
}

inline std::string_view getBiomeForUnion(const std::variant<std::shared_ptr<std::string>, std::string, std::string_view>& biome) {
    std::string_view out;
    if (std::holds_alternative<std::shared_ptr<std::string>>(biome)) {
        out = *std::get<std::shared_ptr<std::string>>(biome);
    } else if (std::holds_alternative<std::string>(biome)) {
        out = std::get<std::string>(biome);
    } else if (std::holds_alternative<std::string_view>(biome)) {
        out = std::get<std::string_view>(biome);
    }

    return xaero::stripName(out);
}

xaero::Region::TileChunk::Chunk::Chunk(const Chunk &other) {
    caveDepth = other.caveDepth;
    caveStart = other.caveStart;
    chunkInterpretationVersion = other.chunkInterpretationVersion;

    if (!other.isPopulated()) {
        return;
    }

    allocateColumns();

    *pixels = *other.pixels;
}

xaero::Region::TileChunk::Chunk::Chunk(Chunk &&other) noexcept {
    caveDepth = other.caveDepth;
    caveStart = other.caveStart;
    chunkInterpretationVersion = other.chunkInterpretationVersion;

    pixels = std::exchange(other.pixels, nullptr);
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

    *pixels = *other.pixels;

    return *this;
}

xaero::Region::TileChunk::Chunk & xaero::Region::TileChunk::Chunk::operator=(Chunk &&other) noexcept {
    if (this == &other) return *this;

    deallocateColumns();

    caveDepth = other.caveDepth;
    caveStart = other.caveStart;
    chunkInterpretationVersion = other.chunkInterpretationVersion;

    pixels = std::exchange(other.pixels, nullptr);

    return *this;
}

const xaero::BlockState* xaero::Region::TileChunk::Chunk::Pixel::Overlay::getState() const {
    return getStateForUnion(state);
}

bool xaero::Region::TileChunk::Chunk::Pixel::hasOverlays() const {
    return overlays.size() > 0;
}

const xaero::BlockState* xaero::Region::TileChunk::Chunk::Pixel::getState() const {
    return getStateForUnion(state);
}

std::optional<std::string_view> xaero::Region::TileChunk::Chunk::Pixel::getBiome() const {
    if (!biome) return std::nullopt;
    return getBiomeForUnion(biome.value());
}

std::array<xaero::Region::TileChunk::Chunk::Pixel, 16>& xaero::Region::TileChunk::Chunk::operator[](const int x) {
    return (*pixels)[x];
}

const std::array<xaero::Region::TileChunk::Chunk::Pixel, 16>& xaero::Region::TileChunk::Chunk::operator[
](const int x) const {
    return (*pixels)[x];
}

bool xaero::Region::TileChunk::Chunk::isPopulated() const {
    return pixels;
}

void xaero::Region::TileChunk::Chunk::allocateColumns() {
    if (pixels) return;
    pixels = new std::array<std::array<Pixel, 16>, 16>();
}

void xaero::Region::TileChunk::Chunk::deallocateColumns() noexcept {
    delete[] reinterpret_cast<Pixel*>(pixels);
    pixels = nullptr;
}

xaero::Region::TileChunk::Chunk::Chunk() : chunkInterpretationVersion(1) {
}

xaero::Region::TileChunk::Chunk::~Chunk() {
    delete pixels;
}

std::array<xaero::Region::TileChunk::Chunk, 4>& xaero::Region::TileChunk::operator[](int x) {
    return (*chunks)[x];
}

const std::array<xaero::Region::TileChunk::Chunk, 4>& xaero::Region::TileChunk::operator[](int x) const {
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
    chunks = new std::array<std::array<Chunk, 4>, 4>();
}

void xaero::Region::TileChunk::deallocateChunks() noexcept {
    delete chunks;
    chunks = nullptr;
}

xaero::Region::TileChunk::~TileChunk() {
    delete chunks;
}

xaero::Region::TileChunk::TileChunk(const TileChunk &other) {
    if (!other.isPopulated()) {
        return;
    }

    allocateChunks();

    *chunks = *other.chunks;
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

    *chunks = *other.chunks;

    return *this;
}

xaero::Region::TileChunk & xaero::Region::TileChunk::operator=(TileChunk &&other) noexcept {
    if (this == &other) return *this;

    deallocateChunks();

    chunks = std::exchange(other.chunks, nullptr);

    return *this;
}

std::array<xaero::Region::TileChunk, 8>& xaero::Region::operator[](const int x) {
    return tileChunks[x];
}

const std::array<xaero::Region::TileChunk, 8>& xaero::Region::operator[](const int x) const {
    return tileChunks[x];
}

const xaero::Region::TileChunk::Chunk::Pixel* xaero::Region::operator[
](const std::uint16_t relX, const std::uint16_t relZ) const {
    if (relX >= 512 || relZ >= 512) return nullptr; // out of bounds!

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
                    TileChunk::Chunk& chunk = other[tileChunkX][tileChunkZ][chunkX][chunkZ];
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
