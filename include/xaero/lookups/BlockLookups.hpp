#pragma once

#include "../types/LookupTypes.hpp"

// this is done with an ifdef and without code gen because I want to be able to include the ./include and have every header that could possibly be referenced
#ifdef XAERO_DEFAULT_LOOKUPS

namespace xaero {
    extern const StateLookup defaultStateLookup;

    extern const BiomeLookup defaultBiomeLookup;

    extern const LookupPack defaultLookupPack;
}

#endif
