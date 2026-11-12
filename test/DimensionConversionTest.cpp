#include "simplnx/Filter/Actions/CreateStringArrayAction.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <catch2/catch.hpp>

#include <cmath>
#include <limits>
#include <tuple>

using namespace nx::core;

TEST_CASE("simplnx::DimensionConversion: table scalar and product boundaries", "[simplnx][DimensionConversion]")
{
  const float64 upper = std::ldexp(1.0, std::numeric_limits<usize>::digits);
  const std::vector<std::pair<float64, std::string>> invalidValues = {{-1, "below the minimum"},
                                                                      {0, "below the minimum"},
                                                                      {0.5, "below the minimum"},
                                                                      {1.5, "not a whole number"},
                                                                      {std::numeric_limits<float64>::quiet_NaN(), "not finite"},
                                                                      {std::numeric_limits<float64>::infinity(), "not finite"},
                                                                      {-std::numeric_limits<float64>::infinity(), "not finite"},
                                                                      {upper, "exclusive upper bound"}};
  for(const auto& [value, phrase] : invalidValues)
  {
    CAPTURE(value, phrase);
    const std::vector<float64> values = {3, value};
    auto result = ConvertDimensions(values, "Tuple", ZeroDimensionPolicy::Reject);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors()[0].code == DimensionConversion::k_InvalidValue);
    CHECK(result.errors()[0].message.find("index 1") != std::string::npos);
    CHECK(result.errors()[0].message.find(phrase) != std::string::npos);
    CHECK(result.errors()[0].message.find("value ") != std::string::npos);
    CHECK(result.errors()[0].message.find("finite whole numbers in [1,") != std::string::npos);
  }
  const std::vector<float64> valid = {3, 4};
  auto result = ConvertDimensions(valid, "Tuple", ZeroDimensionPolicy::Reject);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  CHECK(result.value() == ShapeType{3, 4});
  const std::vector<float64> largest = {std::nextafter(upper, 0.0)};
  auto boundary = ConvertDimensions(largest, "Tuple", ZeroDimensionPolicy::Reject);
  SIMPLNX_RESULT_REQUIRE_VALID(boundary);
  CHECK(boundary.value()[0] == std::numeric_limits<usize>::max() - usize{2047});
  const std::vector<float64> overflow = {4294967296.0, 4294967296.0};
  auto tooLarge = ConvertDimensions(overflow, "Tuple", ZeroDimensionPolicy::Reject);
  SIMPLNX_RESULT_REQUIRE_INVALID(tooLarge);
  CHECK(tooLarge.errors()[0].code == DimensionConversion::k_ProductOverflow);
  CHECK(tooLarge.errors()[0].message.find("dimensions [Y=4294967296, X=4294967296] have a product") != std::string::npos);
  // Every scalar is checked before multiplication, even after a zero or an overflowing prefix.
  for(const auto& values : std::vector<std::vector<float64>>{{4294967296.0, 4294967296.0, 0}, {0, 4294967296.0, 4294967296.0}, {4294967296.0, 0, 4294967296.0}})
  {
    auto zero = ConvertDimensions(values, "Tuple", ZeroDimensionPolicy::Allow);
    SIMPLNX_RESULT_REQUIRE_VALID(zero);
    auto count = ComputeDimensionProduct(zero.value(), "Tuple");
    SIMPLNX_RESULT_REQUIRE_VALID(count);
    CHECK(count.value() == 0);
  }
  for(const auto& values : std::vector<std::vector<float64>>{{0, -1}, {4294967296.0, 4294967296.0, -1}})
  {
    auto invalid = ConvertDimensions(values, "Tuple", ZeroDimensionPolicy::Allow);
    SIMPLNX_RESULT_REQUIRE_INVALID(invalid);
    CHECK(invalid.errors()[0].code == DimensionConversion::k_InvalidValue);
    CHECK(invalid.errors()[0].message.find("below the minimum") != std::string::npos);
  }
}

