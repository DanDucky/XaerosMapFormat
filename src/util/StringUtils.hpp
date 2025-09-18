#pragma once

#include <string_view>

namespace xaero {
    // super specialized for "minecraft:", literally checks for the 9th char or whatever and sees if it's :
    [[nodiscard]] std::string_view stripName(const std::string_view& name) noexcept;
}