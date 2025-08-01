#include <xaero/lookups/BlockLookups.hpp>

#ifdef XAERO_DEFAULT_LOOKUPS
[[maybe_unused]] const xaero::LookupPack xaero::defaultLookupPack = {
    defaultStateLookup,
    defaultStateIDLookup,
    defaultStateIDLookupSize,
    defaultBiomeLookup
};
#endif
