#pragma once
#include <filesystem>

#include "types/LookupTypes.hpp"
#include "types/Region.hpp"
#include "util/PairHash.hpp"

namespace xaero {
    struct RegionImage;
    class Map : public std::unordered_map<std::pair<std::int32_t, std::int32_t>, Region, PairHash> {
    private:
        const LookupPack* lookups;
    public:

        Map();
        explicit Map(const LookupPack* lookups);

        void setLookups(const LookupPack* lookups);
        [[nodiscard]] const LookupPack* getLookups() const;

        enum class MergeType : std::uint8_t {
            OVERRIDE,
            ABOVE,
            BELOW
        };

        // coordinates can be defaulted only if the file name contains the coordinates like in the default xaero's map saves
        void addRegion(const std::filesystem::path& file,
                std::int32_t regionX = std::numeric_limits<std::int32_t>::min(),
                std::int32_t regionZ = std::numeric_limits<std::int32_t>::min(),
                MergeType merge=MergeType::OVERRIDE);
        void addRegion(const std::string_view& data, std::int32_t regionX, std::int32_t regionZ, MergeType merge=MergeType::OVERRIDE);
        void addRegion(std::istream& data, std::int32_t regionX, std::int32_t regionZ, MergeType merge=MergeType::OVERRIDE);

        // so this WILL modify the region passed if MergeType::BELOW, so just don't use the region after adding it :)
        template<typename T> // for forwarding business
        void addRegion(T&& region, std::int32_t regionX, std::int32_t regionZ, MergeType merge=MergeType::OVERRIDE);

        template<typename T>
        void addPixel(T&& pixel, std::int32_t x, std::int32_t z);
        template<typename T>
        void addChunk(T&& chunk, std::int32_t chunkX, std::int32_t chunkZ);

        [[nodiscard]] std::optional<RegionImage> generateImage(std::int32_t regionX, std::int32_t regionZ) const;
        [[nodiscard]] std::vector<std::pair<std::pair<std::int32_t, std::int32_t>, RegionImage>> generateImages() const;

        [[nodiscard]] const Region::TileChunk::Chunk* getChunk(std::int32_t chunkX, std::int32_t chunkZ) const;
        [[nodiscard]] Region::TileChunk::Chunk* getChunk(std::int32_t chunkX, std::int32_t chunkZ);

        [[nodiscard]] const Region::TileChunk::Chunk::Pixel* getPixel(std::int32_t x, std::int32_t z) const;
        [[nodiscard]] Region::TileChunk::Chunk::Pixel* getPixel(std::int32_t x, std::int32_t z);

        bool writeRegion(std::int32_t regionX, std::int32_t regionZ, const std::filesystem::path& path) const;
        [[nodiscard]] std::string getSerialized(std::int32_t regionX, std::int32_t regionZ) const;

        bool writeRegions(const std::filesystem::path& rootPath) const;
        [[nodiscard]] std::vector<std::pair<std::pair<std::int32_t, std::int32_t>, std::string>> getSerialized() const;
    };

    template<typename T>
    void Map::addRegion(T &&region, const std::int32_t regionX, const std::int32_t regionZ, const MergeType merge) {
        const auto contained = find({regionX, regionZ});

        if (contained == end()) {
            emplace(std::pair{regionX, regionZ}, std::forward<T>(region));
            return;
        }

        switch (merge) {
            case MergeType::OVERRIDE:
                contained->second = std::forward<T>(region);
                break;
            case MergeType::ABOVE:
                contained->second.mergeMove(region);
                break;
            case MergeType::BELOW:
                region.mergeMove(contained->second);
                contained->second = std::forward<T>(region);
                break;
        }
    }

    template<typename T>
    void Map::addPixel(T &&pixel, const std::int32_t x, const std::int32_t z) {
        std::int32_t regionX = x >> 9;
        std::int32_t regionZ = z >> 9;

        if (const auto contained = find({regionX, regionZ});
            contained == end()) {
            Region region;

            auto& tileChunk = region[x >> 6 & 7][z >> 6 & 7];
            tileChunk.allocateChunks();
            auto& chunk = tileChunk[x >> 4 & 3][z >> 4 & 3];
            chunk.allocateColumns();
            chunk[x & 15][z & 15] = std::forward<T>(pixel);

            emplace(std::pair{regionX, regionZ}, std::move(region));
        } else {
            auto& tileChunk = contained->second[x >> 6 & 7][z >> 6 & 7];
            tileChunk.allocateChunks();
            auto& chunk = tileChunk[x >> 4 & 3][z >> 4 & 3];
            chunk.allocateColumns();

            chunk[x & 15][z & 15] = std::forward<T>(pixel);
        }
    }

    template<typename T>
    void Map::addChunk(T &&chunk, const std::int32_t chunkX, const std::int32_t chunkZ) {
        std::int32_t regionX = chunkX >> 5;
        std::int32_t regionZ = chunkZ >> 5;

        if (const auto contained = find({regionX, regionZ});
            contained == end()) {
            Region region;

            auto& tileChunk = region[chunkX >> 2 & 7][chunkZ >> 2 & 7];
            tileChunk.allocateChunks();
            tileChunk[chunkX & 3][chunkZ & 3] = std::forward<T>(chunk);

            emplace(std::pair{regionX, regionZ}, std::move(region));
        } else {
            auto& tileChunk = contained->second[chunkX >> 2 & 7][chunkZ >> 2 & 7];
            tileChunk.allocateChunks();
            tileChunk[chunkX & 3][chunkZ & 3] = std::forward<T>(chunk);
        }
    }
}
