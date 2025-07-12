#pragma once

#include <string_view>
#include <unordered_map>
#include <map>
#include <optional>
#include <nbt_tags.h>
#include <tuple>

#include "RegionImage.hpp"

namespace xaero {
    struct ValueCompare {
        [[nodiscard]] bool operator()(const nbt::value& lhs, const nbt::value& rhs) const;
    };
    struct CompoundCompare {
        [[nodiscard]] bool operator()(const nbt::tag_compound& lhs, const nbt::tag_compound& rhs) const;
    };

    using StateLookup = std::unordered_map<std::string_view, std::map<nbt::tag_compound, RegionImage::Pixel, CompoundCompare>>;
    using StateIDLookup = std::optional<std::tuple<std::string_view, nbt::tag_compound, RegionImage::Pixel>>[];
} // xaero
