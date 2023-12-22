#include "simplnx/Common/Uuid.hpp"
#include "simplnx/Common/StringLiteral.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

TEST_CASE("UuidTest")
{
  SECTION("default")
  {
    constexpr StringLiteral k_UuidString = "b6936d18-7476-4855-9e13-e795d717c50f";
    constexpr std::array<std::byte, 16> k_UuidBytes = {std::byte{0xb6u}, std::byte{0x93u}, std::byte{0x6du}, std::byte{0x18u}, std::byte{0x74u}, std::byte{0x76u}, std::byte{0x48u}, std::byte{0x55u},
                                                       std::byte{0x9eu}, std::byte{0x13u}, std::byte{0xe7u}, std::byte{0x95u}, std::byte{0xd7u}, std::byte{0x17u}, std::byte{0xc5u}, std::byte{0x0fu}};
    std::optional<Uuid> uuid = Uuid::FromString(k_UuidString);

    REQUIRE(uuid.has_value());

    REQUIRE(uuid->data == k_UuidBytes);

    REQUIRE(uuid->time_low() == 0xb6936d18u);
    REQUIRE(uuid->time_mid() == 0x7476u);
    REQUIRE(uuid->time_hi_version() == 0x4855u);
    REQUIRE(uuid->clock_seq_hi_and_res_clock_seq_low() == 0x9e13u);
    REQUIRE(uuid->clock_seq_hi_variant() == 0x9eu);
    REQUIRE(uuid->clock_seq_low() == 0x13u);
    REQUIRE(uuid->node() == 0xe795d717c50full);

    REQUIRE(uuid->str() == k_UuidString);
  }
  SECTION("leading zeros")
  {
    constexpr StringLiteral k_UuidString = "00ab00ff-00ab-0abc-0508-00000ff00000";
    constexpr std::array<std::byte, 16> k_UuidBytes = {std::byte{0x00u}, std::byte{0xabu}, std::byte{0x00u}, std::byte{0xffu}, std::byte{0x00u}, std::byte{0xabu}, std::byte{0x0au}, std::byte{0xbcu},
                                                       std::byte{0x05u}, std::byte{0x08u}, std::byte{0x00u}, std::byte{0x00u}, std::byte{0x0fu}, std::byte{0xf0u}, std::byte{0x00u}, std::byte{0x00u}};
    std::optional<Uuid> uuid = Uuid::FromString(k_UuidString);

    REQUIRE(uuid.has_value());

    REQUIRE(uuid->data == k_UuidBytes);

    REQUIRE(uuid->time_low() == 0x00ab00ffu);
    REQUIRE(uuid->time_mid() == 0x00abu);
    REQUIRE(uuid->time_hi_version() == 0x0abcu);
    REQUIRE(uuid->clock_seq_hi_and_res_clock_seq_low() == 0x0508u);
    REQUIRE(uuid->clock_seq_hi_variant() == 0x05u);
    REQUIRE(uuid->clock_seq_low() == 0x08u);
    REQUIRE(uuid->node() == 0x00000ff00000ull);

    REQUIRE(uuid->str() == k_UuidString);
  }
  SECTION("comparison")
  {
    std::optional<Uuid> uuid1 = Uuid::FromString("c95d4cae-f129-4fb7-8cee-837ec05309c1");
    REQUIRE(uuid1.has_value());
    std::optional<Uuid> uuid2 = Uuid::FromString("cefba6b0-cc46-49a7-85c1-184699faf85f");
    REQUIRE(uuid2.has_value());

    REQUIRE(*uuid1 != *uuid2);
    REQUIRE(*uuid1 < *uuid2);
    REQUIRE(*uuid1 <= *uuid2);
    REQUIRE_FALSE(*uuid1 == *uuid2);
    REQUIRE_FALSE(*uuid1 > *uuid2);
    REQUIRE_FALSE(*uuid1 >= *uuid2);
  }
}
