#pragma once
#include <filesystem>
#include <list>

namespace xaero {
    struct RegionImage;
    struct Region;

    class Parser {
    public:
        static inline Region parseRegion(const std::filesystem::path& file);
        static inline Region parseRegion(const std::string& data);
        static inline Region parseRegion(const std::string_view& data);

        static inline RegionImage generateImage(const Region& region);
        static inline RegionImage generateImage(const std::filesystem::path& file);
        static inline RegionImage generateImage(const std::string& data);
        static inline RegionImage generateImage(const std::string_view& data);

        void addRegion(const std::filesystem::path& file);
        void addRegion(const std::string& data);
        void addRegion(const std::string_view& data);

        void clearRegions();
        void removeRegion(int x, int z);

        [[nodiscard]] RegionImage generateImage(int x, int z) const;
        [[nodiscard]] std::list<RegionImage> generateImages() const;

        [[nodiscard]] Region getParsedRegion(int x, int z) const;
        [[nodiscard]] std::list<Region> getParsedRegions() const;
    };
}
