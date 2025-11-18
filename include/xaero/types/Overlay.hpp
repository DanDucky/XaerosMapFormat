#pragma once

#include <cstdint>
#include <optional>
#include <variant>
#include <memory>

#include "BlockState.hpp"

namespace xaero {
    struct Overlay {
        std::uint8_t light;
        std::optional<std::int32_t> opacity;
        std::variant<std::monostate, BlockState, std::shared_ptr<BlockState>, const BlockState*> state;

        [[nodiscard]] const BlockState& getState() const;
    };
}
