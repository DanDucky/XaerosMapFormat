#include "../../include/xaero/types/Region.hpp"

bool xaero::Region::TileChunk::Chunk::Pixel::hasOverlays() const {
    return !overlays.empty();
}

bool xaero::Region::TileChunk::Chunk::Pixel::isAir() const {
    if (std::holds_alternative<std::monostate>(state)) {
        return true;
    } else if (std::holds_alternative<std::int32_t>(state)) {
        return std::get<std::int32_t>(state) == 0;
    } else if (std::holds_alternative<BlockState>(state)) {
        const auto name = std::string_view(std::get<BlockState>(state).name);
        const auto first = name.find_first_of(':');
        return name.substr(first != std::string_view::npos ? first + 1 : 0) == "air";
    } else if (std::holds_alternative<BlockState*>(state)) {
        const auto name = std::string_view(std::get<BlockState*>(state)->name);
        const auto first = name.find_first_of(':');
        return name.substr(first != std::string_view::npos ? first + 1 : 0) == "air";
    } else if (std::holds_alternative<std::shared_ptr<BlockState>>(state)) {
        const auto name = std::string_view(std::get<std::shared_ptr<BlockState>>(state)->name);
        const auto first = name.find_first_of(':');
        return name.substr(first != std::string_view::npos ? first + 1 : 0) == "air";
    }
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

xaero::Region::TileChunk::~TileChunk() {
    delete[] reinterpret_cast<Chunk*>(chunks);
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

    const auto& tileChunk = tileChunks[relX >> 7][relZ >> 7];

    if (!tileChunk.isPopulated()) return nullptr;

    const auto& chunk = tileChunk[relX >> 4 & 3][relZ >> 4 & 3];

    if (!chunk.isPopulated()) return nullptr;

    return &chunk[relX & 15][relZ & 15];
}

xaero::Region::TileChunk::Chunk::Pixel * xaero::Region::operator[](std::uint16_t relX, std::uint16_t relZ) {
    if (relX >= 521 || relZ >= 512) return nullptr;

    auto& tileChunk = tileChunks[relX >> 7][relZ >> 7];

    if (!tileChunk.isPopulated()) return nullptr;

    auto& chunk = tileChunk[relX >> 4 & 3][relZ >> 4 & 3];

    if (!chunk.isPopulated()) return nullptr;

    return &chunk[relX & 15][relZ & 15];
}

xaero::Region::Region() : majorVersion(6), minorVersion(8) {

}
