#pragma once
#include <algorithm>
#include <iostream>
#include <cstdint>
#include <ranges>

#include "BitView.hpp"

namespace xaero {
    class ByteInputStream final {
    private:
        std::istream& stream;
    public:
        explicit ByteInputStream(std::istream& inputStream);

        [[nodiscard]] std::istream& getStream() const;

        [[nodiscard]] operator std::istream&() const;

        template<typename T>
        [[nodiscard]] T getNext();

        /**
         * reads in an MUTF string from the stream, necessary to interact with java's readUTF()
         * @return c++ std::string representing the string read in standard c++ ascii
         */
        [[nodiscard]] std::string getNextMUTF();

        template<typename T>
        [[nodiscard]] T peekNext();

        template<typename T>
        [[nodiscard]] BitView<T> peekNextAsView();
        template<typename T>
        [[nodiscard]] BitView<T> getNextAsView();

        void skip(int n);

        [[nodiscard]] bool eof() const;
    };

    template<typename T>
    T ByteInputStream::getNext() {
        T output;
        stream.read(reinterpret_cast<char*>(&output), sizeof(T));
        std::ranges::reverse(reinterpret_cast<char*>(&output), reinterpret_cast<char*>(&output) + sizeof(T));
        return output;
    }

    template<typename T>
    T ByteInputStream::peekNext() {
        const auto position = stream.tellg();
        const auto output = getNext<T>();
        stream.seekg(position);
        return output;
    }

    template<typename T>
    BitView<T> ByteInputStream::peekNextAsView() {
        return BitView<T>(peekNext<T>());
    }

    template<typename T>
    BitView<T> ByteInputStream::getNextAsView() {
        return BitView<T>(getNext<T>());
    }
}
