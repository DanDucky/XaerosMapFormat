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

void xaero::Map::addRegion(const std::filesystem::path &file, const MergeType merge, std::int32_t regionX, std::int32_t regionZ) {
    if (regionX == std::numeric_limits<std::int32_t>::min() || regionZ == std::numeric_limits<std::int32_t>::min()) {
        const std::string_view stem {file.stem().c_str()};
        const auto split = stem.find_first_of('_');

        if (split == std::string_view::npos) {
            throw std::out_of_range(std::format("could not parse file name for region pos: {}", stem));
        }

        std::from_chars(stem.data(), &stem.at(split), regionX);
        std::from_chars(&stem.at(split + 1), &stem.back(), regionZ);
    }
    addRegion(parseRegion(file), merge, regionX, regionZ);
}

void xaero::Map::addRegion(const std::string_view &data, const MergeType merge, const std::int32_t regionX, const std::int32_t regionZ) {
    addRegion(parseRegion(data), merge, regionX, regionZ);
}

void xaero::Map::addRegion(std::istream &data, const MergeType merge, const std::int32_t regionX, const std::int32_t regionZ) {
    addRegion(parseRegion(data), merge, regionX, regionZ);
}

void xaero::Map::addRegion(Region &&region, const MergeType merge, const std::int32_t regionX, const std::int32_t regionZ) {
    const auto contained = find({regionX, regionZ});

    if (contained == end()) {
        emplace(std::pair{regionX, regionZ}, std::forward<Region>(region));
        return;
    }

    switch (merge) {
        case MergeType::OVERRIDE:
            contained->second = std::forward<Region>(region);
            break;
        case MergeType::ABOVE:
            contained->second.mergeMove(region);
            break;
        case MergeType::BELOW:
            region.mergeMove(contained->second);
            contained->second = std::forward<Region>(region);
            break;
    }
}

void xaero::Map::addPixel(Region::TileChunk::Chunk::Pixel &&pixel, const std::int32_t x, const std::int32_t z) {
    std::int32_t regionX = x >> 9;
    std::int32_t regionZ = z >> 9;

    if (const auto contained = find({regionX, regionZ});
        contained == end()) {
        Region region;

        auto& tileChunk = region[x >> 6 & 7][z >> 6 & 7];
        tileChunk.allocateChunks();
        auto& chunk = tileChunk[x >> 4 & 3][z >> 4 & 3];
        chunk.allocateColumns();
        chunk[x & 15][z & 15] = std::forward<Region::TileChunk::Chunk::Pixel>(pixel);

        emplace(std::pair{regionX, regionZ}, std::move(region));
        } else {
            auto& tileChunk = contained->second[x >> 6 & 7][z >> 6 & 7];
            tileChunk.allocateChunks();
            auto& chunk = tileChunk[x >> 4 & 3][z >> 4 & 3];
            chunk.allocateColumns();

            chunk[x & 15][z & 15] = std::forward<Region::TileChunk::Chunk::Pixel>(pixel);
        }
}

void xaero::Map::addChunk(Region::TileChunk::Chunk &&chunk, const std::int32_t chunkX, const std::int32_t chunkZ) {
    std::int32_t regionX = chunkX >> 5;
    std::int32_t regionZ = chunkZ >> 5;

    if (const auto contained = find({regionX, regionZ});
        contained == end()) {
        Region region;

        auto& tileChunk = region[chunkX >> 2 & 7][chunkZ >> 2 & 7];
        tileChunk.allocateChunks();
        tileChunk[chunkX & 3][chunkZ & 3] = std::forward<Region::TileChunk::Chunk>(chunk);

        emplace(std::pair{regionX, regionZ}, std::move(region));
        } else {
            auto& tileChunk = contained->second[chunkX >> 2 & 7][chunkZ >> 2 & 7];
            tileChunk.allocateChunks();
            tileChunk[chunkX & 3][chunkZ & 3] = std::forward<Region::TileChunk::Chunk>(chunk);
        }
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

    return xaero::writeRegion(contained->second, path, lookups);
}

std::string xaero::Map::getSerialized(std::int32_t regionX, std::int32_t regionZ) const {
    const auto contained = find({regionX, regionZ});

    if (contained == end()) return "";

    return xaero::serializeRegion(contained->second, lookups);
}

bool xaero::Map::writeRegions(const std::filesystem::path &rootPath) const {
    for (const auto&[coordinates, region] : *this) {
        if (!xaero::writeRegion(region, rootPath / std::format("{}_{}.zip", coordinates.first, coordinates.second), lookups)) {
            return false;
        }
    }
    return true;
}

std::vector<std::pair<std::pair<std::int32_t, std::int32_t>, std::string>> xaero::Map::getSerialized() const {
    std::vector<std::pair<std::pair<std::int32_t, std::int32_t>, std::string>> output;
    output.reserve(size());

    for (const auto&[coordinates, region] : *this) {
        output.emplace_back(coordinates, serializeRegion(region, lookups));
    }

    return output;
}
