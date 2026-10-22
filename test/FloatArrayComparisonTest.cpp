#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace nx::core;

namespace
{
constexpr int32 k_InjectedReadError = -98781;
constexpr int32 k_InjectedReadWarning = 98782;
const DataPath k_ExemplarPath({"Exemplar"});
const DataPath k_ComputedPath({"Computed"});

/**
 * @class RecordingFloatStore
 * @brief Records real resident reads and can fail one selected call.
 * @tparam T Floating-point scalar type.
 */
template <typename T>
class RecordingFloatStore : public DataStore<T>
{
public:
  using DataStore<T>::DataStore;

  ~RecordingFloatStore() override = default;
  RecordingFloatStore(const RecordingFloatStore&) = default;
  RecordingFloatStore(RecordingFloatStore&&) = default;
  RecordingFloatStore& operator=(const RecordingFloatStore&) = delete;
  RecordingFloatStore& operator=(RecordingFloatStore&&) = default;

  mutable std::vector<std::pair<usize, usize>> reads;
  usize failingReadCall = 0;
  bool warnOnRead = false;

  /**
   * @brief Records each range before reading values or returning the selected error.
   * @param startIndex First flat value index.
   * @param buffer Receives real resident values on success.
   * @return Known injected error or the resident read result, with an optional known warning.
   */
  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    reads.emplace_back(startIndex, buffer.size());
    auto result = reads.size() == failingReadCall ? MakeErrorResult(k_InjectedReadError, "Injected floating-array read error") : DataStore<T>::copyIntoBuffer(startIndex, buffer);
    if(warnOnRead)
    {
      result.warnings().push_back({k_InjectedReadWarning, "Injected floating-array read warning"});
    }
    return result;
  }
};

/**
 * @brief Checks every stored value without calling the comparison helper or its bulk read path.
 * @tparam T Floating-point scalar type.
 * @param store Resident values to verify.
 * @param expected Independent snapshot supplied by the fixture.
 */
template <typename T>
void CheckPreservedValues(const RecordingFloatStore<T>& store, const std::vector<T>& expected)
{
  REQUIRE(store.getSize() == expected.size());
  for(usize valueIndex = 0; valueIndex < expected.size(); ++valueIndex)
  {
    CAPTURE(valueIndex);
    const T actualValue = store.getValue(valueIndex);
    if(std::isnan(expected[valueIndex]))
    {
      CHECK(std::isnan(actualValue));
    }
    else
    {
      CHECK(actualValue == expected[valueIndex]);
      CHECK(std::signbit(actualValue) == std::signbit(expected[valueIndex]));
    }
  }
}

/**
 * @struct FloatComparisonFixture
 * @brief Owns two real arrays and independent copies of all their original values.
 * @tparam T Floating-point scalar type.
 */
template <typename T>
struct FloatComparisonFixture
{
  DataStructure dataStructure;
  std::vector<T> exemplaryValues;
  std::vector<T> computedValues;
  std::shared_ptr<RecordingFloatStore<T>> exemplaryStore;
  std::shared_ptr<RecordingFloatStore<T>> computedStore;
  DataArray<T>* exemplaryArray = nullptr;
  DataArray<T>* computedArray = nullptr;

