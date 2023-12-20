#include "simplnx/Filter/Parameters.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

namespace
{
constexpr StringLiteral k_FooParamKey = "FooParam";
constexpr StringLiteral k_BarParamKey = "BarParam";
constexpr StringLiteral k_BazParamKey = "BazParam";
constexpr StringLiteral k_BizParamKey = "BizParam";
} // namespace

TEST_CASE("ParametersTest")
{
  Parameters params;
  params.insert(std::make_unique<Int32Parameter>(k_FooParamKey, "Foo", "Test parameter", 42));
  params.insertSeparator({"Separator Name"});

  REQUIRE_FALSE(params.containsGroup(k_BarParamKey));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_BarParamKey, "Bar", "Test parameter", false));
  REQUIRE(params.containsGroup(k_BarParamKey));

  params.insert(std::make_unique<StringParameter>(k_BazParamKey, "Baz", "Test parameter", "default"));

  params.linkParameters(k_BarParamKey, k_BazParamKey, true);

  REQUIRE(params.size() == 3);

  REQUIRE(params.getLayout().size() == 4);

  REQUIRE(std::holds_alternative<Parameters::ParameterKey>(params.getLayout().at(0)));
  REQUIRE(std::holds_alternative<Parameters::Separator>(params.getLayout().at(1)));

  REQUIRE(params.getNumberOfLinkedGroups(k_FooParamKey) == 0);
  REQUIRE(params.getNumberOfLinkedGroups(k_BazParamKey) == 1);

  REQUIRE(params.getLinkedGroups(k_FooParamKey).empty());
  const auto& bazGroups = params.getLinkedGroups(k_BazParamKey);
  REQUIRE(bazGroups.size() == 1);
  REQUIRE(bazGroups.at(0).first == k_BarParamKey);

  const std::vector<std::string> expectedKeys = {k_FooParamKey, k_BarParamKey, k_BazParamKey};

  REQUIRE(params.getKeys() == expectedKeys);

  const std::vector<std::string> expectedGroupKeys = {k_BarParamKey};

  REQUIRE(params.getGroupKeys() == expectedGroupKeys);

  const std::vector<std::string> expectedKeyInGroup = {k_BazParamKey};

  REQUIRE(params.getKeysInGroup(k_BarParamKey) == expectedKeyInGroup);

  REQUIRE(params.isParameterActive(k_FooParamKey, {}));

  Arguments args;
  args.insertOrAssign(k_BarParamKey, true);

  REQUIRE(params.isParameterActive(k_BazParamKey, args));

  args.insertOrAssign(k_BarParamKey, false);

  REQUIRE_FALSE(params.isParameterActive(k_BazParamKey, args));

  // Test multiple linked groups

  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_BizParamKey, "Biz", "Test Parameter", 0, ChoicesParameter::Choices{"Zero", "One", "Two"}));

  REQUIRE(params.size() == 4);

  params.linkParameters(k_BizParamKey, k_BazParamKey, std::make_any<ChoicesParameter::ValueType>(1));

  args.insertOrAssign(k_BarParamKey, false);
  args.insertOrAssign(k_BizParamKey, std::make_any<ChoicesParameter::ValueType>(0));
  REQUIRE_FALSE(params.isParameterActive(k_BazParamKey, args));

  args.insertOrAssign(k_BarParamKey, true);
  args.insertOrAssign(k_BizParamKey, std::make_any<ChoicesParameter::ValueType>(0));
  REQUIRE(params.isParameterActive(k_BazParamKey, args));

  args.insertOrAssign(k_BarParamKey, false);
  args.insertOrAssign(k_BizParamKey, std::make_any<ChoicesParameter::ValueType>(1));
  REQUIRE(params.isParameterActive(k_BazParamKey, args));

  args.insertOrAssign(k_BarParamKey, true);
  args.insertOrAssign(k_BizParamKey, std::make_any<ChoicesParameter::ValueType>(1));
  REQUIRE(params.isParameterActive(k_BazParamKey, args));
}
