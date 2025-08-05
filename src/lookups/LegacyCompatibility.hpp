#pragma once

#include <string_view>
#include <cstdint>
#include <memory>
#include <tag_compound.h>
#include <cstddef>
#include <vector>
#include <xaero/types/BlockState.hpp>

namespace xaero {
    std::string_view fixBiome (const std::string_view& biome);

    std::string_view getBiomeFromID(std::uint32_t biomeID);

    void convertNBT(std::unique_ptr<nbt::tag_compound>& nbt, std::int16_t majorVersion);

    const BlockState* getStateFromID(std::uint32_t stateID);

    using StateIDLookup = std::vector<BlockState>[]; // I know this is evil but it all lives in a similar cache area so whatever

    extern const StateIDLookup stateIDLookup;
    extern const std::size_t stateIDLookupSize;
}
