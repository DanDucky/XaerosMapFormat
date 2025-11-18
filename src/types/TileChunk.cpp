#include "xaero/types/TileChunk.hpp"

#include <utility>

std::array<xaero::Chunk, 4>& xaero::TileChunk::operator[](int x) {
    return (*chunks)[x];
}

const std::array<xaero::Chunk, 4>& xaero::TileChunk::operator[](int x) const {
    return (*chunks)[x];
}

bool xaero::TileChunk::isPopulated() const {
    return chunks;
}

xaero::TileChunk::operator bool() const {
    return isPopulated();
}

void xaero::TileChunk::allocateChunks() {
    if (chunks) return;
    chunks = new std::array<std::array<Chunk, 4>, 4>();
}

void xaero::TileChunk::deallocateChunks() noexcept {
    delete chunks;
    chunks = nullptr;
}

xaero::TileChunk::~TileChunk() {
    delete chunks;
}

xaero::TileChunk::TileChunk(const TileChunk &other) {
    if (!other.isPopulated()) {
        return;
    }

    allocateChunks();

    *chunks = *other.chunks;
}

xaero::TileChunk::TileChunk(TileChunk &&other) noexcept {
    if (!other.isPopulated()) return;

    chunks = std::exchange(other.chunks, nullptr);
}

xaero::TileChunk & xaero::TileChunk::operator=(const TileChunk &other) {
    if (this == &other) return *this;

    if (!other.isPopulated()) {
        deallocateChunks();
        return *this;
    }

    allocateChunks();

    *chunks = *other.chunks;

    return *this;
}

xaero::TileChunk & xaero::TileChunk::operator=(TileChunk &&other) noexcept {
    if (this == &other) return *this;

    deallocateChunks();

    chunks = std::exchange(other.chunks, nullptr);

    return *this;
}
