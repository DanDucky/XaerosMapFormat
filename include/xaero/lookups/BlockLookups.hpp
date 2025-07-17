#pragma once

#ifdef XAERO_DEFAULT_LOOKUPS

#include "LookupTypes.hpp"

namespace xaero {
    extern const StateLookup defaultStateLookup;

    extern const DefaultStateIDLookup defaultStateIDLookup;

    // if you need to do a massive copy of the data in the lookup, these variables will be useful in extracting it
    // the ability to do this is there, but I haven't put any other infrastructure because I don't want to commit
    // this project to having support for map data from modpacks
    extern const std::size_t defaultStateIDLookupSize;
    extern const std::size_t defaultStateIDLookupChunkSize;
}

#endif
