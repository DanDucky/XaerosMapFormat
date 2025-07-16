#pragma once

#include <string_view>
#include <unordered_map>
#include <map>
#include <optional>
#include <cstddef>
#include <nbt_tags.h>

#include "../util/RegionImage.hpp"

namespace xaero {
    struct CompoundCompare {
        [[nodiscard]] bool operator()(const nbt::tag_compound& lhs, const nbt::tag_compound& rhs) const;
    };
    struct ValueCompare {
        [[nodiscard]] bool operator()(const nbt::value& lhs, const nbt::value& rhs) const;
    };

    struct StateIDPack {
        std::string_view name;
        nbt::tag_compound properties;
        RegionImage::Pixel color;
    };

    struct DefaultStateIDLookup {
        const std::optional<const StateIDPack>& operator[](std::size_t index) const;
    };

    using StateLookup = std::unordered_map<std::string_view, std::map<nbt::tag_compound, RegionImage::Pixel, CompoundCompare>>;
    using StateIDLookupElement = std::optional<const StateIDPack>;
    using StateIDLookupChunk = StateIDLookupElement[];
} // xaero
