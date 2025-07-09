#include "../src/util/ByteInputStream.hpp"
#include "catch2/catch_test_macros.hpp"

TEST_CASE("ByteInputSteam", "[ByteInputStream]") {
    auto stringStream = std::istringstream("abcdefg");
    auto stream = xaero::ByteInputStream(stringStream);
    REQUIRE(stream.getNext<char>() == 'a');
    REQUIRE(stream.peekNext<char>() == 'b');
    REQUIRE(stream.getNext<char>() == 'b');
    stream.skipBits(8);
    REQUIRE(stream.getNext<char>() == 'd');

}