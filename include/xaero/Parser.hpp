#pragma once
#include <filesystem>
#include <list>
#include "xaero/util/Region.hpp"

namespace xaero {
    struct RegionImage;

    class Parser {
    public:
        /**
         * @param file path to xaero zip file, please do not unzip!
         * @return parsed region
         */
        static Region parseRegion(const std::filesystem::path& file);
        static Region parseRegion(const std::string& data);
        static Region parseRegion(const std::string_view& data);
        static Region parseRegion(std::istream& data);

        static RegionImage generateImage(const Region& region);
        static RegionImage generateImage(const std::filesystem::path& file);
        static RegionImage generateImage(const std::string& data);
        static RegionImage generateImage(const std::string_view& data);
        static RegionImage generateImage(std::istream& data);

        void addRegion(const std::filesystem::path& file);
        void addRegion(const std::string& data);
        void addRegion(const std::string_view& data);
        void addRegion(std::istream& data);

        void clearRegions();
        void removeRegion(int x, int z);

        [[nodiscard]] RegionImage generateImage(int x, int z) const;
        [[nodiscard]] std::list<RegionImage> generateImages() const;

        [[nodiscard]] Region getParsedRegion(int x, int z) const;
        [[nodiscard]] std::list<Region> getParsedRegions() const;
    };
}
