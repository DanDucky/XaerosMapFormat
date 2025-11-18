#include "xaero/types/Overlay.hpp"

#include "util/VariantUtils.hpp"

const xaero::BlockState& xaero::Overlay::getState() const {
    return getStateForUnion(state);
}
