#pragma once
#include <filesystem>
#include <list>
#include <span>

#include "lookups/BlockLookups.hpp"
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
        /**
         * @param data unzipped data at the entry "region.xaero" in a xaero region file
         * @return parsed region
         */
        static Region parseRegion(const std::span<char>& data);
        /**
         * @param data data stream to the entry "region.xaero" in a xaero region file
         * @return parsed region
         */
        static Region parseRegion(std::istream& data);

        static std::string serializeRegion(const Region& region, const LookupPack& lookups
#ifdef XAERO_DEFAULT_LOOKUPS
        ={defaultStateLookup, defaultStateIDLookup, defaultStateIDLookupSize}
#endif
        );
        static std::string packRegion(const std::span<std::uint8_t>& serialized);
        static void writeRegion(const Region& region, const std::filesystem::path& path, const LookupPack& lookups
#ifdef XAERO_DEFAULT_LOOKUPS
        ={defaultStateLookup, defaultStateIDLookup, defaultStateIDLookupSize}
#endif
        );

        static void writeRegion(const std::span<std::uint8_t>& serialized, const std::filesystem::path& path);

        /**
         * @param region region to render
         * @return rendered region
         */
        static RegionImage generateImage(const Region& region);
        /**
         * @param file path to xaero zip file, please do not unzip!
         * @return rendered region
         */
        static RegionImage generateImage(const std::filesystem::path& file);
        /**
         * @param data unzipped data at the entry "region.xaero" in a xaero region file
         * @return rendered region
         */
        static RegionImage generateImage(const std::span<char>& data);
        /**
         * @param data data stream to the entry "region.xaero" in a xaero region file
         * @return rendered region
         */
        static RegionImage generateImage(std::istream& data);

        enum class MergeType : std::uint8_t {
            OVERRIDE,
            ABOVE,
            BELOW
        };

        void addRegion(const std::filesystem::path& file, MergeType merge=MergeType::OVERRIDE);
        void addRegion(const std::span<std::uint8_t>& data, MergeType merge=MergeType::OVERRIDE);
        void addRegion(std::istream& data, MergeType merge=MergeType::OVERRIDE);
        void addRegion(const Region region, MergeType merge=MergeType::OVERRIDE);

        void addPixel(const Region::TileChunk::Chunk::Pixel pixel, std::int32_t x, std::int32_t z);
        void addChunk(const Region::TileChunk::Chunk chunk, std::int32_t chunkX, std::int32_t chunkZ);

        void clearRegions();
        void removeRegion(std::int32_t x, std::int32_t z);
        void removeChunk(std::int32_t x, std::int32_t z);

        [[nodiscard]] RegionImage generateImage(std::int32_t x, std::int32_t z) const;
        [[nodiscard]] std::list<RegionImage> generateImages() const;

        [[nodiscard]] const Region* getRegion(std::int32_t x, std::int32_t z) const;
        [[nodiscard]] Region* getRegion(std::int32_t x, std::int32_t z);

        [[nodiscard]] const Region::TileChunk::Chunk* getChunk(std::int32_t x, std::int32_t z) const;
        [[nodiscard]] Region::TileChunk::Chunk* getChunk(std::int32_t x, std::int32_t z);

        [[nodiscard]] const Region::TileChunk::Chunk::Pixel* getPixel(std::int32_t x, std::int32_t z) const;
        [[nodiscard]] Region::TileChunk::Chunk::Pixel* getPixel(std::int32_t x, std::int32_t z);

        [[nodiscard]] std::list<Region> getParsedRegions() const;
    };
}
