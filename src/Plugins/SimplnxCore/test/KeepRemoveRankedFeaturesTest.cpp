#include "SimplnxCore/Filters/KeepRemoveRankedFeaturesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <limits>
#include <set>
#include <tuple>
#include <vector>

using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
const DataPath k_ImageGeomPath({k_DataContainer});
const DataPath k_FeatureIdsPath({k_DataContainer, k_CellData, k_FeatureIds});
const std::string k_NumElements("NumElements");
const DataPath k_FeatureAmPath({k_DataContainer, k_CellFeatureData});
const DataPath k_RankingArrayPath({k_DataContainer, k_CellFeatureData, k_NumElements});

/**
 * @brief Builds a 5x2x1 Image Geometry holding 4 features of distinct, deliberately non-monotonic sizes.
 *
 * Cell layout (row major, y = 0 then y = 1):
 *   row 0:  1 1 1 2 3
 *   row 1:  1 4 4 4 3
 *
 * Feature sizes: id 1 -> 4 cells, id 2 -> 1 cell, id 3 -> 2 cells, id 4 -> 3 cells.
 * Ranked largest first: 1 (4), 4 (3), 3 (2), 2 (1). The sizes are not in id order, so a sort is
 * genuinely exercised rather than accidentally satisfied by the natural ordering.
 */
DataStructure BuildTestData()
{
  DataStructure dataStructure;

  auto* imageGeomPtr = ImageGeom::Create(dataStructure, k_DataContainer);
  const std::vector<usize> dims = {5, 2, 1};
  imageGeomPtr->setDimensions(dims);
  imageGeomPtr->setOrigin({0.0f, 0.0f, 0.0f});
  imageGeomPtr->setSpacing({1.0f, 1.0f, 1.0f});

  const std::vector<usize> cellTupleDims(dims.rbegin(), dims.rend());
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, k_CellData, cellTupleDims, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);

  auto* featureIdsPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, k_FeatureIds, cellTupleDims, {1}, cellAmPtr->getId());
  auto& featureIdsRef = featureIdsPtr->getDataStoreRef();
  const std::vector<int32> ids = {1, 1, 1, 2, 3, 1, 4, 4, 4, 3};
  for(usize i = 0; i < ids.size(); i++)
  {
    featureIdsRef[i] = ids[i];
  }

  // Feature Attribute Matrix: 5 tuples, where index 0 is the unused dummy feature.
  const std::vector<usize> featureTupleDims = {5};
  auto* featureAmPtr = AttributeMatrix::Create(dataStructure, k_CellFeatureData, featureTupleDims, imageGeomPtr->getId());

  auto* numElementsPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, k_NumElements, featureTupleDims, {1}, featureAmPtr->getId());
  auto& numElementsRef = numElementsPtr->getDataStoreRef();
  const std::vector<int32> sizes = {0, 4, 1, 2, 3};
  for(usize i = 0; i < sizes.size(); i++)
  {
    numElementsRef[i] = sizes[i];
  }

  return dataStructure;
}

Arguments MakeArgs(uint64 operation, uint64 rankFrom, uint64 numFeatures)
{
  Arguments args;
  args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_Operation_Key, std::make_any<ChoicesParameter::ValueType>(operation));
  args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_RankFrom_Key, std::make_any<ChoicesParameter::ValueType>(rankFrom));
  args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_Criterion_Key, std::make_any<ChoicesParameter::ValueType>(0ULL));
  args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_NumFeatures_Key, std::make_any<uint64>(numFeatures));
  args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_Percent_Key, std::make_any<float64>(10.0));
  args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_FillRemovedFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_RankingArrayPath_Key, std::make_any<DataPath>(k_RankingArrayPath));
  args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  return args;
}

/**
 * @brief Checks for one specific warning code.
 *
 * Asserting only that the warning list is non-empty is not enough. Unrelated machinery such as the
 * NeighborList removal check also contributes warnings, so a bare non-empty assertion passes even
 * when the warning under test was never emitted.
 */
