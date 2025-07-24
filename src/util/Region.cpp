#include "../../include/xaero/util/Region.hpp"

bool xaero::Region::TileChunk::Chunk::Pixel::hasOverlays() const {
    return !overlays.empty();
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
