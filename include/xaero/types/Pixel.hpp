#pragma once

#include <cstdint>
#include <optional>
#include <variant>
#include <memory>
#include <vector>

#include "BlockState.hpp"
#include "Overlay.hpp"

namespace xaero {
    struct Pixel {
        std::uint8_t light;
        std::int16_t height;
        std::optional<std::uint8_t> topHeight;

        // the rationale for this being optional instead of having a std::monostate is that xaero "officially" might not have a biome, but it ALWAYS has a state
        std::optional<std::variant<std::shared_ptr<std::string>, std::string, std::string_view>> biome;

        // nullptr is default / air
        std::variant<const BlockState*, std::shared_ptr<BlockState>> state = nullptr;

        std::vector<Overlay> overlays;

        [[nodiscard]] bool hasOverlays() const;

        [[nodiscard]] const BlockState& getState() const;
        [[nodiscard]] std::optional<std::string_view> getBiome() const;
    };
}
