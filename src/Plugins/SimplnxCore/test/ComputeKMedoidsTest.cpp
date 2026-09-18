#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <catch2/catch.hpp>
#include <cmath>
#include <filesystem>
#include <fstream>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

#include "SimplnxCore/Filters/Algorithms/ComputeKMedoids.hpp"
#include "SimplnxCore/Filters/ComputeKMedoidsFilter.hpp"

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
constexpr std::array<uint32, 12> k_CircleIndexes = {553, 554, 555, 557, 601, 602, 649, 651, 696, 697, 742, 744};
constexpr std::array<uint32, 7> k_TriangleIndexes = {556, 600, 603, 647, 694, 743, 745};
constexpr std::array<uint32, 6> k_XIndexes = {604, 648, 650, 695, 698, 741};

const std::string k_ClusterData = "ClusterData";
const std::string k_ClusterDataNX = k_ClusterData + "NX";

const DataPath k_QuadGeomPath = DataPath({Constants::k_DataContainer});
const DataPath k_CellPath = k_QuadGeomPath.createChildPath(Constants::k_CellData);
const DataPath k_ClusterDataPathNX = k_QuadGeomPath.createChildPath(k_ClusterDataNX);

const std::string k_ClusterIdsName = "ClusterIds";
const std::string k_MedoidsName = "ClusterMedoids";
const std::string k_ClusterIdsNameNX = k_ClusterIdsName + "NX";
const std::string k_MedoidsNameNX = k_MedoidsName + "NX";

const DataPath k_ClusterIdsPathNX = k_CellPath.createChildPath(k_ClusterIdsNameNX);

const DataPath k_ParityGeomPath({"KMedoids Parity ImageGeom"});
const DataPath k_ParityCellPath = k_ParityGeomPath.createChildPath("Cell Data");
const DataPath k_ParityInputPath = k_ParityCellPath.createChildPath("Input");
const DataPath k_ParityIdsPath = k_ParityCellPath.createChildPath("Ids");
const DataPath k_ParityMaskPath = k_ParityCellPath.createChildPath("Mask");
const DataPath k_ParityFeaturePath = k_ParityGeomPath.createChildPath("Feature Data");
const DataPath k_ParityMedoidsPath = k_ParityFeaturePath.createChildPath("Medoids");

/**
 * @brief Builds deterministic two-cluster input for seeded K-medoids parity tests.
 * @param dataStructure Receives the geometry and output arrays.
 * @param maskKind 0 for no mask, 1 for Boolean, or 2 for uint8.
 * @param metric Distance metric for the returned algorithm input.
 * @return Input values that reference the created arrays.
 */
