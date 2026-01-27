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

        // nullptr is default / air
        std::variant<const BlockState*, std::shared_ptr<BlockState>> state = nullptr;

        [[nodiscard]] const BlockState& getState() const;
    };
}
