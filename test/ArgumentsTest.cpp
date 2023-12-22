#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Common/StringLiteral.hpp"

#include <catch2/catch.hpp>

#include <string>

using namespace nx::core;

TEST_CASE("ArgumentsTest")
{
  static constexpr StringLiteral k_FooKey = "foo";
  Arguments args;

  SECTION("insert and retrieve primitive")
  {
    static constexpr int32 k_FooValue = 42;

    args.insert(k_FooKey, k_FooValue);

    auto value = args.value<int32>(k_FooKey);

    REQUIRE(value == k_FooValue);
  }
  SECTION("insert and retrieve class")
  {
    const std::string k_FooValue = "value";

    args.insert(k_FooKey, k_FooValue);

    auto value = args.value<std::string>(k_FooKey);

    REQUIRE(value == k_FooValue);
  }
  SECTION("valueRef test")
  {
    const std::string k_FooValue = "value";

    args.insert(k_FooKey, k_FooValue);

    const auto& value = args.valueRef<std::string>(k_FooKey);

    REQUIRE(value == k_FooValue);
  }
  SECTION("ref test")
  {
    int32 value = 42;

    args.insert(k_FooKey, std::ref(value));

    auto& valueRefWrapper = args.ref<int32>(k_FooKey);

    REQUIRE(&value == &valueRefWrapper);
    REQUIRE(value == valueRefWrapper);

    value += 1;

    REQUIRE(value == valueRefWrapper);
  }
  SECTION("const ref test")
  {
    int32 value = 42;

    args.insert(k_FooKey, std::cref(value));

    const auto& valueRefWrapper = args.ref<const int32>(k_FooKey);

    REQUIRE(&value == &valueRefWrapper);
    REQUIRE(value == valueRefWrapper);
  }
  SECTION("contains test")
  {
    static constexpr int32 k_Value = 42;

    args.insert(k_FooKey, k_Value);

    REQUIRE(args.contains(k_FooKey));
  }
  SECTION("not contains test")
  {
    REQUIRE(!args.contains(k_FooKey));
  }
  SECTION("size test")
  {
    args.insert("foo", 42);
    args.insert("bar", 3.2f);
    args.insert("baz", std::string("baz"));

    REQUIRE(args.size() == 3);
  }
  SECTION("empty test")
  {
    REQUIRE(args.empty());
  }
  SECTION("iterate")
  {
    args.insert("foo", 42);
    args.insert("bar", 3.2f);
    args.insert("baz", std::string("baz"));

    for(auto&& [key, value] : args)
    {
      REQUIRE(!key.empty());
      REQUIRE(value.has_value());
    }
  }
}
