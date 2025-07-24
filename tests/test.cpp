#include <fstream>

#include "../src/util/ByteInputStream.hpp"
#include "catch2/catch_test_macros.hpp"
#include <iostream>

#include <xaero/util/IndexableView.hpp>
#include <xaero/Parser.hpp>

#ifdef XAERO_DEFAULT_LOOKUPS
#   include <xaero/lookups/BlockLookups.hpp>
#endif

#ifdef XAERO_DEFAULT_LOOKUPS
TEST_CASE("BlockLookups", "[BlockLookups]") {
    const xaero::IndexableView<const xaero::StateIDLookupElement&> wrapper = {xaero::defaultStateIDLookup};
    for (int i = 0; i < xaero::defaultStateIDLookupSize; i++) {
        if (xaero::defaultStateIDLookup[i].has_value()) {
            REQUIRE(wrapper[i]->name == xaero::defaultStateIDLookup[i]->name);
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