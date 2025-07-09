#pragma once
#include <iostream>

namespace xaero {
    class ByteInputStream final {
    private:
        std::istream& stream;
        std::uint8_t subBytePosition = 0;
    public:
        explicit ByteInputStream(std::istream& inputStream);

        /**
         * @return the current input stream
         * @warning using the stream directly will NOT update the subBytePosition correctly! Please use with caution!
         */
        [[nodiscard]] std::istream& getStream() const;

        template<typename T>
        [[nodiscard]] T getNext();

        template<typename T>
        [[nodiscard]] T peekNext();

        /**
         * @param n number of bits to advance and grab from the current byte. can only be 0 < n <= 8
         * @return returns those bits shifted to the beginning of the byte
         */
        std::uint8_t getNextBits(const std::uint8_t n);
        /**
         * @param n number of bits to advance and grab from the current byte. can only be 0 < n <= 8
         * @return returns those bits shifted to the beginning of the byte
         */
        std::uint8_t peekNextBits(const std::uint8_t n);

        void skip(int n);
        void skipBits(int n);

        void skipToNextByte();

        [[nodiscard]] bool eof() const;
    };

    template<typename T>
    T ByteInputStream::getNext() {
        T output;
        stream.read(reinterpret_cast<char*>(&output), sizeof(T));
        subBytePosition = 0;
        return output;
    }

    template<typename T>
    T ByteInputStream::peekNext() {
        const auto position = stream.tellg();
        const auto output = getNext<T>();
        stream.seekg(position);
        return output;
    }
}
