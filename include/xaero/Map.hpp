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
        void addRegion(Region&& region, std::int32_t regionX, std::int32_t regionZ, MergeType merge=MergeType::OVERRIDE);

        void addPixel(Region::TileChunk::Chunk::Pixel&& pixel, std::int32_t x, std::int32_t z);
        void addChunk(Region::TileChunk::Chunk&& chunk, std::int32_t chunkX, std::int32_t chunkZ);

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
}
