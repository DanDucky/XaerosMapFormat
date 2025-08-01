#pragma once

#include <string_view>
#include "xaero/types/LookupTypes.hpp"

namespace xaero {
    std::string_view fixBiome (const std::string_view& biome);

    std::string_view getBiomeFromID(std::uint32_t biomeID);

    void convertNBT(std::unique_ptr<nbt::tag_compound>& nbt, std::int16_t majorVersion);
}
