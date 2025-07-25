#pragma once
#include <filesystem>
#include <list>

#include "types/LookupTypes.hpp"
#include "types/Region.hpp"

namespace xaero {
    struct RegionImage;

    class Map {
    public:
        /**
         * @param file path to xaero zip file, please do not unzip!
         * @return parsed region
         */
        static Region parseRegion(const std::filesystem::path& file);
        static Region parseRegion(const std::span<std::uint8_t>& data);
        static Region parseRegion(std::istream& data);

        static std::string serializeRegion(const Region& region, const LookupPack& lookups);
        static std::string packRegion(const std::vector<std::uint8_t>& serialized);

        static RegionImage generateImage(const Region& region);
        static RegionImage generateImage(const std::filesystem::path& file);
        static RegionImage generateImage(const std::span<std::uint8_t>& data);
        static RegionImage generateImage(std::istream& data);

        void addRegion(const std::filesystem::path& file);
        void addRegion(const std::span<std::uint8_t>& data);
        void addRegion(std::istream& data);
        void addRegion(const Region region);

        void addPixel(const Region::TileChunk::Chunk::Pixel pixel, std::int32_t x, std::int32_t z);
        void addChunk(const Region::TileChunk::Chunk chunk, std::int32_t chunkX, std::int32_t chunkZ);

        void clearRegions();
        void removeRegion(std::int32_t x, std::int32_t z);

        [[nodiscard]] RegionImage generateImage(std::int32_t x, std::int32_t z) const;
        [[nodiscard]] std::list<RegionImage> generateImages() const;

        [[nodiscard]] Region getParsedRegion(std::int32_t x, std::int32_t z) const;
        [[nodiscard]] std::list<Region> getParsedRegions() const;
    };
}
