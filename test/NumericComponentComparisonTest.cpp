#include "ArrayComparisonRecordingStore.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <cmath>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

using namespace nx::core;

namespace
{
using UnitTest::ArrayComparison::RecordingStore;
using ReadRanges = std::vector<std::pair<usize, usize>>;

template <typename T>
struct ComponentArrays
{
  DataStructure data;
  std::shared_ptr<RecordingStore<T>> expectedStore;
  std::shared_ptr<RecordingStore<T>> actualStore;
  DataArray<T>* expected;
  DataArray<T>* actual;

  ComponentArrays(usize tuples, usize components)
  : expectedStore(std::make_shared<RecordingStore<T>>(ShapeType{tuples}, ShapeType{components}, T{}))
  , actualStore(std::make_shared<RecordingStore<T>>(ShapeType{tuples}, ShapeType{components}, T{}))
  , expected(DataArray<T>::Create(data, "Expected", expectedStore))
  , actual(DataArray<T>::Create(data, "Actual", actualStore))
  {
    REQUIRE(expected != nullptr);
    REQUIRE(actual != nullptr);
  }

  Result<> compare(usize startTuple, usize component) const
  {
    return UnitTest::EvaluateNumericArrays(*expected, *actual, GetDataType<T>(), startTuple, component);
  }
};
} // namespace

TEST_CASE("Numeric component comparison bounds reads when a tuple exceeds 40000 elements", "[simplnx][NumericComponentComparison]")
{
  ComponentArrays<uint64> arrays(4, 40001);
  constexpr uint64 maxValue = std::numeric_limits<uint64>::max();
  // The first tuple and every other component are outside the requested selection.
  arrays.actualStore->fill(1);
  for(const usize index : {80001, 120002, 160003})
  {
    arrays.expectedStore->setValue(index, maxValue);
    arrays.actualStore->setValue(index, maxValue);
  }
  SECTION("Only the requested component is compared")
  {
    const auto result = arrays.compare(1, 40000);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
  }
  SECTION("Adjacent unsigned values in the last selected tuple differ")
  {
    arrays.actualStore->setValue(160003, maxValue - 1);
    const auto result = arrays.compare(1, 40000);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -90074);
  }
  const ReadRanges expectedReads = {{80001, 1}, {120002, 1}, {160003, 1}};
  CHECK(arrays.expectedStore->reads == expectedReads);
  CHECK(arrays.actualStore->reads == expectedReads);
}

TEST_CASE("Numeric component comparison resumes at the selected component across a block boundary", "[simplnx][NumericComponentComparison]")
{
  ComponentArrays<int64> arrays(13337, 3);
  arrays.actualStore->setValue(2, 1);     // The selected component in skipped tuple 0.
  arrays.actualStore->setValue(6, 1);     // An unselected component inside the first read.
  arrays.actualStore->setValue(40005, 1); // An unselected component between the two reads.
  constexpr usize firstSelectedIndexInSecondBlock = 40007;
  arrays.expectedStore->setValue(firstSelectedIndexInSecondBlock, std::numeric_limits<int64>::min());
  arrays.actualStore->setValue(firstSelectedIndexInSecondBlock, std::numeric_limits<int64>::min());
  ReadRanges expectedReads = {{5, 40000}, {40007, 4}};
  SECTION("Equal signed values")
  {
    UnitTest::CompareDataArraysByComponent<int64>(*arrays.expected, *arrays.actual, 1, 2);
  }
  SECTION("Last selected element in the first block differs")
  {
    arrays.actualStore->setValue(40004, 1);
    const auto result = arrays.compare(1, 2);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -90074);
    expectedReads = {{5, 40000}};
  }
  SECTION("Opposite signed limits")
  {
    arrays.actualStore->setValue(firstSelectedIndexInSecondBlock, std::numeric_limits<int64>::max());
    const auto result = arrays.compare(1, 2);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -90074);
  }
  CHECK(arrays.expectedStore->reads == expectedReads);
  CHECK(arrays.actualStore->reads == expectedReads);
}

TEST_CASE("Numeric component comparison validates the selection before reading", "[simplnx][NumericComponentComparison]")
{
  ComponentArrays<int32> arrays(2, 3);
  SECTION("Start tuple equals the tuple count")
  {
    const auto result = arrays.compare(2, 0);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -90073);
  }
  SECTION("Component equals the component count")
  {
    const auto result = arrays.compare(0, 3);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -90073);
  }
  SECTION("Arrays with equal total sizes have incompatible tuple strides")
  {
    ComponentArrays<int32> other(3, 2);
    const auto result = UnitTest::EvaluateNumericArrays(*arrays.expected, *other.actual, DataType::int32, 0, 1);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -90072);
    CHECK(other.actualStore->reads.empty());
  }
  CHECK(arrays.expectedStore->reads.empty());
  CHECK(arrays.actualStore->reads.empty());
}

TEST_CASE("Numeric component comparison retains floating point precision and its NaN policy", "[simplnx][NumericComponentComparison]")
{
  ComponentArrays<float64> arrays(2, 3);
  constexpr float64 tolerance = static_cast<float64>(UnitTest::EPSILON);
  SECTION("An absolute difference at the inclusive tolerance matches")
  {
    arrays.actualStore->setValue(5, tolerance);
    const auto result = arrays.compare(1, 2);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
  }
  SECTION("The next representable double exceeds the tolerance")
  {
    arrays.actualStore->setValue(5, std::nextafter(tolerance, std::numeric_limits<float64>::infinity()));
    const auto result = arrays.compare(1, 2);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -90074);
  }
  SECTION("Paired NaNs in the selected component fail")
  {
    arrays.expectedStore->setValue(5, std::numeric_limits<float64>::quiet_NaN());
    arrays.actualStore->setValue(5, std::numeric_limits<float64>::quiet_NaN());
    const auto result = arrays.compare(1, 2);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -90074);
  }
}
