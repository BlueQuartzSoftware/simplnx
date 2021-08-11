#include <catch2/catch.hpp>

#include "complex/Common/Bit.hpp"

using namespace complex;

TEST_CASE("BitTest")
{
  SECTION("i8 byteswap")
  {
    constexpr i8 original = 0xAB;
    constexpr i8 expected = 0xAB;
    constexpr i8 swapped = byteswap(original);
    REQUIRE(swapped == expected);
  }
  SECTION("u8 byteswap")
  {
    constexpr u8 original = 0x12u;
    constexpr u8 expected = 0x12u;
    constexpr u8 swapped = byteswap(original);
    REQUIRE(swapped == expected);
  }
  SECTION("i16 byteswap")
  {
    constexpr i16 original = 0xABCD;
    constexpr i16 expected = 0xCDAB;
    constexpr i16 swapped = byteswap(original);
    REQUIRE(swapped == expected);
  }
  SECTION("u16 byteswap")
  {
    constexpr u16 original = 0x1234u;
    constexpr u16 expected = 0x3412u;
    constexpr u16 swapped = byteswap(original);
    REQUIRE(swapped == expected);
  }
  SECTION("i32 byteswap")
  {
    constexpr i32 original = 0xABCDEF01;
    constexpr i32 expected = 0x01EFCDAB;
    constexpr i32 swapped = byteswap(original);
    REQUIRE(swapped == expected);
  }
  SECTION("u32 byteswap")
  {
    constexpr u32 original = 0x12345678u;
    constexpr u32 expected = 0x78563412u;
    constexpr u32 swapped = byteswap(original);
    REQUIRE(swapped == expected);
  }
  SECTION("i64 byteswap")
  {
    constexpr i64 original = 0xABCDEF1234567890;
    constexpr i64 expected = 0x9078563412EFCDAB;
    constexpr i64 swapped = byteswap(original);
    REQUIRE(swapped == expected);
  }
  SECTION("u64 byteswap")
  {
    constexpr u64 original = 0x1234567812345678ull;
    constexpr u64 expected = 0x7856341278563412ull;
    constexpr u64 swapped = byteswap(original);
    REQUIRE(swapped == expected);
  }
}