bool HasWarningCode(const std::vector<Warning>& warnings, int32 code)
{
  return std::any_of(warnings.cbegin(), warnings.cend(), [code](const Warning& warning) { return warning.code == code; });
}

std::vector<int32> ReadFeatureIds(const DataStructure& dataStructure)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath));
  const auto& featureIdsRef = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath).getDataStoreRef();
  std::vector<int32> result(featureIdsRef.getNumberOfTuples());
  for(usize i = 0; i < result.size(); i++)
  {
    result[i] = featureIdsRef[i];
  }
  return result;
}

/**
 * @brief Builds a 6x2x1 Image Geometry holding 5 features, again with sizes out of id order.
 *
 * Cell layout (row major, y = 0 then y = 1):
 *   row 0:  1 1 1 1 1 2
 *   row 1:  3 3 4 4 4 5
 *
 * Feature sizes: id 1 -> 5, id 2 -> 1, id 3 -> 2, id 4 -> 3, id 5 -> 1.
 * Ranked largest first: 1 (5), 4 (3), 3 (2), then 2 and 5 tied at 1 with 2 winning on id order.
 *
 * With 5 features, "keep the 2 largest" and "remove the 3 smallest" select different counts, so the
 * duality invariant is genuinely exercised rather than satisfied by k == N - k.
 */
DataStructure BuildFiveFeatureData()
{
  DataStructure dataStructure;

  auto* imageGeomPtr = ImageGeom::Create(dataStructure, k_DataContainer);
  const std::vector<usize> dims = {6, 2, 1};
  imageGeomPtr->setDimensions(dims);
  imageGeomPtr->setOrigin({0.0f, 0.0f, 0.0f});
  imageGeomPtr->setSpacing({1.0f, 1.0f, 1.0f});

  const std::vector<usize> cellTupleDims(dims.rbegin(), dims.rend());
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, k_CellData, cellTupleDims, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);

  auto* featureIdsPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, k_FeatureIds, cellTupleDims, {1}, cellAmPtr->getId());
  auto& featureIdsRef = featureIdsPtr->getDataStoreRef();
  const std::vector<int32> ids = {1, 1, 1, 1, 1, 2, 3, 3, 4, 4, 4, 5};
  for(usize i = 0; i < ids.size(); i++)
  {
    featureIdsRef[i] = ids[i];
  }

  const std::vector<usize> featureTupleDims = {6};
  auto* featureAmPtr = AttributeMatrix::Create(dataStructure, k_CellFeatureData, featureTupleDims, imageGeomPtr->getId());

  auto* numElementsPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, k_NumElements, featureTupleDims, {1}, featureAmPtr->getId());
  auto& numElementsRef = numElementsPtr->getDataStoreRef();
  const std::vector<int32> sizes = {0, 5, 1, 2, 3, 1};
  for(usize i = 0; i < sizes.size(); i++)
  {
    numElementsRef[i] = sizes[i];
  }

  return dataStructure;
}

/**
 * @brief Returns the set of distinct positive FeatureIds present, which identifies the survivors
 * independently of how they were renumbered.
 */
std::set<int32> SurvivingIdSet(const DataStructure& dataStructure)
{
  std::set<int32> ids;
  for(int32 featureId : ReadFeatureIds(dataStructure))
  {
    if(featureId > 0)
    {
      ids.insert(featureId);
    }
  }
  return ids;
}

} // namespace

