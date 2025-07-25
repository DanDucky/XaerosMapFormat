#include "../../include/xaero/types/RegionImage.hpp"

const xaero::RegionImage::Pixel * xaero::RegionImage::operator[](const int x) const {
    return image[x];
}

xaero::RegionImage::Pixel * xaero::RegionImage::operator[](const int x) {
    return image[x];
}
