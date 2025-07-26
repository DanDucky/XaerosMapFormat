#pragma once

#include <ostream>
#include <cstdint>
#include <cstddef>
#include <bit>

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

    void writeMUTF(const std::string& string);

};

template<typename T>
void ByteOutputStream::write(T value) {
    if constexpr (std::endian::native == std::endian::little) {
        std::ranges::reverse(reinterpret_cast<std::uint8_t*>(&value), reinterpret_cast<std::uint8_t*>(&value) + sizeof(value));
    } else if constexpr (std::endian::native != std::endian::big) {
        // test endianness at runtime
        union {
            std::uint16_t value = 1;
            std::uint8_t array[2];
        };
        if (array[0] == 1) std::ranges::reverse(reinterpret_cast<std::uint8_t*>(&value), reinterpret_cast<std::uint8_t*>(&value) + sizeof(value));
    }
    stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

template<typename T>
void ByteOutputStream::write(BitWriter<T> value) {
    write(std::move(value.getValue()));
}
