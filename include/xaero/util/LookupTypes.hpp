#pragma once

#include <string_view>
#include <unordered_map>
#include <map>

#include "RegionImage.hpp"

namespace nbt {
    class value;
    class tag_compound;
}

namespace xaero {
    struct ValueCompare {
        [[nodiscard]] bool operator()(const nbt::value& lhs, const nbt::value& rhs) const;
    };
    struct CompoundCompare {
        [[nodiscard]] bool operator()(const nbt::tag_compound& lhs, const nbt::tag_compound& rhs) const;
    };

    using StateLookup = std::unordered_map<std::string_view, std::map<nbt::tag_compound, RegionImage::Pixel, CompoundCompare>>;
    using StateIDLookup = std::optional<std::tuple<nbt::tag_compound, RegionImage::Pixel, std::string_view>>[];
} // xaero
