#pragma once
#include <string>
#include <tag_compound.h>

namespace xaero {
    struct BlockState {
        std::string name;
        nbt::tag_compound properties;

        explicit BlockState(nbt::tag_compound nbt);
        explicit BlockState(std::string name, nbt::tag_compound properties);

        [[nodiscard]] std::string_view strippedName() const;
        [[nodiscard]] std::string taggedName() const;

        [[nodiscard]] bool operator==(const BlockState & other) const;

        [[nodiscard]] bool isName(const std::string_view& other) const;
    };
}