TEST_CASE("SimplnxCore::KeepRemoveRankedFeaturesFilter: Feature Count", "[SimplnxCore][KeepRemoveRankedFeatures]")
{
  UnitTest::LoadPlugins();

  // Original ids:            1 1 1 2 3 1 4 4 4 3
  // Sizes: 1->4, 2->1, 3->2, 4->3.  Ranked largest first: 1, 4, 3, 2.
  // Surviving features are always renumbered by ascending original id.
  const std::vector<std::tuple<std::string, uint64, uint64, uint64, std::vector<int32>>> cases = {
      // Keep the 2 largest: features 1 and 4 survive, renumbered 1 and 2.
      {"Keep 2 Largest", 0ULL, 0ULL, 2ULL, {1, 1, 1, 0, 0, 1, 2, 2, 2, 0}},
      // Keep the 2 smallest: features 2 and 3 survive, renumbered 1 and 2.
      {"Keep 2 Smallest", 0ULL, 1ULL, 2ULL, {0, 0, 0, 1, 2, 0, 0, 0, 0, 2}},
      // Remove the 2 largest leaves the same survivors as keeping the 2 smallest.
      {"Remove 2 Largest", 1ULL, 0ULL, 2ULL, {0, 0, 0, 1, 2, 0, 0, 0, 0, 2}},
      // Remove the 2 smallest leaves the same survivors as keeping the 2 largest.
      {"Remove 2 Smallest", 1ULL, 1ULL, 2ULL, {1, 1, 1, 0, 0, 1, 2, 2, 2, 0}},
  };

  for(const auto& [name, operation, rankFrom, numFeatures, expected] : cases)
  {
    DYNAMIC_SECTION(name)
    {
      DataStructure dataStructure = BuildTestData();
      KeepRemoveRankedFeaturesFilter filter;
      Arguments args = MakeArgs(operation, rankFrom, numFeatures);

      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

      auto executeResult = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

      REQUIRE(ReadFeatureIds(dataStructure) == expected);

      // Two features survived, so the compacted Feature Attribute Matrix holds 3 tuples: dummy + 2.
      REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_RankingArrayPath));
      REQUIRE(dataStructure.getDataRefAs<Int32Array>(k_RankingArrayPath).getNumberOfTuples() == 3);

      UnitTest::CheckArraysInheritTupleDims(dataStructure);
    }
  }
}

