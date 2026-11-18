#include "simplnx/Core/Application.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>

#include <limits>
#include <stdexcept>
#include <tuple>

/**
 * @brief Test every currently compiled filters instantiation.
 */
TEST_CASE("Filter List Instantiation")
{
  UnitTest::LoadPlugins();
  auto* filterList = nx::core::Application::Instance()->getFilterList();
  REQUIRE(filterList != nullptr);

  const auto& handles = filterList->getFilterHandles();
  REQUIRE(!handles.empty());

  nx::core::DataStructure dataStructure;

  for(const auto& handle : handles)
  {
    auto filter = filterList->createFilter(handle);
    REQUIRE(filter != nullptr);

    UNSCOPED_INFO(fmt::format("Filter List Instantiation::{} Instantiation. [simplnx][{}]", filter->className(), filter->name()));

    nx::core::Arguments args;
    dataStructure.clear();

    auto params = filter->parameters();
    for(const auto& [name, param] : params)
    {
      args.insert(name, param->defaultValue());
    }

    REQUIRE(!filter->defaultTags().empty());

    auto preflightResult = filter->preflight(dataStructure, args);
    REQUIRE((preflightResult.outputActions.valid() || preflightResult.outputActions.invalid())); // Preflight exists
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("Numeric Parameter Defaults", "[simplnx][Filter][Parameters]")
{
  using namespace nx::core;

  UnitTest::LoadPlugins();
  const auto* filterList = Application::Instance()->getFilterList();
  const auto handles = filterList->getFilterHandles();
  REQUIRE(!handles.empty());

  const auto isNumericParameter = []<class... T>(const Uuid& uuid, std::tuple<T...>*) {
    return ((uuid == ParameterTraits<NumberParameter<T>>::uuid || uuid == ParameterTraits<VectorParameter<T>>::uuid) || ...);
  };
  using NumericTypes = std::tuple<int8, uint8, int16, uint16, int32, uint32, int64, uint64, float32, float64>;
  usize checked = 0;
  for(const auto& handle : handles)
  {
    auto filter = filterList->createFilter(handle);
    REQUIRE(filter != nullptr);
    for(const auto& [key, param] : filter->parameters())
    {
      if(isNumericParameter(param->uuid(), static_cast<NumericTypes*>(nullptr)))
      {
        INFO(filter->className() << " / " << key);
        const auto& valueParameter = dynamic_cast<const ValueParameter&>(param.getRef());
        const auto result = valueParameter.validate(valueParameter.defaultValue());
        SIMPLNX_RESULT_REQUIRE_VALID(result);
        ++checked;
      }
    }
  }
  REQUIRE(checked > 0);
}

namespace
{
void RequireValid(const nx::core::Result<>& result)
{
  SIMPLNX_RESULT_REQUIRE_VALID(result);
}

void RequireCode(const nx::core::Result<>& result, nx::core::int32 code)
{
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors().front().code == code);
}
} // namespace

TEST_CASE("Number Parameter Value Bounds", "[simplnx][Parameters]")
{
  using namespace nx::core;

  Float32Parameter spacing("spacing", "Hatch Spacing", "", 1.0F, Float32Parameter::Bounds::Positive());
  CHECK(spacing.bounds()->lower->value == 0.0F);
  CHECK_FALSE(spacing.bounds()->lower->inclusive);
  RequireValid(spacing.validateNumber(std::numeric_limits<float32>::min()));
  const auto zero = spacing.validateNumber(0.0F);
  RequireCode(zero, k_NumberBelowLowerBound);
  CHECK(zero.errors().front().message == "Hatch Spacing must be greater than 0. The value is 0.");
  RequireCode(dynamic_cast<const ValueParameter&>(*spacing.clone()).validate(0.0F), k_NumberBelowLowerBound);

  Int32Parameter count("count", "Count", "", 2, Int32Parameter::Bounds::Between(1, 3));
  RequireValid(count.validateNumber(1));
  RequireValid(count.validateNumber(3));
  RequireCode(count.validateNumber(0), k_NumberBelowLowerBound);
  RequireCode(count.validateNumber(4), k_NumberAboveUpperBound);
  UInt8Parameter limit("limit", "Limit", "", 1, UInt8Parameter::Bounds::LessThan(3));
  RequireValid(limit.validateNumber(2));
  RequireCode(limit.validateNumber(3), k_NumberAboveUpperBound);

  Float64Parameter unbounded("number", "Number", "", 0.0);
  CHECK_FALSE(unbounded.bounds().has_value());
  for(const auto value : {std::numeric_limits<float64>::quiet_NaN(), std::numeric_limits<float64>::infinity(), -std::numeric_limits<float64>::infinity()})
  {
    RequireCode(unbounded.validateNumber(value), k_NumberNotFinite);
    RequireCode(spacing.validateNumber(static_cast<float32>(value)), k_NumberNotFinite);
  }
  Float64Parameter::Bounds allowNonFinite;
  allowNonFinite.setAllowNonFinite(true);
  Float64Parameter special("special", "Special Value", "", 0.0, allowNonFinite);
  RequireValid(special.validateNumber(std::numeric_limits<float64>::infinity()));
}

TEST_CASE("Vector Parameter Value Bounds", "[simplnx][Parameters]")
{
  using namespace nx::core;

  VectorFloat32Parameter named("spacing", "Spacing", "", {1.0F, 1.0F}, {"X", "Y"}, VectorFloat32Parameter::Bounds::NonNegative());
  RequireValid(named.validateVector({0.0F, 2.0F}));
  const auto negative = named.validateVector({1.0F, -0.5F});
  RequireCode(negative, k_NumberBelowLowerBound);
  CHECK(negative.errors().front().message == "Spacing (Y) must be greater than or equal to 0. The value is -0.5.");
  const auto clone = named.clone();
  RequireCode(dynamic_cast<const ValueParameter&>(*clone).validate(VectorFloat32Parameter::ValueType{1.0F, -0.5F}), k_NumberBelowLowerBound);
  RequireCode(named.validateVector({1.0F}), -1);

  REQUIRE_THROWS_AS(VectorFloat32Parameter("spacing", "Spacing", "", {1.0F}, {}), std::runtime_error);

  VectorInt32Parameter unnamed("dimensions", "Dimensions", "", {1, 1}, VectorInt32Parameter::Bounds::AtMost(3));
  const auto above = unnamed.validateVector({3, 4});
  RequireCode(above, k_NumberAboveUpperBound);
  CHECK(above.errors().front().message == "Dimensions (element 1) must be less than or equal to 3. The value is 4.");

  VectorFloat64Parameter unbounded("origin", "Origin", "", {0.0, 0.0});
  for(const auto value : {std::numeric_limits<float64>::quiet_NaN(), std::numeric_limits<float64>::infinity(), -std::numeric_limits<float64>::infinity()})
  {
    RequireCode(unbounded.validateVector({0.0, value}), k_NumberNotFinite);
  }
}
