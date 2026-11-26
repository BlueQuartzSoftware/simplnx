#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <limits>

using namespace nx::core;

namespace
{
template <class T>
NeighborList<T>* MakeList(DataStructure& ds, const std::string& name, const std::vector<std::vector<T>>& rows)
{
  auto* list = NeighborList<T>::Create(ds, name, {rows.size()});
  REQUIRE(list != nullptr);
  for(usize i = 0; i < rows.size(); ++i)
  {
    list->setList(static_cast<int32>(i), rows[i]);
  }
  return list;
}
} // namespace

TEST_CASE("NeighborList comparison checks lengths even for empty rows", "[simplnx][NeighborListComparison]")
{
  DataStructure ds;
  const auto* expected = MakeList<int32>(ds, "Expected", {{}, {3}});
  const auto* actual = MakeList<int32>(ds, "Actual", {{}, {}});
  const bool reverse = GENERATE(false, true);
  const auto result = UnitTest::EvaluateNeighborLists(reverse ? actual : expected, reverse ? expected : actual, DataType::int32);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  CHECK(result.errors().front().code == -90064);
  CHECK(result.errors().front().message.find("tuple 1") != std::string::npos);
}

TEST_CASE("NeighborList comparison checks all values and preserves integer precision", "[simplnx][NeighborListComparison]")
{
  DataStructure ds;
  SECTION("A small floating-point difference precedes a large one")
  {
    const auto* expected = MakeList<float64>(ds, "Expected", {{1, 8}});
    const auto* actual = MakeList<float64>(ds, "Actual", {{1.00001, 99}});
    const auto result = UnitTest::EvaluateNeighborLists(expected, actual, DataType::float64);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    CHECK(result.errors().front().code == -90065);
    CHECK(result.errors().front().message.find("slot 1") != std::string::npos);
  }
  SECTION("Adjacent 64-bit integers above the exact double range")
  {
    constexpr int64 value = 9007199254740992LL;
    const auto* expected = MakeList<int64>(ds, "Expected", {{value}});
    const auto* actual = MakeList<int64>(ds, "Actual", {{value + 1}});
    const auto result = UnitTest::EvaluateNeighborLists(expected, actual, DataType::int64);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    CHECK(result.errors().front().code == -90065);
  }
}

TEST_CASE("NeighborList comparison preserves ordering contracts and source values", "[simplnx][NeighborListComparison]")
{
  DataStructure ds;
  const auto* expected = MakeList<int32>(ds, "Expected", {{2, 1, 1}});
  const auto* actual = MakeList<int32>(ds, "Actual", {{1, 2, 1}});
  UnitTest::CompareNeighborLists<int32>(ds, DataPath({"Expected"}), DataPath({"Actual"}));
  const auto ordered = UnitTest::EvaluateNeighborLists(expected, actual, DataType::int32, UnitTest::NeighborListOrder::Ordered);
  SIMPLNX_RESULT_REQUIRE_INVALID(ordered);
  CHECK(ordered.errors().front().code == -90065);
  CHECK(expected->at(usize{0}) == std::vector<int32>{2, 1, 1});
  CHECK(actual->at(usize{0}) == std::vector<int32>{1, 2, 1});
}

TEST_CASE("NeighborList comparison handles NaNs and the exclusive tolerance", "[simplnx][NeighborListComparison]")
{
  using namespace nx::core::UnitTest;
  DataStructure ds;
  constexpr float64 nan = std::numeric_limits<float64>::quiet_NaN();
  constexpr float64 inf = std::numeric_limits<float64>::infinity();
  const auto* expected = MakeList<float64>(ds, "Expected", {{nan, -inf, 2}});
  auto* actual = MakeList<float64>(ds, "Actual", {{2.125, nan, -inf}});
  const auto unordered = NeighborListOrder::Unordered;
  CompareNeighborListFloatArraysWithNans(ds, DataPath({"Expected"}), DataPath({"Actual"}), 0.25);
  const auto atLimit = EvaluateNeighborLists(expected, actual, DataType::float64, unordered, 0.125, NeighborListNaNs::Match);
  SIMPLNX_RESULT_REQUIRE_INVALID(atLimit);
  actual->setList(0, std::vector<float64>{2, inf, -inf});
  const auto differentNanCount = EvaluateNeighborLists(expected, actual, DataType::float64, unordered, 0.25, NeighborListNaNs::Match);
  SIMPLNX_RESULT_REQUIRE_INVALID(differentNanCount);
  CompareNeighborListFloatArraysWithNans(ds, DataPath({"Expected"}), DataPath({"Actual"}), 0.25, false);
  const auto rejectNans = EvaluateNeighborLists(expected, expected, DataType::float64);
  SIMPLNX_RESULT_REQUIRE_INVALID(rejectNans);
}

TEST_CASE("NeighborList comparison requires two supported lists and equal tuple counts", "[simplnx][NeighborListComparison]")
{
  DataStructure ds;
  const auto* one = MakeList<int32>(ds, "One", {{}});
  const auto* two = MakeList<int32>(ds, "Two", {{}, {}});
  const auto wrongCount = UnitTest::EvaluateNeighborLists(one, two, DataType::int32);
  SIMPLNX_RESULT_REQUIRE_INVALID(wrongCount);
  CHECK(wrongCount.errors().front().code == -90063);
  const auto missing = UnitTest::EvaluateNeighborLists(one, nullptr, DataType::int32);
  SIMPLNX_RESULT_REQUIRE_INVALID(missing);
  CHECK(missing.errors().front().code == -90061);
  const auto wrongType = UnitTest::EvaluateNeighborLists(one, one, DataType::float32);
  SIMPLNX_RESULT_REQUIRE_INVALID(wrongType);
  CHECK(wrongType.errors().front().code == -90062);
}
