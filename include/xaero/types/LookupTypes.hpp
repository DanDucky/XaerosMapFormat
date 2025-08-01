#pragma once

#include <string_view>
#include <unordered_map>
#include <optional>
#include <cstddef>
#include <nbt_tags.h>

#include "BlockState.hpp"
#include "RegionImage.hpp"
#include "xaero/util/IndexableView.hpp"


namespace xaero {
    // some types are only available with XAERO_DEFAULT_LOOKUPS! these are types which are customized for that option!

    struct CompoundCompare {
        [[nodiscard]] bool operator()(const nbt::tag_compound& lhs, const nbt::tag_compound& rhs) const noexcept;
    };

    struct StateIDPack {
        BlockState state;
        RegionImage::Pixel color;
        std::int32_t tintIndex = -1; // bs attribute, minecraft's internal files LIE about it constantly, but it's nice to have
    };

    struct StatePack {
        RegionImage::Pixel color;
        std::int32_t tintIndex = -1;
    };

    struct NameHash { // removes minecraft:* from name in the hash, because that's nasty and causes unnecessary collisions
        [[nodiscard]] std::size_t operator()(const std::string_view& name) const noexcept;
    };

    struct NameEquals { // removes minecraft:*
        [[nodiscard]] bool operator()(const std::string_view& a, const std::string_view& b) const noexcept;
    };

    struct NameCompare { // removes minecraft:*
        [[nodiscard]] bool operator()(const std::string_view& a, const std::string_view& b) const noexcept;
    };

    using StateLookup = std::unordered_map<std::string_view, std::map<nbt::tag_compound, StatePack, CompoundCompare>, NameHash, NameEquals>;
    using StateIDLookupElement = std::optional<StateIDPack>;

    struct BiomeColors {
        RegionImage::Pixel grass;
        RegionImage::Pixel water;
        RegionImage::Pixel foliage;
        RegionImage::Pixel dryFoliage;
    };

    using BiomeLookup = std::map<std::string_view, BiomeColors, NameCompare>;

#ifdef XAERO_DEFAULT_LOOKUPS

    struct DefaultStateIDLookup {
        const StateIDLookupElement& operator[](std::size_t index) const;
    };
    using StateIDLookupChunk = StateIDLookupElement[];

#endif

    /**
     * @warning these are stored as references! please don't move them while this is in use anywhere
     */
    struct LookupPack {
        const StateLookup& stateLookup;
        const IndexableView<const StateIDLookupElement&>& stateIDLookup;
        const std::size_t stateIDLookupSize;
        const BiomeLookup& biomeLookup;
    };
} // xaero
