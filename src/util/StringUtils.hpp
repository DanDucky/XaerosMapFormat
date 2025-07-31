#pragma once

#include <string_view>

namespace xaero {
    [[nodiscard]] std::string_view stripName(const std::string_view& name) noexcept;
}