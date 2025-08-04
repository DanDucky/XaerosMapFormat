#include <fstream>
#include <iostream>

#include "../src/util/ByteInputStream.hpp"
#include "../src/util/ByteOutputStream.hpp"

#include <catch2/catch_test_macros.hpp>

#include <xaero/util/IndexableView.hpp>
#include <xaero/Map.hpp>

#ifdef XAERO_DEFAULT_LOOKUPS
#   include <xaero/lookups/BlockLookups.hpp>
#endif

#ifdef XAERO_DEFAULT_LOOKUPS
TEST_CASE("BlockLookups", "[BlockLookups]") {
    const xaero::IndexableView<const xaero::StateIDLookupElement&> wrapper = {xaero::defaultStateIDLookup};
    const xaero::IndexableView<const xaero::StateIDLookupElement&> hi = {};
    for (int i = 0; i < xaero::defaultStateIDLookupSize; i++) {
        if (xaero::defaultLookupPack.stateIDLookup[i].has_value()) {
            REQUIRE(wrapper[i]->state.name == xaero::defaultLookupPack.stateIDLookup[i].value().state.name);
        }
    }
}
#endif

TEST_CASE("ByteInputSteam", "[ByteInputStream]") {
    auto stringStream = std::istringstream("abcdefg");
    auto stream = xaero::ByteInputStream(stringStream);
    REQUIRE(stream.getNext<char>() == 'a');
    REQUIRE(stream.peekNext<char>() == 'b');
    REQUIRE(stream.getNext<char>() == 'b');
    stream.skip(1);
    REQUIRE(stream.getNext<char>() == 'd');
}