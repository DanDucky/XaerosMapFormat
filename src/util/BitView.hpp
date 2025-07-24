#pragma once

#include <bitset>
#include <cmath>
#include <memory>

template <typename T>
class BitView {
private:
    const T data;
    std::size_t position = 0;

public:
    explicit BitView(T data) : data(std::move(data)) {
    }

    T getNextBits(const std::size_t n) {
        const auto output = peekNextBits(n);
        position += n;
        return output;
    }

    T peekNextBits(const std::uint8_t n) {
        return static_cast<T>((data >> position) << (sizeof(T) * 8 - n)) >> (sizeof(T) * 8 - n) ;
    }

    void skipBits(const std::size_t n) {
        position += n;
    }

    void skipToNextByte() {
        position = ((position >> 3) + 1) << 3;
    }

};
