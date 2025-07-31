#include "StringUtils.hpp"

std::string_view xaero::stripName(const std::string_view &name) noexcept {
    if (const auto found = name.find_first_of(':');
        found != std::string_view::npos) {

        return name.substr(found + 1);
    }
    return name;
}
