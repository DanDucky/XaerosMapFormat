#pragma once

#include <algorithm>
#include <cstddef>
#include <cstring>

template<typename T>
struct BitWriter {
private:
    std::size_t position;

public:
    union { // union so uninitialized
        T data;
    };

    explicit BitWriter(T data) : data(std::move(data)) {
        reset();
    }

    BitWriter() {
        reset();
    }

    /**
     * resets the data as well as the position
     */
    void reset() {
        position = 0;
        std::memset(&data, 0, sizeof(data));
    }

    [[nodiscard]] T& getValue() {
        return data;
    }
    [[nodiscard]] const T& getValue() const {
        return data;
    }

    [[nodiscard]] T& operator*() {
        return data;
    }
    [[nodiscard]] const T& operator*() const {
        return data;
    }

    /**
     *
     * @tparam V value type to write
     * @param value value to write
     * @param size amount of that value to write, by default the whole thing
     * @return number of bits written
     */
    template<typename V>
    std::size_t writeNext(const V value, std::size_t size=sizeof(V) * 8) {
        size = std::clamp<std::size_t>(size, 0, sizeof(V) * 8 - position);
        if (size == 0) return 0;

        data |= (static_cast<std::size_t>(value) << (sizeof(value) * 8 - size)) >> (sizeof(data) - size + position);

        return size;
    }

    [[nodiscard]] std::size_t getPosition() const {
        return position;
    }

    void skip(const std::size_t bits) {
        position += bits;
    }

    void setPosition(const std::size_t position) {
        this->position = position;
    }

    void skipToNextByte() {
        position = ((position >> 3) + 1) << 3;
    }
};
