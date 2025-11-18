#pragma once
#include <variant>

#include "StringUtils.hpp"
#include "xaero/types/BlockState.hpp"
#include "../lookups/LegacyCompatibility.hpp"

namespace xaero {
    inline const BlockState& getStateForUnion(const std::variant<std::monostate, BlockState, std::shared_ptr<BlockState>, const BlockState*>& state) {
        if (std::holds_alternative<std::monostate>(state)) {
            return *getStateFromID(0); // air
        }
        if (std::holds_alternative<const BlockState*>(state)) {
            return *std::get<const BlockState*>(state);
        }
        if (std::holds_alternative<BlockState>(state)) {
            return std::get<BlockState>(state);
        }
        return *std::get<std::shared_ptr<BlockState>>(state).get();
    }

    inline std::string_view getBiomeForUnion(const std::variant<std::shared_ptr<std::string>, std::string, std::string_view>& biome) {
        std::string_view out;
        if (std::holds_alternative<std::shared_ptr<std::string>>(biome)) {
            out = *std::get<std::shared_ptr<std::string>>(biome);
        } else if (std::holds_alternative<std::string>(biome)) {
            out = std::get<std::string>(biome);
        } else if (std::holds_alternative<std::string_view>(biome)) {
            out = std::get<std::string_view>(biome);
        }

        return stripName(out);
    }
}
