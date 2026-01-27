#pragma once

#include <filesystem>
#include <string_view>

#include "lookups/BlockLookups.hpp"

namespace xaero {
    struct Region;

    /**
     * @param file path to xaero zip file, please do not unzip!
     * @param lookups lookup pack to avoid unnecessary copies, can reduce memory usage very significantly
     * @return parsed region
     */
    Region parseRegion(const std::filesystem::path& file, const LookupPack* lookups
#ifdef XAERO_DEFAULT_LOOKUPS
    = &defaultLookupPack
#endif
        );

    /**
     * @param data unzipped data at the entry "region.xaero" in a xaero region file
     * @param lookups lookup pack to avoid unnecessary copies, can reduce memory usage very significantly
     * @return parsed region
     */
    Region parseRegion(const std::string_view& data, const LookupPack* lookups
#ifdef XAERO_DEFAULT_LOOKUPS
    = &defaultLookupPack
#endif
        );

    /**
     * @param data data stream to the entry "region.xaero" in a xaero region file
     * @param lookups lookup pack to avoid unnecessary copies, can reduce memory usage very significantly
     * @return parsed region
     */
    Region parseRegion(std::istream& data, const LookupPack* lookups
#ifdef XAERO_DEFAULT_LOOKUPS
    = &defaultLookupPack
#endif
        );

    // lookups CAN be nullptr, it just means XaeroMapFormat *WILL NOT* check if your properties in BlockStates are correct, meaning the files might not end up being readable if you've made any mistakes.
    std::string serializeRegion(const Region& region, const LookupPack *lookups);

    std::string packRegion(const std::string_view &serialized);

    // lookups CAN be nullptr, it just means XaeroMapFormat *WILL NOT* check if your properties in BlockStates are correct, meaning the files might not end up being readable if you've made any mistakes.
    bool writeRegion(const Region& region, const std::filesystem::path& path, const LookupPack *lookups);

    bool writeRegion(const std::string_view &serialized, const std::filesystem::path &path);

    RegionImage generateImage(const Region& region, const LookupPack *lookups
#ifdef XAERO_DEFAULT_LOOKUPS
    = &defaultLookupPack
#endif
    );

    RegionImage generateImage(const std::filesystem::path& file, const LookupPack *lookups
#ifdef XAERO_DEFAULT_LOOKUPS
    = &defaultLookupPack
#endif
    );
    /**
     * @param data unzipped data at the entry "region.xaero" in a xaero region file
     * @param lookups color lookups
     * @return rendered region
     */
    RegionImage generateImage(const std::string_view& data, const LookupPack *lookups
#ifdef XAERO_DEFAULT_LOOKUPS
    = &defaultLookupPack
#endif
    );
    /**
     * @param data data stream to the entry "region.xaero" in a xaero region file
     * @param lookups color lookups
     * @return rendered region
     */
    RegionImage generateImage(std::istream& data, const LookupPack *lookups
#ifdef XAERO_DEFAULT_LOOKUPS
    = &defaultLookupPack
#endif
    );
}
