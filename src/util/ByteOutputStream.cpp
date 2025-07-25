#include "ByteOutputStream.hpp"

ByteOutputStream::ByteOutputStream(std::ostream &stream) : stream(stream) {
}

std::ostream & ByteOutputStream::getStream() const {
    return stream;
}

ByteOutputStream::operator std::ostream&() const {
    return stream;
}