TEST_CASE("SimplnxCore::KeepRemoveRankedFeaturesFilter: Percent of Feature Count", "[SimplnxCore][KeepRemoveRankedFeatures]")
{
  UnitTest::LoadPlugins();
  KeepRemoveRankedFeaturesFilter filter;

  SECTION("50% of 4 Features keeps the 2 largest")
  {
    DataStructure dataStructure = BuildTestData();
    Arguments args = MakeArgs(0ULL, 0ULL, 2ULL);
    args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_Criterion_Key, std::make_any<ChoicesParameter::ValueType>(1ULL));
    args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_Percent_Key, std::make_any<float64>(50.0));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const std::vector<int32> expected = {1, 1, 1, 0, 0, 1, 2, 2, 2, 0};
    REQUIRE(ReadFeatureIds(dataStructure) == expected);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("30% of 4 Features rounds to 1")
  {
    // 0.30 * 4 = 1.2, which rounds to 1. Only the single largest Feature (id 1) survives.
    DataStructure dataStructure = BuildTestData();
    Arguments args = MakeArgs(0ULL, 0ULL, 2ULL);
    args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_Criterion_Key, std::make_any<ChoicesParameter::ValueType>(1ULL));
    args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_Percent_Key, std::make_any<float64>(30.0));

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const std::vector<int32> expected = {1, 1, 1, 0, 0, 1, 0, 0, 0, 0};
    REQUIRE(ReadFeatureIds(dataStructure) == expected);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("1% of 4 Features clamps up to 1 and warns")
  {
    // 0.01 * 4 = 0.04, which rounds to 0. The clamp keeps the filter doing something.
    DataStructure dataStructure = BuildTestData();
    Arguments args = MakeArgs(0ULL, 0ULL, 2ULL);
    args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_Criterion_Key, std::make_any<ChoicesParameter::ValueType>(1ULL));
    args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_Percent_Key, std::make_any<float64>(1.0));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    REQUIRE(HasWarningCode(preflightResult.outputActions.warnings(), -78121));

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const std::vector<int32> expected = {1, 1, 1, 0, 0, 1, 0, 0, 0, 0};
    REQUIRE(ReadFeatureIds(dataStructure) == expected);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::KeepRemoveRankedFeaturesFilter: Percent of Summed Value", "[SimplnxCore][KeepRemoveRankedFeatures]")
{
  UnitTest::LoadPlugins();
  KeepRemoveRankedFeaturesFilter filter;

  // Sizes 4, 1, 2, 3 sum to 10, so a percentage maps onto a cumulative sum without rounding noise.
  SECTION("70% of the summed size keeps the 2 largest")
  {
    // Target = 7.0. Cumulative largest first: 4 (below 7, continue), 7 (reaches 7, stop). k = 2.
    DataStructure dataStructure = BuildTestData();
    Arguments args = MakeArgs(0ULL, 0ULL, 2ULL);
    args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_Criterion_Key, std::make_any<ChoicesParameter::ValueType>(2ULL));
    args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_Percent_Key, std::make_any<float64>(70.0));

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const std::vector<int32> expected = {1, 1, 1, 0, 0, 1, 2, 2, 2, 0};
    REQUIRE(ReadFeatureIds(dataStructure) == expected);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("The Feature straddling the cutoff is included")
  {
    // Target = 5.0. Cumulative: 4 (below 5, continue), 7 (passes 5, stop). k = 2, not 1: the
    // Feature that crosses the threshold counts, so the selection always reaches the target.
    DataStructure dataStructure = BuildTestData();
    Arguments args = MakeArgs(0ULL, 0ULL, 2ULL);
    args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_Criterion_Key, std::make_any<ChoicesParameter::ValueType>(2ULL));
    args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_Percent_Key, std::make_any<float64>(50.0));

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const std::vector<int32> expected = {1, 1, 1, 0, 0, 1, 2, 2, 2, 0};
    REQUIRE(ReadFeatureIds(dataStructure) == expected);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("A small percentage still selects the single largest Feature")
  {
    // Target = 0.1. The first Feature accumulated already passes it, so k = 1.
    DataStructure dataStructure = BuildTestData();
    Arguments args = MakeArgs(0ULL, 0ULL, 2ULL);
    args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_Criterion_Key, std::make_any<ChoicesParameter::ValueType>(2ULL));
    args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_Percent_Key, std::make_any<float64>(1.0));

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const std::vector<int32> expected = {1, 1, 1, 0, 0, 1, 0, 0, 0, 0};
    REQUIRE(ReadFeatureIds(dataStructure) == expected);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("Negative values are an error")
  {
    DataStructure dataStructure = BuildTestData();
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_RankingArrayPath));
    auto& numElementsRef = dataStructure.getDataRefAs<Int32Array>(k_RankingArrayPath).getDataStoreRef();
    numElementsRef[2] = -5;

    Arguments args = MakeArgs(0ULL, 0ULL, 2ULL);
    args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_Criterion_Key, std::make_any<ChoicesParameter::ValueType>(2ULL));
    args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_Percent_Key, std::make_any<float64>(50.0));

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  }

  SECTION("A zero total sum is an error")
  {
    DataStructure dataStructure = BuildTestData();
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_RankingArrayPath));
    auto& numElementsRef = dataStructure.getDataRefAs<Int32Array>(k_RankingArrayPath).getDataStoreRef();
    for(usize i = 0; i < numElementsRef.getNumberOfTuples(); i++)
    {
      numElementsRef[i] = 0;
    }

    Arguments args = MakeArgs(0ULL, 0ULL, 2ULL);
    args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_Criterion_Key, std::make_any<ChoicesParameter::ValueType>(2ULL));
    args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_Percent_Key, std::make_any<float64>(50.0));

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  }
}

