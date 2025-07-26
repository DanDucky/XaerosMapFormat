#include "../../include/xaero/types/BlockState.hpp"

#include <format>

#include "tag_string.h"

std::string_view BlockState::strippedName() const {
    const auto found = name.find(':');
    if (found == std::string::npos) return name;

    return std::string_view(name.begin() + found + 1, name.end());
}

BlockState::BlockState(nbt::tag_compound nbt) :
    name (std::move(nbt.at("Name").as<nbt::tag_string>().get())),
    properties(nbt.has_key("Properties") ? std::move(nbt.at("Properties").as<nbt::tag_compound>()) : nbt::tag_compound{}) {
}

BlockState::BlockState(std::string name, nbt::tag_compound properties) : name(std::move(name)), properties(std::move(properties)) {
}

nbt::tag_compound BlockState::getNBT() const {
    return {
        std::pair{"Name", std::format("minecraft:{}", strippedName())},
        std::pair{"Properties", properties}
    };
}

bool BlockState::operator==(const BlockState &other) const {
    return other.strippedName() == strippedName() && other.properties == properties;
}
