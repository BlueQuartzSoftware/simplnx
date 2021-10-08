#include <catch2/catch.hpp>

#include "complex/Core/Parameters/BoolParameter.hpp"
#include "complex/Core/Parameters/NumberParameter.hpp"
#include "complex/Core/Parameters/StringParameter.hpp"
#include "complex/Filter/Parameters.hpp"

using namespace complex;

namespace
{
constexpr StringLiteral k_FooParamKey = "FooParam";
constexpr StringLiteral k_BarParamKey = "BarParam";
constexpr StringLiteral k_BazParamKey = "BazParam";
} // namespace

TEST_CASE("ParametersTest")
{
  Parameters params;
  params.insert(std::make_unique<Int32Parameter>(k_FooParamKey.str(), "Foo", "Test parameter", 42));
  params.insertSeparator({"Separator Name"});

  REQUIRE_FALSE(params.containsGroup(k_BarParamKey.view()));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_BarParamKey.str(), "Bar", "Test parameter", false));
  REQUIRE(params.containsGroup(k_BarParamKey.view()));

  params.insert(std::make_unique<StringParameter>(k_BazParamKey.str(), "Baz", "Test parameter", "default"));

  params.linkParameters(k_BarParamKey.str(), k_BazParamKey.str(), true);

  REQUIRE(params.size() == 3);

  REQUIRE(params.getLayout().size() == 4);

  REQUIRE(std::holds_alternative<Parameters::ParameterKey>(params.getLayout().at(0)));
  REQUIRE(std::holds_alternative<Parameters::Separator>(params.getLayout().at(1)));

  REQUIRE_FALSE(params.hasGroup(k_FooParamKey.view()));
  REQUIRE(params.hasGroup(k_BazParamKey.view()));

  REQUIRE(params.getGroup(k_FooParamKey.view()).empty());
  REQUIRE(params.getGroup(k_BazParamKey.view()) == k_BarParamKey.view());

  const std::vector<std::string> expectedKeys = {k_FooParamKey.str(), k_BarParamKey.str(), k_BazParamKey.str()};

  REQUIRE(params.getKeys() == expectedKeys);

  const std::vector<std::string> expectedGroupKeys = {k_BarParamKey.str()};

  REQUIRE(params.getGroupKeys() == expectedGroupKeys);

  const std::vector<std::string> expectedKeyInGroup = {k_BazParamKey.str()};

  REQUIRE(params.getKeysInGroup(k_BarParamKey.view()) == expectedKeyInGroup);

  REQUIRE(params.isParameterActive(k_FooParamKey.view(), {}));

  REQUIRE(params.isParameterActive(k_BazParamKey.view(), true));
  REQUIRE_FALSE(params.isParameterActive(k_BazParamKey.view(), false));
}
