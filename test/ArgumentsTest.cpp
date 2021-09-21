#include <string>

#include <catch2/catch.hpp>

#include <complex/Common/StringLiteral.hpp>
#include <complex/Filter/Arguments.hpp>

using namespace complex;

TEST_CASE("ArgumentsTest")
{
  static constexpr StringLiteral k_FooKey = "foo";
  Arguments args;

  SECTION("insert and retrieve primitive")
  {
    static constexpr i32 k_FooValue = 42;

    args.insert(k_FooKey.str(), k_FooValue);

    auto value = args.value<i32>(k_FooKey.view());

    REQUIRE(value == k_FooValue);
  }
  SECTION("insert and retrieve class")
  {
    const std::string k_FooValue = "value";

    args.insert(k_FooKey.str(), k_FooValue);

    auto value = args.value<std::string>(k_FooKey.view());

    REQUIRE(value == k_FooValue);
  }
  SECTION("valueRef test")
  {
    const std::string k_FooValue = "value";

    args.insert(k_FooKey.str(), k_FooValue);

    const auto& value = args.valueRef<std::string>(k_FooKey.view());

    REQUIRE(value == k_FooValue);
  }
  SECTION("ref test")
  {
    i32 value = 42;

    args.insert(k_FooKey.str(), std::ref(value));

    auto& valueRefWrapper = args.ref<i32>(k_FooKey.view());

    REQUIRE(&value == &valueRefWrapper);
    REQUIRE(value == valueRefWrapper);

    value += 1;

    REQUIRE(value == valueRefWrapper);
  }
  SECTION("const ref test")
  {
    i32 value = 42;

    args.insert(k_FooKey.str(), std::cref(value));

    const auto& valueRefWrapper = args.ref<const i32>(k_FooKey.view());

    REQUIRE(&value == &valueRefWrapper);
    REQUIRE(value == valueRefWrapper);
  }
  SECTION("contains test")
  {
    static constexpr i32 k_Value = 42;

    args.insert(k_FooKey.str(), k_Value);

    REQUIRE(args.contains(k_FooKey.view()));
  }
  SECTION("not contains test")
  {
    REQUIRE(!args.contains(k_FooKey.view()));
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
