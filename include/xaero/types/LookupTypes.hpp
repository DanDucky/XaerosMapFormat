#pragma once

#include <string_view>
#include <unordered_map>
#include <flat_map>
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
    struct ValueCompare {
        [[nodiscard]] bool operator()(const nbt::value& lhs, const nbt::value& rhs) const noexcept;
    };

    struct StateIDPack {
        BlockState state;
        RegionImage::Pixel color;
    };

    struct NameHash { // removes minecraft:* from name in the hash, because that's nasty and causes unnecessary collisions
        [[nodiscard]] std::size_t operator()(const std::string_view& name) const noexcept;
    };

    struct NameEquals { // removes minecraft:*
        [[nodiscard]] bool operator()(const std::string_view& a, const std::string_view& b) const noexcept;
    };

    struct NameCompare {
        [[nodiscard]] bool operator()(const std::string_view& a, const std::string_view& b) const noexcept;
    };

    using StateLookup = std::unordered_map<std::string_view, std::flat_map<nbt::tag_compound, RegionImage::Pixel, CompoundCompare>, NameHash, NameEquals>;
    using StateIDLookupElement = std::optional<StateIDPack>;

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
    };

#ifdef XAERO_DEFAULT_LOOKUPS
    extern const LookupPack defaultLookupPack;
#endif

} // xaero
