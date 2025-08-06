#pragma once

#include <cstddef>
#include <functional>
#include <utility>

namespace xaero {
    struct PairHash {
        template<typename T1, typename T2>
        [[nodiscard]] std::size_t operator() (const std::pair<T1, T2>& pair) const;
    };

    template<typename T1, typename T2>
    std::size_t PairHash::operator()(const std::pair<T1, T2> &pair) const {
        return std::hash<T1>{}(pair.first) ^ std::hash<T2>{}(pair.second);
    }
}
