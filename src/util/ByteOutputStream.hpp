#pragma once

#include <ostream>
#include <cstdint>
#include <cstddef>
#include <bit>

#include "BitWriter.hpp"
#include "Endianness.hpp"

namespace xaero {
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
}

template<typename T>
void xaero::ByteOutputStream::write(T value) {

    if (endianness == std::endian::little) {
        std::ranges::reverse(reinterpret_cast<std::uint8_t*>(&value), reinterpret_cast<std::uint8_t*>(&value) + sizeof(value));
    }

    stream.write(reinterpret_cast<const char*>(&value), sizeof(value));

    if (stream.fail()) {
        throw std::out_of_range("somehow ran out of space or errored when writing a file");
    }
}

template<typename T>
void xaero::ByteOutputStream::write(BitWriter<T> value) {
    write(std::move(value.getValue()));
}