KMedoidsInputValues buildParityData(DataStructure& dataStructure, int32 maskKind, ClusterUtilities::DistanceMetric metric)
{
  const ShapeType tupleShape = {8, 1, 1};
  auto* imageGeom = ImageGeom::Create(dataStructure, "KMedoids Parity ImageGeom");
  REQUIRE(imageGeom != nullptr);
  imageGeom->setDimensions({8, 1, 1});
  auto* cellData = AttributeMatrix::Create(dataStructure, "Cell Data", tupleShape, imageGeom->getId());
  REQUIRE(cellData != nullptr);
  imageGeom->setCellData(*cellData);
  auto inputStore = std::make_shared<DataStore<float64>>(tupleShape, ShapeType{2}, std::optional<float64>{});
  REQUIRE(Float64Array::Create(dataStructure, "Input", inputStore, cellData->getId()) != nullptr);
  const std::array<float64, 16> inputValues = {1.0, 1.0, 1.0, 2.0, 2.0, 1.0, 2.0, 2.0, 8.0, 8.0, 8.0, 9.0, 9.0, 8.0, 9.0, 9.0};
  auto inputStoreWriteResult = inputStore->copyFromBuffer(0, nonstd::span<const float64>(inputValues.data(), inputValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(inputStoreWriteResult);
  auto idStore = std::make_shared<DataStore<int32>>(tupleShape, ShapeType{1}, std::optional<int32>{});
  REQUIRE(Int32Array::Create(dataStructure, "Ids", idStore, cellData->getId()) != nullptr);
  idStore->fill(97);
  if(maskKind == 1)
  {
    auto* mask = BoolArray::CreateWithStore<BoolDataStore>(dataStructure, "Mask", tupleShape, ShapeType{1}, cellData->getId());
    REQUIRE(mask != nullptr);
    const std::array<bool, 8> maskValues = {true, true, false, true, true, true, false, true};
    auto maskWriteResult = mask->getDataStoreRef().copyFromBuffer(0, nonstd::span<const bool>(maskValues.data(), maskValues.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(maskWriteResult);
  }
  else if(maskKind == 2)
  {
    auto maskStore = std::make_shared<DataStore<uint8>>(tupleShape, ShapeType{1}, std::optional<uint8>{});
    REQUIRE(UInt8Array::Create(dataStructure, "Mask", maskStore, cellData->getId()) != nullptr);
    const std::array<uint8, 8> maskValues = {1, 1, 0, 1, 1, 1, 0, 1};
    auto maskStoreWriteResult = maskStore->copyFromBuffer(0, nonstd::span<const uint8>(maskValues.data(), maskValues.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(maskStoreWriteResult);
  }
  auto* featureData = AttributeMatrix::Create(dataStructure, "Feature Data", ShapeType{3}, imageGeom->getId());
  REQUIRE(featureData != nullptr);
  auto medoidStore = std::make_shared<DataStore<float64>>(ShapeType{3}, ShapeType{2}, std::optional<float64>{});
  REQUIRE(Float64Array::Create(dataStructure, "Medoids", medoidStore, featureData->getId()) != nullptr);
  medoidStore->fill(-1.0);
  return {2, metric, maskKind != 0, k_ParityInputPath, k_ParityMaskPath, k_ParityIdsPath, k_ParityMedoidsPath, 5489};
}

/**
 * @brief Builds zero-tuple input for the empty-input error test.
 * @param dataStructure Receives the empty geometry and arrays.
 * @return Input values that reference the created arrays.
 */
KMedoidsInputValues buildEmptyData(DataStructure& dataStructure)
{
  const ShapeType tupleShape = {0, 1, 1};
  auto* imageGeom = ImageGeom::Create(dataStructure, "KMedoids Parity ImageGeom");
  REQUIRE(imageGeom != nullptr);
  imageGeom->setDimensions({0, 1, 1});
  auto* cellData = AttributeMatrix::Create(dataStructure, "Cell Data", tupleShape, imageGeom->getId());
  REQUIRE(cellData != nullptr);
  imageGeom->setCellData(*cellData);
  REQUIRE(Float64Array::CreateWithStore<Float64DataStore>(dataStructure, "Input", tupleShape, ShapeType{2}, cellData->getId()) != nullptr);
  REQUIRE(Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "Ids", tupleShape, ShapeType{1}, cellData->getId()) != nullptr);
  auto* featureData = AttributeMatrix::Create(dataStructure, "Feature Data", ShapeType{2}, imageGeom->getId());
  REQUIRE(featureData != nullptr);
  REQUIRE(Float64Array::CreateWithStore<Float64DataStore>(dataStructure, "Medoids", ShapeType{2}, ShapeType{2}, featureData->getId()) != nullptr);
  return {1, ClusterUtilities::Euclidean, false, k_ParityInputPath, {}, k_ParityIdsPath, k_ParityMedoidsPath, 5489};
}
} // namespace

TEST_CASE("SimplnxCore::ComputeKMedoids: Scanline rejects zero tuples", "[SimplnxCore][ComputeKMedoids]")
{
  DataStructure dataStructure;
  auto inputValues = buildEmptyData(dataStructure);
  const std::atomic_bool shouldCancel = false;
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  auto result = scope.execute([&] { return ComputeKMedoids(dataStructure, {}, shouldCancel, &inputValues)(); });
  REQUIRE(result.invalid());
  REQUIRE(result.errors().front().code == -54071);
}

TEST_CASE("SimplnxCore::ComputeKMedoids: cancellation leaves outputs unchanged", "[SimplnxCore][ComputeKMedoids]")
{
  const auto scenarios = UnitTest::SelectAlgorithmTestScenariosForInMemoryStores();
  for(const auto scenario : scenarios)
  {
    CAPTURE(scenario);
    {
      DataStructure dataStructure;
      auto inputValues = buildParityData(dataStructure, 0, ClusterUtilities::Euclidean);
      std::atomic_bool shouldCancel = true;
      REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_ParityIdsPath));
      REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64Array>(k_ParityMedoidsPath));
      const auto& ids = dataStructure.getDataRefAs<Int32Array>(k_ParityIdsPath);
      const auto& medoids = dataStructure.getDataRefAs<Float64Array>(k_ParityMedoidsPath);
      std::array<int32, 8> idsBefore = {}, idsAfter = {};
      std::array<float64, 6> medoidsBefore = {}, medoidsAfter = {};
      auto idsBeforeReadResult = ids.getDataStoreRef().copyIntoBuffer(0, nonstd::span<int32>(idsBefore.data(), idsBefore.size()));
      SIMPLNX_RESULT_REQUIRE_VALID(idsBeforeReadResult);
      auto medoidsBeforeReadResult = medoids.getDataStoreRef().copyIntoBuffer(0, nonstd::span<float64>(medoidsBefore.data(), medoidsBefore.size()));
      SIMPLNX_RESULT_REQUIRE_VALID(medoidsBeforeReadResult);
      UnitTest::AlgorithmTestScope scope(scenario);
      auto scopeExecutionResult = scope.execute([&] { return ComputeKMedoids(dataStructure, {}, shouldCancel, &inputValues)(); });
      SIMPLNX_RESULT_REQUIRE_VALID(scopeExecutionResult);
      auto idsAfterReadResult = ids.getDataStoreRef().copyIntoBuffer(0, nonstd::span<int32>(idsAfter.data(), idsAfter.size()));
      SIMPLNX_RESULT_REQUIRE_VALID(idsAfterReadResult);
      auto medoidsAfterReadResult = medoids.getDataStoreRef().copyIntoBuffer(0, nonstd::span<float64>(medoidsAfter.data(), medoidsAfter.size()));
      SIMPLNX_RESULT_REQUIRE_VALID(medoidsAfterReadResult);
      CHECK(idsAfter == idsBefore);
      CHECK(medoidsAfter == medoidsBefore);
    }
  }
}

TEST_CASE("SimplnxCore::ComputeKMedoids: Direct and Scanline seeded metric parity", "[SimplnxCore][ComputeKMedoids]")
{
  const auto maskKind = GENERATE(0, 1, 2);
  for(const auto metric :
      {ClusterUtilities::Euclidean, ClusterUtilities::SquaredEuclidean, ClusterUtilities::Manhattan, ClusterUtilities::Cosine, ClusterUtilities::Pearson, ClusterUtilities::SquaredPearson})
  {
    DYNAMIC_SECTION(fmt::format("mask {} metric {}", maskKind, static_cast<int32>(metric)))
    {
      const std::atomic_bool shouldCancel = false;
      std::array<int32, 8> expectedIds = {};
      std::array<float64, 6> expectedMedoids = {};
      bool hasExpectedValues = false;
      const auto scenarios = UnitTest::SelectAlgorithmTestScenariosForInMemoryStores();
      for(const auto scenario : scenarios)
      {
        CAPTURE(scenario);
        DataStructure dataStructure;
        auto values = buildParityData(dataStructure, maskKind, metric);
        UnitTest::AlgorithmTestScope scope(scenario);
        auto executeResult2 = scope.execute([&] { return ComputeKMedoids(dataStructure, {}, shouldCancel, &values)(); });
        SIMPLNX_RESULT_REQUIRE_VALID(executeResult2);
        REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_ParityIdsPath));
        REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64Array>(k_ParityMedoidsPath));
        std::array<int32, 8> ids = {};
        std::array<float64, 6> medoids = {};
        const auto& idStore = dataStructure.getDataRefAs<Int32Array>(k_ParityIdsPath).getDataStoreRef();
        const auto& medoidStore = dataStructure.getDataRefAs<Float64Array>(k_ParityMedoidsPath).getDataStoreRef();
        auto copyIntoBufferResult = idStore.copyIntoBuffer(0, nonstd::span<int32>(ids.data(), ids.size()));
        SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
        auto copyIntoBufferResult2 = medoidStore.copyIntoBuffer(0, nonstd::span<float64>(medoids.data(), medoids.size()));
        SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult2);
        if(maskKind != 0)
        {
          CHECK(ids[2] == 0);
          CHECK(ids[6] == 0);
        }
        CHECK(medoids[0] == 0.0);
        CHECK(medoids[1] == 0.0);
        const std::array<std::array<float64, 2>, 8> points = {{{1, 1}, {1, 2}, {2, 1}, {2, 2}, {8, 8}, {8, 9}, {9, 8}, {9, 9}}};
        const auto participates = [maskKind](usize tupleIdx) { return maskKind == 0 || (tupleIdx != 2 && tupleIdx != 6); };
        // A two-component analytical distance reference, independent of the
        // production distance helper. Constant vectors have correlation distance
        // one; nonconstant vectors have correlation +1 or -1.
        const auto distance = [metric](const std::array<float64, 2>& left, const std::array<float64, 2>& right) -> float64 {
          const float64 dx = left[0] - right[0];
          const float64 dy = left[1] - right[1];
          switch(metric)
          {
          case ClusterUtilities::Euclidean:
            return std::hypot(dx, dy);
          case ClusterUtilities::SquaredEuclidean:
            return dx * dx + dy * dy;
          case ClusterUtilities::Manhattan:
            return std::abs(dx) + std::abs(dy);
          case ClusterUtilities::Cosine:
            return 1.0 - (left[0] * right[0] + left[1] * right[1]) / (std::hypot(left[0], left[1]) * std::hypot(right[0], right[1]));
          case ClusterUtilities::Pearson:
          case ClusterUtilities::SquaredPearson:
            if(left[0] == left[1] || right[0] == right[1])
            {
              return 1.0;
            }
            return metric == ClusterUtilities::SquaredPearson || ((left[0] < left[1]) == (right[0] < right[1])) ? 0.0 : 2.0;
          }
          FAIL("Unexpected metric in the analytical reference");
          return 0.0;
        };
        const std::array<std::array<float64, 2>, 2> centers = {{{medoids[2], medoids[3]}, {medoids[4], medoids[5]}}};
        for(usize tupleIdx = 0; tupleIdx < points.size(); ++tupleIdx)
        {
          if(!participates(tupleIdx))
          {
            continue;
          }
          REQUIRE(ids[tupleIdx] >= 1);
          REQUIRE(ids[tupleIdx] <= 2);
          const usize clusterIdx = static_cast<usize>(ids[tupleIdx] - 1);
          CHECK(distance(points[tupleIdx], centers[clusterIdx]) <= distance(points[tupleIdx], centers[1 - clusterIdx]) + 1.0E-12);
        }
        for(usize clusterIdx = 0; clusterIdx < centers.size(); ++clusterIdx)
        {
          bool isInputMedoid = false;
          float64 medoidCost = 0.0;
          for(usize tupleIdx = 0; tupleIdx < points.size(); ++tupleIdx)
          {
            isInputMedoid = isInputMedoid || (participates(tupleIdx) && centers[clusterIdx] == points[tupleIdx]);
            if(participates(tupleIdx) && ids[tupleIdx] == static_cast<int32>(clusterIdx + 1))
            {
              medoidCost += distance(centers[clusterIdx], points[tupleIdx]);
            }
          }
          CHECK(isInputMedoid);
          // Empty clusters and duplicate medoids are valid for degenerate
          // metrics. A populated cluster must use a minimum-cost member.
          for(usize candidateIdx = 0; candidateIdx < points.size(); ++candidateIdx)
          {
            if(!participates(candidateIdx) || ids[candidateIdx] != static_cast<int32>(clusterIdx + 1))
            {
              continue;
            }
            float64 candidateCost = 0.0;
            for(usize tupleIdx = 0; tupleIdx < points.size(); ++tupleIdx)
            {
              if(participates(tupleIdx) && ids[tupleIdx] == static_cast<int32>(clusterIdx + 1))
              {
                candidateCost += distance(points[candidateIdx], points[tupleIdx]);
              }
            }
            CHECK(medoidCost <= candidateCost + 1.0E-12);
          }
        }
        if(hasExpectedValues)
        {
          CHECK(ids == expectedIds);
          CHECK(medoids == expectedMedoids);
        }
        else
        {
          expectedIds = ids;
          expectedMedoids = medoids;
          hasExpectedValues = true;
        }
      }
    }
  }
}

