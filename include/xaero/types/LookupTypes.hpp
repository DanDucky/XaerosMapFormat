#pragma once

#include <string_view>
#include <unordered_map>
#include <nbt_tags.h>

#include "RegionImage.hpp"

namespace xaero {
    // some types are only available with XAERO_DEFAULT_LOOKUPS! these are types which are customized for that option!

    struct CompoundCompare {
        [[nodiscard]] bool operator()(const nbt::tag_compound& lhs, const nbt::tag_compound& rhs) const noexcept;
    };

    struct StatePack {
        RegionImage::Pixel color;
        std::int32_t tintIndex = -1;
    };

    using StateLookup = std::unordered_map<std::string_view, std::map<nbt::tag_compound, StatePack, CompoundCompare>>;

    struct BiomeColors {
        RegionImage::Pixel grass;
        RegionImage::Pixel water;
        RegionImage::Pixel foliage;
        RegionImage::Pixel dryFoliage;
    };

    using BiomeLookup = std::unordered_map<std::string_view, BiomeColors>;

    /**
     * @warning these are stored as pointers! please don't move them while this is in use anywhere
     */
    struct LookupPack {
        const StateLookup* stateLookup;
        const BiomeLookup* biomeLookup;
    };
} // xaero
