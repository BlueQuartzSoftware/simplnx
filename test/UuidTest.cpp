#include <catch2/catch.hpp>

#include "complex/Common/Uuid.hpp"

using namespace complex;

TEST_CASE("UuidTest")
{
  constexpr const char k_UuidString[] = "b6936d18-7476-4855-9e13-e795d717c50f";
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