  /**
   * @brief Creates and initializes both resident arrays before a test can enable read failures.
   * @param exemplary Exemplar snapshot.
   * @param computed Computed snapshot.
   * @param exemplaryTuples Exemplar tuple shape; an empty shape selects one tuple per value.
   * @param exemplaryComponents Exemplar component shape.
   * @param computedTuples Computed tuple shape; an empty shape selects one tuple per value.
   * @param computedComponents Computed component shape.
   */
  FloatComparisonFixture(std::vector<T> exemplary, std::vector<T> computed, ShapeType exemplaryTuples = {}, ShapeType exemplaryComponents = {1}, ShapeType computedTuples = {},
                         ShapeType computedComponents = {1})
  : exemplaryValues(std::move(exemplary))
  , computedValues(std::move(computed))
  {
    if(exemplaryTuples.empty())
    {
      exemplaryTuples = {exemplaryValues.size()};
    }
    if(computedTuples.empty())
    {
      computedTuples = {computedValues.size()};
    }
    exemplaryStore = std::make_shared<RecordingFloatStore<T>>(exemplaryTuples, exemplaryComponents, T{});
    computedStore = std::make_shared<RecordingFloatStore<T>>(computedTuples, computedComponents, T{});
    REQUIRE(exemplaryStore->getSize() == exemplaryValues.size());
    REQUIRE(computedStore->getSize() == computedValues.size());
    exemplaryArray = DataArray<T>::Create(dataStructure, k_ExemplarPath.getTargetName(), exemplaryStore);
    computedArray = DataArray<T>::Create(dataStructure, k_ComputedPath.getTargetName(), computedStore);
    REQUIRE(exemplaryArray != nullptr);
    REQUIRE(computedArray != nullptr);
    auto exemplaryWriteResult = exemplaryStore->copyFromBuffer(0, nonstd::span<const T>(exemplaryValues.data(), exemplaryValues.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(exemplaryWriteResult);
    auto computedWriteResult = computedStore->copyFromBuffer(0, nonstd::span<const T>(computedValues.data(), computedValues.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(computedWriteResult);
    requireUnchanged();
    REQUIRE(exemplaryStore->reads.empty());
    REQUIRE(computedStore->reads.empty());
  }

  ~FloatComparisonFixture() = default;
  FloatComparisonFixture(const FloatComparisonFixture&) = default;
  FloatComparisonFixture(FloatComparisonFixture&&) = default;
  FloatComparisonFixture& operator=(const FloatComparisonFixture&) = default;
  FloatComparisonFixture& operator=(FloatComparisonFixture&&) = default;

  /**
   * @brief Evaluates the actual arrays and verifies that every source value stays unchanged.
   * @param epsilon Exclusive absolute-difference limit.
   * @param checkNans Selects the NaN-position policy.
   * @return Actual evaluator result, including any setup or read error.
   */
  Result<UnitTest::FloatArrayComparisonDetail::FloatArrayComparison<T>> evaluate(T epsilon, bool checkNans)
  {
    auto result = UnitTest::FloatArrayComparisonDetail::EvaluateFloatArraysWithNans(*exemplaryArray, *computedArray, epsilon, checkNans);
    requireUnchanged();
    return result;
  }

  /**
   * @brief Verifies full source values and the fixture's array hierarchy.
   */
  void requireUnchanged() const
  {
    CheckPreservedValues(*exemplaryStore, exemplaryValues);
    CheckPreservedValues(*computedStore, computedValues);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
};
} // namespace

TEMPLATE_TEST_CASE("UnitTest::CompareFloatArraysWithNans: equal values and public wrapper", "[UnitTest][FloatArrayComparison]", float32, float64)
{
  const TestType nanValue = std::numeric_limits<TestType>::quiet_NaN();
  FloatComparisonFixture<TestType> fixture({0, TestType{-0.0}, 2, nanValue}, {0, TestType{-0.0}, 2, nanValue});
  auto result = fixture.evaluate(TestType{0.5}, true);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  CHECK(result.value().matches);
  CHECK(result.value().maximumDifference == TestType{0});
  CHECK(result.value().maximumDifferenceIndex == 0);
  CHECK_FALSE(result.value().firstNanMismatchIndex.has_value());
  UnitTest::CompareFloatArraysWithNans<TestType>(fixture.dataStructure, k_ExemplarPath, k_ComputedPath);
  fixture.requireUnchanged();
}

TEMPLATE_TEST_CASE("UnitTest::CompareFloatArraysWithNans: NaN positions and full scan", "[UnitTest][FloatArrayComparison]", float32, float64)
{
  const bool reverseInputs = GENERATE(false, true);
  const bool differentNanPositions = GENERATE(false, true);
  const bool checkNans = GENERATE(false, true);
  const bool finiteMismatch = GENERATE(false, true);
  const TestType nanValue = std::numeric_limits<TestType>::quiet_NaN();
  std::vector<TestType> exemplary = {1, nanValue, 3, 4, 5};
  std::vector<TestType> computed = {1, 2, differentNanPositions ? nanValue : TestType{3}, finiteMismatch ? TestType{6} : TestType{4}, finiteMismatch ? TestType{8} : TestType{5}};
  if(reverseInputs)
  {
    std::swap(exemplary, computed);
  }
  CAPTURE(reverseInputs, differentNanPositions, checkNans, finiteMismatch);
  FloatComparisonFixture<TestType> fixture(exemplary, computed);
  auto result = fixture.evaluate(TestType{0.5}, checkNans);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  CHECK(result.value().matches == (!checkNans && !finiteMismatch));
  if(checkNans)
  {
    REQUIRE(result.value().firstNanMismatchIndex == std::optional<usize>{1});
    CHECK(std::isnan(result.value().exemplaryNanMismatchValue) == !reverseInputs);
    CHECK(std::isnan(result.value().generatedNanMismatchValue) == reverseInputs);
    CHECK((reverseInputs ? result.value().exemplaryNanMismatchValue : result.value().generatedNanMismatchValue) == TestType{2});
  }
  else
  {
    CHECK_FALSE(result.value().firstNanMismatchIndex.has_value());
  }
  CHECK(result.value().maximumDifference == (finiteMismatch ? TestType{3} : TestType{0}));
  CHECK(result.value().maximumDifferenceIndex == (finiteMismatch ? 4 : 0));
  if(!checkNans && !finiteMismatch)
  {
    UnitTest::CompareFloatArraysWithNans<TestType>(fixture.dataStructure, k_ExemplarPath, k_ComputedPath, TestType{0.5}, false);
    fixture.requireUnchanged();
  }
}

TEMPLATE_TEST_CASE("UnitTest::CompareFloatArraysWithNans: exclusive tolerance and maximum", "[UnitTest][FloatArrayComparison]", float32, float64)
{
  const TestType difference = GENERATE(TestType{0.25}, TestType{0.5}, TestType{0.75});
  FloatComparisonFixture<TestType> fixture({0, 0, 0, 0}, {TestType{0.125}, difference, TestType{0.125}, difference});
  auto result = fixture.evaluate(TestType{0.5}, true);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  CHECK(result.value().matches == (difference < TestType{0.5}));
  CHECK(result.value().maximumDifference == difference);
  CHECK(result.value().maximumDifferenceIndex == 1);
  CHECK_FALSE(result.value().firstNanMismatchIndex.has_value());
}

TEST_CASE("UnitTest::CompareFloatArraysWithNans: double precision", "[UnitTest][FloatArrayComparison]")
{
  const float64 difference = 1.0 + std::ldexp(1.0, -40);
  const float64 epsilon = 1.0 + std::ldexp(1.0, -41);
  FloatComparisonFixture<float64> fixture({0.0}, {difference});
  auto result = fixture.evaluate(epsilon, true);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  CHECK_FALSE(result.value().matches);
  CHECK(result.value().maximumDifference == difference);
  CHECK(result.value().maximumDifferenceIndex == 0);
}

TEMPLATE_TEST_CASE("UnitTest::CompareFloatArraysWithNans: infinity pairs", "[UnitTest][FloatArrayComparison]", float32, float64)
{
  const int pairIndex = GENERATE(0, 1, 2, 3, 4, 5);
  const TestType infinity = std::numeric_limits<TestType>::infinity();
  const std::vector<std::pair<TestType, TestType>> pairs = {{infinity, infinity},  {-infinity, -infinity},  {infinity, -infinity},
                                                            {-infinity, infinity}, {TestType{2}, infinity}, {infinity, TestType{2}}};
  CAPTURE(pairIndex);
  FloatComparisonFixture<TestType> fixture({pairs[pairIndex].first}, {pairs[pairIndex].second});
  auto result = fixture.evaluate(TestType{0.5}, true);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  CHECK(result.value().matches == (pairIndex < 2));
  CHECK(result.value().maximumDifference == (pairIndex < 2 ? TestType{0} : infinity));
  CHECK_FALSE(result.value().firstNanMismatchIndex.has_value());
  if(pairIndex < 2)
  {
    UnitTest::CompareFloatArraysWithNans<TestType>(fixture.dataStructure, k_ExemplarPath, k_ComputedPath);
    fixture.requireUnchanged();
  }
}

TEMPLATE_TEST_CASE("UnitTest::CompareFloatArraysWithNans: bounded tail and read failures", "[UnitTest][FloatArrayComparison]", float32, float64)
{
  const bool reverseInputs = GENERATE(false, true);
  // The fault variants reuse this one large fixture to check a short final read on either input.
  const int failingSide = GENERATE(0, 1, 2);
  std::vector<TestType> exemplary(40001, TestType{7});
  std::vector<TestType> computed = exemplary;
  exemplary[40000] = std::numeric_limits<TestType>::quiet_NaN();
  if(reverseInputs)
  {
    std::swap(exemplary, computed);
  }
  CAPTURE(reverseInputs, failingSide);
  FloatComparisonFixture<TestType> fixture(exemplary, computed);
  fixture.exemplaryStore->failingReadCall = failingSide == 1 ? 2 : 0;
  fixture.computedStore->failingReadCall = failingSide == 2 ? 2 : 0;
  fixture.exemplaryStore->warnOnRead = true;
  fixture.computedStore->warnOnRead = true;
  auto result = fixture.evaluate(TestType{0.5}, true);
  const std::vector<std::pair<usize, usize>> fullReads = {{0, 40000}, {40000, 1}};
  const std::vector<std::pair<usize, usize>> firstRead = {{0, 40000}};
  CHECK(fixture.exemplaryStore->reads == fullReads);
  CHECK(fixture.computedStore->reads == (failingSide == 1 ? firstRead : fullReads));
  REQUIRE(result.warnings().size() == (failingSide == 1 ? 3 : 4));
  for(const auto& warning : result.warnings())
  {
    CHECK(warning.code == k_InjectedReadWarning);
    CHECK(warning.message == "Injected floating-array read warning");
  }
  if(failingSide == 0)
  {
    REQUIRE(result.valid());
    CHECK_FALSE(result.value().matches);
    CHECK(result.value().firstNanMismatchIndex == std::optional<usize>{40000});
    CHECK(std::isnan(result.value().exemplaryNanMismatchValue) == !reverseInputs);
    CHECK(std::isnan(result.value().generatedNanMismatchValue) == reverseInputs);
    CHECK(result.value().maximumDifference == TestType{0});
  }
  else
  {
    REQUIRE(result.invalid());
    REQUIRE(result.errors().size() == 1);
    CHECK(result.errors()[0].code == k_InjectedReadError);
    CHECK(result.errors()[0].message == "Injected floating-array read error");
  }
}

TEMPLATE_TEST_CASE("UnitTest::CompareFloatArraysWithNans: shape counts before reads", "[UnitTest][FloatArrayComparison]", float32, float64)
{
  SECTION("Unequal tuple counts in either direction")
  {
    const bool reverseInputs = GENERATE(false, true);
    const usize exemplaryCount = reverseInputs ? 3 : 2;
    const usize computedCount = reverseInputs ? 2 : 3;
    FloatComparisonFixture<TestType> fixture(std::vector<TestType>(exemplaryCount, 1), std::vector<TestType>(computedCount, 1));
    auto result = fixture.evaluate(TestType{0.5}, true);
    REQUIRE(result.invalid());
    REQUIRE(result.errors().size() == 1);
    CHECK(result.errors()[0].code == -90051);
    CHECK(result.errors()[0].message == fmt::format("Floating-array comparison requires equal tuple counts: exemplar {}, computed {}.", exemplaryCount, computedCount));
    CHECK(fixture.exemplaryStore->reads.empty());
    CHECK(fixture.computedStore->reads.empty());
  }
  SECTION("Fewer or more computed components")
  {
    const usize computedComponents = GENERATE(usize{1}, usize{3});
    FloatComparisonFixture<TestType> fixture(std::vector<TestType>(4, 1), std::vector<TestType>(2 * computedComponents, 1), {2}, {2}, {2}, {computedComponents});
    auto result = fixture.evaluate(TestType{0.5}, true);
    REQUIRE(result.invalid());
    REQUIRE(result.errors().size() == 1);
    CHECK(result.errors()[0].code == -90052);
    CHECK(result.errors()[0].message == fmt::format("Floating-array comparison requires equal component counts: exemplar 2, computed {}.", computedComponents));
    CHECK(fixture.exemplaryStore->reads.empty());
    CHECK(fixture.computedStore->reads.empty());
  }
  SECTION("Different shape vectors with equal tuple and component counts")
  {
    const std::vector<TestType> values = {1, 2, 3, 4, 5, 6, 7, 8};
    FloatComparisonFixture<TestType> fixture(values, values, {2, 2}, {2}, {4}, {1, 2});
    auto result = fixture.evaluate(TestType{0.5}, true);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    CHECK(result.value().matches);
    const std::vector<std::pair<usize, usize>> reads = {{0, 8}};
    CHECK(fixture.exemplaryStore->reads == reads);
    CHECK(fixture.computedStore->reads == reads);
    UnitTest::CompareFloatArraysWithNans<TestType>(fixture.dataStructure, k_ExemplarPath, k_ComputedPath);
    fixture.requireUnchanged();
  }
}

TEMPLATE_TEST_CASE("UnitTest::CompareFloatArraysWithNans: empty arrays and unusual tolerances", "[UnitTest][FloatArrayComparison]", float32, float64)
{
  const int epsilonIndex = GENERATE(0, 1, 2, 3, 4);
  const bool emptyArrays = GENERATE(false, true);
  const std::vector<TestType> epsilons = {TestType{0.5}, TestType{0}, TestType{-0.5}, std::numeric_limits<TestType>::quiet_NaN(), std::numeric_limits<TestType>::infinity()};
  const std::vector<TestType> values = emptyArrays ? std::vector<TestType>{} : std::vector<TestType>{1};
  CAPTURE(epsilonIndex, emptyArrays);
  FloatComparisonFixture<TestType> fixture(values, values);
  auto result = fixture.evaluate(epsilons[epsilonIndex], true);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  CHECK(result.value().matches == (epsilonIndex == 0 || epsilonIndex == 4));
  CHECK(result.value().maximumDifference == TestType{0});
  CHECK(result.value().maximumDifferenceIndex == 0);
  if(emptyArrays)
  {
    CHECK(fixture.exemplaryStore->reads.empty());
    CHECK(fixture.computedStore->reads.empty());
  }
}
