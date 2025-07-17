#include "ByteInputStream.hpp"

#include <cmath>
#include <algorithm>

#include "Math.hpp"


xaero::ByteInputStream::ByteInputStream(std::istream &inputStream) : stream(inputStream) {

}

std::istream & xaero::ByteInputStream::getStream() const {
    return stream;
}

std::uint8_t xaero::ByteInputStream::getNextBits(const std::uint8_t n) {
    const std::uint8_t output = peekNextBits(std::clamp<std::uint8_t>(n, 0, 8));
    skipBits(std::clamp<std::uint8_t>(n, 0, 8));
    return output;
}

std::uint8_t xaero::ByteInputStream::peekNextBits(const std::uint8_t n) {
    return peekNext<std::uint16_t>() >> subBytePosition & static_cast<std::uint16_t>(std::pow(2, n)) - 1;
}

void xaero::ByteInputStream::skip(const int n) {
    stream.ignore(n);
}

void xaero::ByteInputStream::skipBits(const int n) {
    const auto rollovers = math::addWithRollover<std::uint8_t>(subBytePosition, n, 0, 8);
    skip(rollovers);
}

void xaero::ByteInputStream::skipToNextByte() {
    subBytePosition = 0;
    skip(1);
}

bool xaero::ByteInputStream::eof() const {
    return stream.eof();
}