TEST_CASE("simplnx::DimensionConversion: unsigned string grammar", "[simplnx][DimensionConversion]")
{
  for(const std::string text : {"3,4", " 3 , 4 ", "\t3\r,\n4\f\v"})
  {
    auto result = ConvertDimensions(text, "Tuple", ZeroDimensionPolicy::Reject);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    CHECK(result.value() == ShapeType{3, 4});
  }
  for(const auto& [policy, acceptedRange] :
      std::vector<std::pair<ZeroDimensionPolicy, std::string>>{{ZeroDimensionPolicy::Allow, "unsigned decimal integers in [0, "}, {ZeroDimensionPolicy::Reject, "unsigned decimal integers in [1, "}})
  {
    for(const std::string text : {"3abc", "-1", "+1", "", ",3", "3,", "3,,4", "1e3", "18446744073709551616"})
    {
      CAPTURE(policy, text);
      auto result = ConvertDimensions(text, "Tuple", policy);
      SIMPLNX_RESULT_REQUIRE_INVALID(result);
      CHECK(result.errors()[0].code == DimensionConversion::k_InvalidToken);
      CHECK(result.errors()[0].message.find("token '") != std::string::npos);
      CHECK(result.errors()[0].message.find(acceptedRange + std::to_string(std::numeric_limits<usize>::max()) + "]") != std::string::npos);
    }
  }
  auto maximum = ConvertDimensions(std::to_string(std::numeric_limits<usize>::max()), "Tuple", ZeroDimensionPolicy::Reject);
  SIMPLNX_RESULT_REQUIRE_VALID(maximum);
  CHECK(maximum.value() == ShapeType{std::numeric_limits<usize>::max()});
  auto zero = ConvertDimensions(std::string("0"), "Tuple", ZeroDimensionPolicy::Allow);
  SIMPLNX_RESULT_REQUIRE_VALID(zero);
  CHECK(zero.value() == ShapeType{0});
  auto rejected = ConvertDimensions(std::string("0"), "Component", ZeroDimensionPolicy::Reject);
  SIMPLNX_RESULT_REQUIRE_INVALID(rejected);
  CHECK(rejected.errors()[0].code == DimensionConversion::k_ZeroDimension);
  CHECK(rejected.errors()[0].message.find("cannot be 0") != std::string::npos);
  auto overflow = ConvertDimensions(std::string("4294967296,4294967296"), "Tuple", ZeroDimensionPolicy::Allow);
  SIMPLNX_RESULT_REQUIRE_INVALID(overflow);
  CHECK(overflow.errors()[0].code == DimensionConversion::k_ProductOverflow);
  CHECK(overflow.errors()[0].message.find("dimensions [Y=4294967296, X=4294967296] have a product") != std::string::npos);
  for(const std::string text : {"4294967296,4294967296,0", "0,4294967296,4294967296", "4294967296,0,4294967296"})
  {
    auto result = ConvertDimensions(text, "Tuple", ZeroDimensionPolicy::Allow);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    auto product = ComputeDimensionProduct(result.value(), "Tuple");
    SIMPLNX_RESULT_REQUIRE_VALID(product);
    CHECK(product.value() == 0);
  }
  auto scalarFirst = ConvertDimensions(std::string("4294967296,4294967296,-1"), "Tuple", ZeroDimensionPolicy::Allow);
  SIMPLNX_RESULT_REQUIRE_INVALID(scalarFirst);
  CHECK(scalarFirst.errors()[0].code == DimensionConversion::k_InvalidToken);
}

TEST_CASE("simplnx::CreateStringArrayAction: checked counts in both modes", "[simplnx][CreateStringArrayAction]")
{
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  for(const auto& [shape, code, expectedShape] :
      std::vector<std::tuple<ShapeType, int32, std::string>>{{{usize{1} << 32, usize{1} << 32}, DimensionConversion::k_StringTupleOverflow, "dimensions [Y=4294967296, X=4294967296]"},
                                                             {{usize{1} << 63}, DimensionConversion::k_StringCapacity, "dimensions [X=9223372036854775808]"},
                                                             {{}, -6002, ""}})
  {
    CAPTURE(shape, code, mode);
    DataStructure ds;
    const DataPath path({"Strings"});
    CreateStringArrayAction action(shape, path);
    auto result = action.apply(ds, mode);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    CHECK(result.errors()[0].code == code);
    if(code != -6002)
    {
      CHECK(result.errors()[0].message.find("Strings") != std::string::npos);
      CHECK(result.errors()[0].message.find(expectedShape) != std::string::npos);
    }
    CHECK(ds.getData(path) == nullptr);
  }
  for(const ShapeType shape : {ShapeType{0}, ShapeType{2}})
  {
    DataStructure ds;
    CreateStringArrayAction action(shape, DataPath({"Strings"}), "value");
    auto result = action.apply(ds, mode);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE_NOTHROW(ds.getDataRefAs<StringArray>(DataPath({"Strings"})));
    const auto& array = ds.getDataRefAs<StringArray>(DataPath({"Strings"}));
    CHECK(array.getTupleShape() == shape);
    CHECK(array.getNumberOfTuples() == shape[0]);
    if(shape[0] != 0)
    {
      CHECK(array[0] == "value");
    }
    UnitTest::CheckArraysInheritTupleDims(ds);
  }
}

