#include "../../include/xaero/util/Region.hpp"

void xaero::Region::TileChunk::Chunk::Pixel::allocateOverlays() {
    overlays = new Overlay[numberOfOverlays];
}

xaero::Region::TileChunk::Chunk::Pixel::~Pixel() {
    delete[] overlays;
}

xaero::Region::TileChunk::Chunk::Pixel * xaero::Region::TileChunk::Chunk::operator[](int x) {
    return *columns[x];
}

const xaero::Region::TileChunk::Chunk::Pixel * xaero::Region::TileChunk::Chunk::operator[](int x) const {
    return *columns[x];
}

void xaero::Region::TileChunk::Chunk::allocateColumns() {
    if (isVoid) return; // todo this could / should probably be inferred by columns being nullptr or not
    columns = reinterpret_cast<Pixel(*)[16][16]>(new Pixel[16 * 16]);
}

xaero::Region::TileChunk::Chunk::~Chunk() {
    delete[] columns;
}

xaero::Region::TileChunk::Chunk * xaero::Region::TileChunk::operator[](int x) {
    return *chunks[x];
}

const xaero::Region::TileChunk::Chunk * xaero::Region::TileChunk::operator[](int x) const {
    return *chunks[x];
}

void xaero::Region::TileChunk::allocateChunks() {
    chunks = reinterpret_cast<Chunk(*)[4][4]>(new Chunk[4 * 4]);
}

xaero::Region::TileChunk::~TileChunk() {
    delete[] chunks;
}

xaero::Region::TileChunk * xaero::Region::operator[](const int x) {
    return tileChunks[x];
}

const xaero::Region::TileChunk * xaero::Region::operator[](const int x) const {
    return tileChunks[x];
}
