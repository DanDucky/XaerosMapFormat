#include "StringUtils.hpp"

std::string_view xaero::stripName(const std::string_view &name) noexcept {
    if (name.length() <= 10) return name;

    if (name.at(9) == ':') {
        return name.substr(10);
    }

    return name;
}
