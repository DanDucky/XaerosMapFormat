#include "xaero/types/Chunk.hpp"

#include <utility>

xaero::Chunk::Chunk() : chunkInterpretationVersion(1) {
}

const std::array<xaero::Pixel, 16>& xaero::Chunk::operator[
](const int x) const {
    return (*pixels)[x];
}

std::array<xaero::Pixel, 16>& xaero::Chunk::operator[](const int x) {
    return (*pixels)[x];
}

xaero::Chunk::Chunk(const Chunk &other) {
    caveDepth = other.caveDepth;
    caveStart = other.caveStart;
    chunkInterpretationVersion = other.chunkInterpretationVersion;

    if (!other.isPopulated()) {
        return;
    }

    allocateColumns();

    *pixels = *other.pixels;
}

xaero::Chunk::Chunk(Chunk &&other) noexcept {
    caveDepth = other.caveDepth;
    caveStart = other.caveStart;
    chunkInterpretationVersion = other.chunkInterpretationVersion;

    pixels = std::exchange(other.pixels, nullptr);
}

xaero::Chunk & xaero::Chunk::operator=(const Chunk &other) {
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

xaero::Chunk & xaero::Chunk::operator=(Chunk &&other) noexcept {
    if (this == &other) return *this;

    deallocateColumns();

    caveDepth = other.caveDepth;
    caveStart = other.caveStart;
    chunkInterpretationVersion = other.chunkInterpretationVersion;

    pixels = std::exchange(other.pixels, nullptr);

    return *this;
}

bool xaero::Chunk::isPopulated() const {
    return pixels;
}

void xaero::Chunk::allocateColumns() {
    if (pixels) return;
    pixels = new std::array<std::array<Pixel, 16>, 16>();
}

void xaero::Chunk::deallocateColumns() noexcept {
    delete[] reinterpret_cast<Pixel*>(pixels);
    pixels = nullptr;
}

xaero::Chunk::~Chunk() {
    delete pixels;
}