TEST_CASE("SimplnxCore::ComputeKMedoidsFilter: No-mask preflight does not create a cell-sized temporary mask", "[SimplnxCore][ComputeKMedoidsFilter]")
{
  UnitTest::LoadPlugins();
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "k_files_v2.tar.gz", "k_files_v2");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/k_files_v2/7_0_medoids_exemplar.dream3d", unit_test::k_TestFilesDir)));

  ComputeKMedoidsFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeKMedoidsFilter::k_UseSeed_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeKMedoidsFilter::k_SeedValue_Key, std::make_any<uint64>(5489));
  args.insertOrAssign(ComputeKMedoidsFilter::k_InitClusters_Key, std::make_any<uint64>(3));
  args.insertOrAssign(ComputeKMedoidsFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeKMedoidsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(k_CellPath.createChildPath("DAMAGE")));
  args.insertOrAssign(ComputeKMedoidsFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_ClusterIdsNameNX));
  args.insertOrAssign(ComputeKMedoidsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_ClusterDataPathNX));
  args.insertOrAssign(ComputeKMedoidsFilter::k_MedoidsArrayName_Key, std::make_any<std::string>(k_MedoidsNameNX));

  auto preflightResult = filter.preflight(dataStructure, args);
  auto assertionResult = preflightResult.outputActions;
  SIMPLNX_RESULT_REQUIRE_VALID(assertionResult);
  CHECK(preflightResult.outputActions.value().actions.size() == 4);
  CHECK(preflightResult.outputActions.value().deferredActions.empty());
}

