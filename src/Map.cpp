#include "xaero/Map.hpp"

#include "xaero/RegionTools.hpp"

#include <format>
#include <ranges>

xaero::Map::Map() : unordered_map(), lookups(
#ifdef XAERO_DEFAULT_LOOKUPS
                        &defaultLookupPack
#else
nullptr
#endif
                    ) {
}

xaero::Map::Map(const LookupPack *lookups) : unordered_map(), lookups(lookups) {

}

void xaero::Map::setLookups(const LookupPack *lookups) {
    this->lookups = lookups;
}

const xaero::LookupPack * xaero::Map::getLookups() const {
    return lookups;
}

void xaero::Map::addRegion(const std::filesystem::path &file, std::int32_t regionX, std::int32_t regionZ,
                           const MergeType merge) {
    if (regionX == std::numeric_limits<std::int32_t>::min() || regionZ == std::numeric_limits<std::int32_t>::min()) {
        const auto stem = file.stem().string();
        const auto split = stem.find_first_of('_');

        std::from_chars(stem.data(), stem.data() + split, regionX);
        std::from_chars(stem.data() + split + 1, stem.data() + stem.size(), regionZ);
    }
    addRegion(parseRegion(file), regionX, regionZ, merge);
}

void xaero::Map::addRegion(const std::string_view &data, const std::int32_t regionX, const std::int32_t regionZ, const MergeType merge) {
    addRegion(parseRegion(data), regionX, regionZ, merge);
}

void xaero::Map::addRegion(std::istream &data, const std::int32_t regionX, const std::int32_t regionZ, const MergeType merge) {
    addRegion(parseRegion(data), regionX, regionZ, merge);
}

std::optional<xaero::RegionImage> xaero::Map::generateImage(std::int32_t regionX, std::int32_t regionZ) const {
    const auto contained = find({regionX, regionZ});

    if (contained == end()) return std::nullopt;

    return xaero::generateImage(contained->second, lookups);
}

std::vector<std::pair<std::pair<std::int32_t, std::int32_t>, xaero::RegionImage>> xaero::Map::generateImages() const {
    std::vector<std::pair<std::pair<std::int32_t, std::int32_t>, RegionImage>> output;
    output.reserve(size());

    for (const auto&[coordinates, region] : *this) {
        output.emplace_back(coordinates, xaero::generateImage(region, lookups));
    }

    return output;
}

const xaero::Region::TileChunk::Chunk * xaero::Map::getChunk(const std::int32_t chunkX, const std::int32_t chunkZ) const {
    const auto contained = find({chunkX >> 5, chunkZ >> 5});

    if (contained == end()) return nullptr;

    const auto& tileChunk = contained->second[chunkX >> 2 & 7][chunkZ >> 2 & 7];

    if (!tileChunk.isPopulated()) return nullptr;

    return &tileChunk[chunkX & 3][chunkZ & 3];
}

xaero::Region::TileChunk::Chunk * xaero::Map::getChunk(const std::int32_t chunkX, const std::int32_t chunkZ) {
    const auto contained = find({chunkX >> 5, chunkZ >> 5});

    if (contained == end()) return nullptr;

    auto& tileChunk = contained->second[chunkX >> 2 & 7][chunkZ >> 2 & 7];

    if (!tileChunk.isPopulated()) return nullptr;

    return &tileChunk[chunkX & 3][chunkZ & 3];
}

const xaero::Region::TileChunk::Chunk::Pixel * xaero::Map::getPixel(const std::int32_t x, const std::int32_t z) const {
    const auto contained = find({x >> 9, z >> 9});

    if (contained == end()) return nullptr;

    return contained->second[x & 511, z & 511];
}

xaero::Region::TileChunk::Chunk::Pixel * xaero::Map::getPixel(const std::int32_t x, const std::int32_t z) {
    const auto contained = find({x >> 9, z >> 9});

    if (contained == end()) return nullptr;

    return contained->second[x & 511, z & 511];
}

bool xaero::Map::writeRegion(std::int32_t regionX, std::int32_t regionZ, const std::filesystem::path &path) const {
    const auto contained = find({regionX, regionZ});

    if (contained == end()) return false;

    return xaero::writeRegion(contained->second, path);
}

std::string xaero::Map::getSerialized(std::int32_t regionX, std::int32_t regionZ) const {
    const auto contained = find({regionX, regionZ});

    if (contained == end()) return "";

    return xaero::serializeRegion(contained->second);
}

bool xaero::Map::writeRegions(const std::filesystem::path &rootPath) const {
    for (const auto&[coordinates, region] : *this) {
        if (!xaero::writeRegion(region, rootPath / std::format("{}_{}.zip", coordinates.first, coordinates.second))) {
            return false;
        }
    }
    return true;
}

std::vector<std::pair<std::pair<std::int32_t, std::int32_t>, std::string>> xaero::Map::getSerialized() const {
    std::vector<std::pair<std::pair<std::int32_t, std::int32_t>, std::string>> output;
    output.reserve(size());

    for (const auto&[coordinates, region] : *this) {
        output.emplace_back(coordinates, serializeRegion(region));
    }

    return output;
}
