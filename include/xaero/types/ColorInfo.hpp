#pragma once
#include "RegionImage.hpp"

namespace xaero {
    struct ColorInfo {
        RegionImage::Pixel color;
        std::int32_t tintIndex;
    };
}
