#include "simplnx/Common/Bit.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

TEST_CASE("BitTest")
{
  SECTION("int8 byteswap")
  {
    constexpr int8 k_Original = 0xAB;
    constexpr int8 k_Expected = 0xAB;
    constexpr int8 k_Swapped = byteswap(k_Original);
    REQUIRE(k_Swapped == k_Expected);
  }
  SECTION("uint8 byteswap")
  {
    constexpr uint8 k_Original = 0x12u;
    constexpr uint8 k_Expected = 0x12u;
    constexpr uint8 k_Swapped = byteswap(k_Original);
    REQUIRE(k_Swapped == k_Expected);
  }
  SECTION("int16 byteswap")
  {
    constexpr int16 k_Original = 0xABCD;
    constexpr int16 k_Expected = 0xCDAB;
    constexpr int16 k_Swapped = byteswap(k_Original);
    REQUIRE(k_Swapped == k_Expected);
  }
  SECTION("uint16 byteswap")
  {
    constexpr uint16 k_Original = 0x1234u;
    constexpr uint16 k_Expected = 0x3412u;
    constexpr uint16 k_Swapped = byteswap(k_Original);
    REQUIRE(k_Swapped == k_Expected);
  }
  SECTION("int32 byteswap")
  {
    constexpr int32 k_Original = 0xABCDEF01;
    constexpr int32 k_Expected = 0x01EFCDAB;
    constexpr int32 k_Swapped = byteswap(k_Original);
    REQUIRE(k_Swapped == k_Expected);
  }
  SECTION("uint32 byteswap")
  {
    constexpr uint32 k_Original = 0x12345678u;
    constexpr uint32 k_Expected = 0x78563412u;
    constexpr uint32 k_Swapped = byteswap(k_Original);
    REQUIRE(k_Swapped == k_Expected);
  }
  SECTION("int64 byteswap")
  {
    constexpr int64 k_Original = 0xABCDEF1234567890;
    constexpr int64 k_Expected = 0x9078563412EFCDAB;
    constexpr int64 k_Swapped = byteswap(k_Original);
    REQUIRE(k_Swapped == k_Expected);
  }
  SECTION("uint64 byteswap")
  {
    constexpr uint64 k_Original = 0x1234567812345678ull;
    constexpr uint64 k_Expected = 0x7856341278563412ull;
    constexpr uint64 k_Swapped = byteswap(k_Original);
    REQUIRE(k_Swapped == k_Expected);
  }
}
