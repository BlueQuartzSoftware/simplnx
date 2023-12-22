#include "simplnx/Common/Bit.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

TEST_CASE("BitTest")
{
  SECTION("int8 byteswap")
  {
    constexpr int8 original = 0xAB;
    constexpr int8 expected = 0xAB;
    constexpr int8 swapped = byteswap(original);
    REQUIRE(swapped == expected);
  }
  SECTION("uint8 byteswap")
  {
    constexpr uint8 original = 0x12u;
    constexpr uint8 expected = 0x12u;
    constexpr uint8 swapped = byteswap(original);
    REQUIRE(swapped == expected);
  }
  SECTION("int16 byteswap")
  {
    constexpr int16 original = 0xABCD;
    constexpr int16 expected = 0xCDAB;
    constexpr int16 swapped = byteswap(original);
    REQUIRE(swapped == expected);
  }
  SECTION("uint16 byteswap")
  {
    constexpr uint16 original = 0x1234u;
    constexpr uint16 expected = 0x3412u;
    constexpr uint16 swapped = byteswap(original);
    REQUIRE(swapped == expected);
  }
  SECTION("int32 byteswap")
  {
    constexpr int32 original = 0xABCDEF01;
    constexpr int32 expected = 0x01EFCDAB;
    constexpr int32 swapped = byteswap(original);
    REQUIRE(swapped == expected);
  }
  SECTION("uint32 byteswap")
  {
    constexpr uint32 original = 0x12345678u;
    constexpr uint32 expected = 0x78563412u;
    constexpr uint32 swapped = byteswap(original);
    REQUIRE(swapped == expected);
  }
  SECTION("int64 byteswap")
  {
    constexpr int64 original = 0xABCDEF1234567890;
    constexpr int64 expected = 0x9078563412EFCDAB;
    constexpr int64 swapped = byteswap(original);
    REQUIRE(swapped == expected);
  }
  SECTION("uint64 byteswap")
  {
    constexpr uint64 original = 0x1234567812345678ull;
    constexpr uint64 expected = 0x7856341278563412ull;
    constexpr uint64 swapped = byteswap(original);
    REQUIRE(swapped == expected);
  }
}