TEST_CASE("SimplnxCore::ComputeKMedoidsFilter: Rejects unrepresentable clusters and an empty eligible mask", "[SimplnxCore][ComputeKMedoidsFilter]")
{
  UnitTest::LoadPlugins();
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "k_files_v2.tar.gz", "k_files_v2");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/k_files_v2/7_0_medoids_exemplar.dream3d", unit_test::k_TestFilesDir)));

  ComputeKMedoidsFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeKMedoidsFilter::k_UseSeed_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeKMedoidsFilter::k_SeedValue_Key, std::make_any<uint64>(5489));
  args.insertOrAssign(ComputeKMedoidsFilter::k_InitClusters_Key, std::make_any<uint64>(3));
  args.insertOrAssign(ComputeKMedoidsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(k_CellPath.createChildPath("DAMAGE")));
  args.insertOrAssign(ComputeKMedoidsFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>("MaskedIds"));
  args.insertOrAssign(ComputeKMedoidsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_QuadGeomPath.createChildPath("MaskedClusters")));
  args.insertOrAssign(ComputeKMedoidsFilter::k_MedoidsArrayName_Key, std::make_any<std::string>("MaskedMedoids"));

  args.insertOrAssign(ComputeKMedoidsFilter::k_InitClusters_Key, std::make_any<uint64>(static_cast<uint64>(std::numeric_limits<int32>::max())));
  auto overflowPreflight = filter.preflight(dataStructure, args);
  REQUIRE(overflowPreflight.outputActions.invalid());

  args.insertOrAssign(ComputeKMedoidsFilter::k_InitClusters_Key, std::make_any<uint64>(3));
  const DataPath maskPath = k_CellPath.createChildPath("AllFalseMask");
  const auto& input = dataStructure.getDataRefAs<IDataArray>(k_CellPath.createChildPath("DAMAGE"));
  auto* mask = BoolArray::CreateWithStore<BoolDataStore>(dataStructure, "AllFalseMask", input.getTupleShape(), ShapeType{1}, dataStructure.getDataRefAs<AttributeMatrix>(k_CellPath).getId());
  REQUIRE(mask != nullptr);
  args.insertOrAssign(ComputeKMedoidsFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeKMedoidsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(maskPath));
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  REQUIRE(executeResult.result.invalid());
}

