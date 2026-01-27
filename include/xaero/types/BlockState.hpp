#pragma once
#include <string>
#include <optional>
#include <tag_compound.h>

#include "ColorInfo.hpp"

namespace xaero {
    struct BlockState {
        std::string name;
        nbt::tag_compound properties;
        std::optional<ColorInfo> color; // having this extra few bytes lets us skip 1000s of massive lookups later

        explicit BlockState(nbt::tag_compound nbt);
        explicit BlockState(std::string name, nbt::tag_compound properties, std::optional<ColorInfo> color=std::nullopt);

        [[nodiscard]] std::string_view strippedName() const;
        [[nodiscard]] std::string taggedName() const;

        // does not check color data
        [[nodiscard]] bool operator==(const BlockState & other) const;

        [[nodiscard]] bool isName(const std::string_view& other) const;
    };
}