TEST_CASE("simplnx::DimensionConversion: legacy predicates and scalar precedence", "[simplnx][DimensionConversion]")
{
  const float64 nan = std::numeric_limits<float64>::quiet_NaN();
  const float64 negativeInfinity = -std::numeric_limits<float64>::infinity();
  const float64 positiveInfinity = std::numeric_limits<float64>::infinity();
  const float64 upperExclusive = std::ldexp(1.0, 64);
  // These expected codes come from the approved legacy mapping, not the implementation predicate.
  const std::vector<std::tuple<LegacyDimensionCheck, float64, int32, int32>> cases = {{LegacyDimensionCheck::Zero, 0.0, -78603, -78603},
                                                                                      {LegacyDimensionCheck::Zero, -1.0, -78603, -79000},
                                                                                      {LegacyDimensionCheck::Zero, 1.5, -78603, -79000},
                                                                                      {LegacyDimensionCheck::Zero, nan, -78603, -79000},
                                                                                      {LegacyDimensionCheck::Zero, negativeInfinity, -78603, -79000},
                                                                                      {LegacyDimensionCheck::Zero, positiveInfinity, -78603, -79000},
                                                                                      {LegacyDimensionCheck::Zero, upperExclusive, -78603, -79000},
                                                                                      {LegacyDimensionCheck::NonPositive, 0.0, -65403, -65403},
                                                                                      {LegacyDimensionCheck::NonPositive, -1.0, -65403, -65403},
                                                                                      {LegacyDimensionCheck::NonPositive, 1.5, -65403, -79000},
                                                                                      {LegacyDimensionCheck::NonPositive, nan, -65403, -79000},
                                                                                      {LegacyDimensionCheck::NonPositive, negativeInfinity, -65403, -65403},
                                                                                      {LegacyDimensionCheck::NonPositive, positiveInfinity, -65403, -79000},
                                                                                      {LegacyDimensionCheck::NonPositive, upperExclusive, -65403, -79000},
                                                                                      {LegacyDimensionCheck::PositiveWhole, 0.0, -78605, -78605},
                                                                                      {LegacyDimensionCheck::PositiveWhole, -1.0, -78605, -78605},
                                                                                      {LegacyDimensionCheck::PositiveWhole, 1.5, -78605, -78605},
                                                                                      {LegacyDimensionCheck::PositiveWhole, nan, -78605, -78605},
                                                                                      {LegacyDimensionCheck::PositiveWhole, negativeInfinity, -78605, -78605},
                                                                                      {LegacyDimensionCheck::PositiveWhole, positiveInfinity, -78605, -79000},
                                                                                      {LegacyDimensionCheck::PositiveWhole, upperExclusive, -78605, -79000}};
  for(const auto& [check, value, legacyCode, expectedCode] : cases)
  {
    CAPTURE(check, value, legacyCode, expectedCode);
    const std::vector<float64> values = {value};
    auto result = ConvertDimensions(values, "Tuple", ZeroDimensionPolicy::Reject, check, legacyCode);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    CHECK(result.errors()[0].code == expectedCode);
    CHECK(result.errors()[0].message.find(fmt::format("value {}", value)) != std::string::npos);
  }
  const std::vector<float64> values = {1.5, 0};
  auto first = ConvertDimensions(values, "Tuple", ZeroDimensionPolicy::Reject, LegacyDimensionCheck::Zero, -78603);
  SIMPLNX_RESULT_REQUIRE_INVALID(first);
  CHECK(first.errors()[0].code == DimensionConversion::k_InvalidValue);
  CHECK(first.errors()[0].message.find("index 0") != std::string::npos);
}