TEST_CASE("SimplnxCore::ComputeKMedoidsFilter: Valid Filter Execution", "[SimplnxCore][ComputeKMedoidsFilter]")
{
  UnitTest::LoadPlugins();

  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "k_files_v2.tar.gz", "k_files_v2");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/k_files_v2/7_0_medoids_exemplar.dream3d", unit_test::k_TestFilesDir)));

  {
    ComputeKMedoidsFilter filter;
    Arguments args;

    // Use a fixed seed so both algorithm paths receive the same initial medoids.
    args.insertOrAssign(ComputeKMedoidsFilter::k_UseSeed_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeKMedoidsFilter::k_SeedValue_Key, std::make_any<uint64>(5489)); // Default Seed
    args.insertOrAssign(ComputeKMedoidsFilter::k_InitClusters_Key, std::make_any<uint64>(3));
    args.insertOrAssign(ComputeKMedoidsFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(ComputeKMedoidsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(k_CellPath.createChildPath("DAMAGE")));
    args.insertOrAssign(ComputeKMedoidsFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_ClusterIdsNameNX));
    args.insertOrAssign(ComputeKMedoidsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_ClusterDataPathNX));
    args.insertOrAssign(ComputeKMedoidsFilter::k_MedoidsArrayName_Key, std::make_any<std::string>(k_MedoidsNameNX));

    auto preflightResult = filter.preflight(dataStructure, args);
    auto assertionResult2 = preflightResult.outputActions;
    SIMPLNX_RESULT_REQUIRE_VALID(assertionResult2);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    auto assertionResult3 = executeResult.result;
    SIMPLNX_RESULT_REQUIRE_VALID(assertionResult3);
  }

  /*
   * Random distributions can assign different cluster identifiers on each platform.
   * The test therefore verifies a 5 by 5 symbol pattern instead of fixed identifiers.
   *
   * Rows 1 and 2 are `X C T C T` and `T X C C X`.
   * Rows 3 through 5 are `T X C X C`, `T C C T X`, and `C C C T C`.
   *
   * X, C, and T identify the values at indices 741, 742, and 743.
   * Those three values must differ before they define the remaining expected positions.
   */

  auto& clusterIds = dataStructure.getDataRefAs<Int32Array>(k_ClusterIdsPathNX);

  int32 xVal = clusterIds[741];
  int32 cVal = clusterIds[742];
  int32 tVal = clusterIds[743];

  REQUIRE(xVal != cVal);
  REQUIRE(cVal != tVal);
  REQUIRE(tVal != xVal);

  for(auto index : k_XIndexes)
  {
    REQUIRE(xVal == clusterIds[index]);
  }

  for(auto index : k_CircleIndexes)
  {
    REQUIRE(cVal == clusterIds[index]);
  }

  for(auto index : k_TriangleIndexes)
  {
    REQUIRE(tVal == clusterIds[index]);
  }

  // The optional output supports manual inspection of the clustering result.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_k_medoids_0_test.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeKMedoidsFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeKMedoidsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeKMedoidsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeKMedoidsFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<ComputeKMedoidsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      // Successful pipeline loading verifies the AMPathBuilderFilterParameterConverter value.
      CHECK(args.value<uint64>(ComputeKMedoidsFilter::k_InitClusters_Key) == 5);
      CHECK(args.value<ChoicesParameter::ValueType>(ComputeKMedoidsFilter::k_DistanceMetric_Key) == 0);
      CHECK(args.value<bool>(ComputeKMedoidsFilter::k_UseMask_Key) == true);
      CHECK(args.value<bool>(ComputeKMedoidsFilter::k_UseSeed_Key) == true);
      CHECK(args.value<uint64>(ComputeKMedoidsFilter::k_SeedValue_Key) == 5);
      CHECK(args.value<DataPath>(ComputeKMedoidsFilter::k_SelectedArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeKMedoidsFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeKMedoidsFilter::k_FeatureIdsArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeKMedoidsFilter::k_MedoidsArrayName_Key) == "TestName");
    }
  }
}
