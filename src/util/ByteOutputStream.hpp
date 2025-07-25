#pragma once

#include <ostream>
#include <cstdint>
#include <cstddef>

#include "BitWriter.hpp"

class ByteOutputStream {
private:
    std::ostream& stream;
public:
    ByteOutputStream(std::ostream& stream);

    [[nodiscard]] std::ostream& getStream() const;

    [[nodiscard]] operator std::ostream&() const;

    template<typename T>
    void write(T value);

    template<typename T>
    void write(BitWriter<T> value);

};

template<typename T>
void ByteOutputStream::write(T value) {
    std::ranges::reverse(reinterpret_cast<std::uint8_t*>(&value), reinterpret_cast<std::uint8_t*>(&value) + sizeof(value));
    stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

template<typename T>
void ByteOutputStream::write(BitWriter<T> value) {
    write(std::move(value.getValue()));
}
