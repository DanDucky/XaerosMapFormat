#include "xaero/types/Pixel.hpp"

#include "util/VariantUtils.hpp"

bool xaero::Pixel::hasOverlays() const {
    return !overlays.empty();
}

const xaero::BlockState& xaero::Pixel::getState() const {
    return getStateForUnion(state);
}

std::optional<std::string_view> xaero::Pixel::getBiome() const {
    if (!biome) return std::nullopt;
    return getBiomeForUnion(biome.value());
}