TEST_CASE("SimplnxCore::KeepRemoveRankedFeaturesFilter: Ties and non-finite values and fill", "[SimplnxCore][KeepRemoveRankedFeatures]")
{
  UnitTest::LoadPlugins();
  KeepRemoveRankedFeaturesFilter filter;

  SECTION("A tie straddling the cutoff warns and breaks by ascending feature id")
  {
    // Give Features 3 and 4 the same size of 3. Ranked largest first: 1 (4), then the tied pair
    // 3 and 4, then 2 (1). Keeping 2 must take Feature 1 and, of the tied pair, the lower id.
    DataStructure dataStructure = BuildTestData();
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_RankingArrayPath));
    auto& numElementsRef = dataStructure.getDataRefAs<Int32Array>(k_RankingArrayPath).getDataStoreRef();
    numElementsRef[3] = 3;

    Arguments args = MakeArgs(0ULL, 0ULL, 2ULL);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(HasWarningCode(executeResult.result.warnings(), -78122));

    // Features 1 and 3 survive and renumber to 1 and 2. Feature 3 occupied cells 4 and 9.
    const std::vector<int32> expected = {1, 1, 1, 0, 2, 1, 0, 0, 0, 2};
    REQUIRE(ReadFeatureIds(dataStructure) == expected);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("A non-finite ranking value is an error")
  {
    DataStructure dataStructure = BuildTestData();
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(k_FeatureAmPath));
    auto& featureAmRef = dataStructure.getDataRefAs<AttributeMatrix>(k_FeatureAmPath);
    auto* ratiosPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, "AspectRatios", {5}, {1}, featureAmRef.getId());
    auto& ratiosRef = ratiosPtr->getDataStoreRef();
    ratiosRef[0] = 0.0f;
    ratiosRef[1] = 1.0f;
    ratiosRef[2] = std::numeric_limits<float32>::quiet_NaN();
    ratiosRef[3] = 3.0f;
    ratiosRef[4] = 4.0f;

    Arguments args = MakeArgs(0ULL, 0ULL, 2ULL);
    args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_RankingArrayPath_Key, std::make_any<DataPath>(DataPath({k_DataContainer, k_CellFeatureData, "AspectRatios"})));

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  }

  SECTION("Fill terminates when the input has background cells")
  {
    // Cell 0 becomes background (FeatureId 0). Background is never a fill target, so it must stay 0
    // and the shared fill loop must still terminate. Before the shared utility skipped background
    // cells this section ran forever.
    DataStructure dataStructure = BuildTestData();
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath));
    dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath).getDataStoreRef()[0] = 0;
    Arguments args = MakeArgs(0ULL, 0ULL, 2ULL);
    args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_FillRemovedFeatures_Key, std::make_any<bool>(true));

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const std::vector<int32> featureIds = ReadFeatureIds(dataStructure);
    REQUIRE(featureIds[0] == 0);
    for(usize i = 1; i < featureIds.size(); i++)
    {
      REQUIRE(featureIds[i] > 0);
    }

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("Fill leaves no Cell at zero")
  {
    DataStructure dataStructure = BuildTestData();
    Arguments args = MakeArgs(0ULL, 0ULL, 2ULL);
    args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_FillRemovedFeatures_Key, std::make_any<bool>(true));

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    for(int32 featureId : ReadFeatureIds(dataStructure))
    {
      REQUIRE(featureId > 0);
    }

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::KeepRemoveRankedFeaturesFilter: Invariants", "[SimplnxCore][KeepRemoveRankedFeatures]")
{
  UnitTest::LoadPlugins();
  KeepRemoveRankedFeaturesFilter filter;

  // Sizes on the five feature fixture: 1->5, 2->1, 3->2, 4->3, 5->1.
  // Ranked largest first: 1, 4, 3, 2, 5 (2 before 5 on the id tie-break).

  SECTION("Keep k largest and Remove (N-k) smallest select the same survivors")
  {
    // k = 2 of 5, so the dual is "remove the 3 smallest" — different counts, real duality check.
    DataStructure keepDs = BuildFiveFeatureData();
    auto keepResult = filter.execute(keepDs, MakeArgs(0ULL, 0ULL, 2ULL));
    SIMPLNX_RESULT_REQUIRE_VALID(keepResult.result);

    DataStructure removeDs = BuildFiveFeatureData();
    auto removeResult = filter.execute(removeDs, MakeArgs(1ULL, 1ULL, 3ULL));
    SIMPLNX_RESULT_REQUIRE_VALID(removeResult.result);

    REQUIRE(ReadFeatureIds(keepDs) == ReadFeatureIds(removeDs));
    // Features 1 and 4 survive, renumbered 1 and 2.
    const std::vector<int32> expected = {1, 1, 1, 1, 1, 0, 0, 0, 2, 2, 2, 0};
    REQUIRE(ReadFeatureIds(keepDs) == expected);
  }

  SECTION("Survivors are renumbered contiguously from 1 with no gaps")
  {
    DataStructure dataStructure = BuildFiveFeatureData();
    auto executeResult = filter.execute(dataStructure, MakeArgs(0ULL, 0ULL, 3ULL));
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const std::set<int32> survivors = SurvivingIdSet(dataStructure);
    REQUIRE(survivors.size() == 3);
    for(int32 expectedId = 1; expectedId <= 3; expectedId++)
    {
      REQUIRE(survivors.count(expectedId) == 1);
    }

    // The compacted Attribute Matrix holds the dummy tuple plus one per survivor.
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_RankingArrayPath));
    REQUIRE(dataStructure.getDataRefAs<Int32Array>(k_RankingArrayPath).getNumberOfTuples() == 4);
  }

  SECTION("Keeping k largest is a superset of keeping k-1 largest")
  {
    // Compare against the ORIGINAL ids, since renumbering differs between the two runs.
    // Keeping 2 gives {1, 4}; keeping 3 gives {1, 4, 3}. Compare cell-wise: every cell surviving
    // under k=2 must also survive under k=3.
    DataStructure smallDs = BuildFiveFeatureData();
    auto smallDsResult = filter.execute(smallDs, MakeArgs(0ULL, 0ULL, 2ULL)).result;
    SIMPLNX_RESULT_REQUIRE_VALID(smallDsResult);
    DataStructure largeDs = BuildFiveFeatureData();
    auto largeDsResult = filter.execute(largeDs, MakeArgs(0ULL, 0ULL, 3ULL)).result;
    SIMPLNX_RESULT_REQUIRE_VALID(largeDsResult);

    const std::vector<int32> smallIds = ReadFeatureIds(smallDs);
    const std::vector<int32> largeIds = ReadFeatureIds(largeDs);
    REQUIRE(smallIds.size() == largeIds.size());
    for(usize i = 0; i < smallIds.size(); i++)
    {
      if(smallIds[i] > 0)
      {
        REQUIRE(largeIds[i] > 0);
      }
    }
  }
}

