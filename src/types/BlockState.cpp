#include "../../include/xaero/types/BlockState.hpp"

#include <format>
#include <tag_string.h>

#include "util/StringUtils.hpp"

std::string_view xaero::BlockState::strippedName() const {
    return stripName(name);
}

xaero::BlockState::BlockState(nbt::tag_compound nbt) :
    name (nbt.at("Name").as<nbt::tag_string>().get()),
    properties(nbt.has_key("Properties") ? std::move(nbt.at("Properties").as<nbt::tag_compound>()) : nbt::tag_compound{}) {
}

xaero::BlockState::BlockState(std::string name, nbt::tag_compound properties) : name(std::move(name)), properties(std::move(properties)) {
}

nbt::tag_compound xaero::BlockState::getNBT() const {
    if (properties.size() > 0) {
        return {
            std::pair{"Name", std::format("minecraft:{}", strippedName())},
            std::pair{"Properties", properties}
        };
    }
    return {
        std::pair{"Name", std::format("minecraft:{}", strippedName())},
    };
}

bool xaero::BlockState::operator==(const BlockState &other) const {
    return other.strippedName() == strippedName() && other.properties == properties;
}

bool xaero::BlockState::isName(const std::string_view &other) const {

    return strippedName() == stripName(other);
}