TEST_CASE("SimplnxCore::KeepRemoveRankedFeaturesFilter: Remove with percent criteria", "[SimplnxCore][KeepRemoveRankedFeatures]")
{
  UnitTest::LoadPlugins();
  KeepRemoveRankedFeaturesFilter filter;

  SECTION("Remove 40% of the Feature count, ranked smallest first")
  {
    // 0.40 * 5 = 2, so features 2 and 5 (both size 1, 2 first on id order) are removed.
    // Survivors 1, 3, 4 renumber to 1, 2, 3 by ascending original id.
    DataStructure dataStructure = BuildFiveFeatureData();
    Arguments args = MakeArgs(1ULL, 1ULL, 2ULL);
    args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_Criterion_Key, std::make_any<ChoicesParameter::ValueType>(1ULL));
    args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_Percent_Key, std::make_any<float64>(40.0));

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const std::vector<int32> expected = {1, 1, 1, 1, 1, 0, 2, 2, 3, 3, 3, 0};
    REQUIRE(ReadFeatureIds(dataStructure) == expected);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("Remove Features making up the smallest 20% of summed size")
  {
    // Total = 12, target = 2.4. Smallest first: 2 (1) -> 1.0, 5 (1) -> 2.0, 3 (2) -> 4.0 crosses.
    // So features 2, 5 and 3 are removed; 1 and 4 survive and renumber to 1 and 2.
    DataStructure dataStructure = BuildFiveFeatureData();
    Arguments args = MakeArgs(1ULL, 1ULL, 2ULL);
    args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_Criterion_Key, std::make_any<ChoicesParameter::ValueType>(2ULL));
    args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_Percent_Key, std::make_any<float64>(20.0));

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const std::vector<int32> expected = {1, 1, 1, 1, 1, 0, 0, 0, 2, 2, 2, 0};
    REQUIRE(ReadFeatureIds(dataStructure) == expected);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::KeepRemoveRankedFeaturesFilter: Unsigned ranking array", "[SimplnxCore][KeepRemoveRankedFeatures]")
{
  UnitTest::LoadPlugins();
  KeepRemoveRankedFeaturesFilter filter;

  // Exercises the is_signed_v == false branch, where the negative value check is compiled out.
  DataStructure dataStructure = BuildTestData();
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(k_FeatureAmPath));
  auto& featureAmRef = dataStructure.getDataRefAs<AttributeMatrix>(k_FeatureAmPath);
  auto* sizesPtr = UnitTest::CreateTestDataArray<uint16>(dataStructure, "UnsignedSizes", {5}, {1}, featureAmRef.getId());
  auto& sizesRef = sizesPtr->getDataStoreRef();
  const std::vector<uint16> sizes = {0, 4, 1, 2, 3};
  for(usize i = 0; i < sizes.size(); i++)
  {
    sizesRef[i] = sizes[i];
  }

  Arguments args = MakeArgs(0ULL, 0ULL, 2ULL);
  args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_RankingArrayPath_Key, std::make_any<DataPath>(DataPath({k_DataContainer, k_CellFeatureData, "UnsignedSizes"})));
  args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_Criterion_Key, std::make_any<ChoicesParameter::ValueType>(2ULL));
  args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_Percent_Key, std::make_any<float64>(70.0));

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const std::vector<int32> expected = {1, 1, 1, 0, 0, 1, 2, 2, 2, 0};
  REQUIRE(ReadFeatureIds(dataStructure) == expected);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::KeepRemoveRankedFeaturesFilter: Preflight validation", "[SimplnxCore][KeepRemoveRankedFeatures]")
{
  UnitTest::LoadPlugins();
  KeepRemoveRankedFeaturesFilter filter;

  SECTION("Number of Features of 0 is an error")
  {
    DataStructure dataStructure = BuildTestData();
    Arguments args = MakeArgs(0ULL, 0ULL, 0ULL);
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }

  SECTION("Number of Features above the feature count warns and clamps")
  {
    // 99 requested but only 4 features exist. Keep-largest so the clamp leaves everything surviving
    // rather than tripping the all-flagged error.
    DataStructure dataStructure = BuildTestData();
    Arguments args = MakeArgs(0ULL, 0ULL, 99ULL);
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    REQUIRE(HasWarningCode(preflightResult.outputActions.warnings(), -78120));
  }

  SECTION("Percent at or below 0 is an error")
  {
    DataStructure dataStructure = BuildTestData();
    Arguments args = MakeArgs(0ULL, 0ULL, 2ULL);
    args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_Criterion_Key, std::make_any<ChoicesParameter::ValueType>(1ULL));
    args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_Percent_Key, std::make_any<float64>(0.0));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }

  SECTION("Percent above 100 is an error")
  {
    DataStructure dataStructure = BuildTestData();
    Arguments args = MakeArgs(0ULL, 0ULL, 2ULL);
    args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_Criterion_Key, std::make_any<ChoicesParameter::ValueType>(1ULL));
    args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_Percent_Key, std::make_any<float64>(100.1));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }

  SECTION("A Feature Attribute Matrix holding only the dummy feature is an error")
  {
    DataStructure dataStructure = BuildTestData();
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(k_FeatureAmPath));
    auto& featureAmRef = dataStructure.getDataRefAs<AttributeMatrix>(k_FeatureAmPath);
    featureAmRef.resizeTuples({1});

    Arguments args = MakeArgs(0ULL, 0ULL, 2ULL);
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }

  SECTION("Keeping every feature flags nothing and warns")
  {
    DataStructure dataStructure = BuildTestData();
    Arguments args = MakeArgs(0ULL, 0ULL, 4ULL);
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    REQUIRE(HasWarningCode(preflightResult.outputActions.warnings(), -78123));
  }

  SECTION("Removing every feature warns at preflight and fails at execute")
  {
    DataStructure dataStructure = BuildTestData();
    Arguments args = MakeArgs(1ULL, 0ULL, 4ULL);
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    REQUIRE(HasWarningCode(preflightResult.outputActions.warnings(), -78124));

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  }

  SECTION("A ranking array whose parent is not an Attribute Matrix is an error")
  {
    DataStructure dataStructure = BuildTestData();
    // Put a feature-shaped array under a plain DataGroup rather than an Attribute Matrix.
    auto* groupPtr = DataGroup::Create(dataStructure, "NotAnAttributeMatrix");
    UnitTest::CreateTestDataArray<int32>(dataStructure, "Sizes", {5}, {1}, groupPtr->getId());

    Arguments args = MakeArgs(0ULL, 0ULL, 2ULL);
    args.insertOrAssign(KeepRemoveRankedFeaturesFilter::k_RankingArrayPath_Key, std::make_any<DataPath>(DataPath({"NotAnAttributeMatrix", "Sizes"})));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }

  SECTION("A NeighborList in the Feature Attribute Matrix produces a warning")
  {
    DataStructure dataStructure = BuildTestData();
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(k_FeatureAmPath));
    auto& featureAmRef = dataStructure.getDataRefAs<AttributeMatrix>(k_FeatureAmPath);
    auto* neighborListPtr = Int32NeighborList::Create(dataStructure, "NeighborList", ShapeType{5}, featureAmRef.getId());
    REQUIRE(neighborListPtr != nullptr);

    auto preflightResult = filter.preflight(dataStructure, MakeArgs(0ULL, 0ULL, 2ULL));
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Assert both the code and that the message names the list that will actually be removed.
    const auto& warnings = preflightResult.outputActions.warnings();
    const auto neighborListWarning = std::find_if(warnings.cbegin(), warnings.cend(), [](const Warning& warning) { return warning.code == -5558; });
    REQUIRE(neighborListWarning != warnings.cend());
    REQUIRE(neighborListWarning->message.find("NeighborList") != std::string::npos);
  }

  SECTION("Number of Features above the count clamps at execute as well as preflight")
  {
    // Keep-largest with a count of 99 clamps to 4, keeping every Feature and removing nothing.
    DataStructure dataStructure = BuildTestData();
    Arguments args = MakeArgs(0ULL, 0ULL, 99ULL);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    // Nothing removed, so FeatureIds are untouched and the Attribute Matrix keeps all 5 tuples.
    const std::vector<int32> expected = {1, 1, 1, 2, 3, 1, 4, 4, 4, 3};
    REQUIRE(ReadFeatureIds(dataStructure) == expected);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_RankingArrayPath));
    REQUIRE(dataStructure.getDataRefAs<Int32Array>(k_RankingArrayPath).getNumberOfTuples() == 5);
  }
}
