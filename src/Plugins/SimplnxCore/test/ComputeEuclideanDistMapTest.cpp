#include "SimplnxCore/Filters/ComputeEuclideanDistMapFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <array>
#include <catch2/catch.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>

using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;
namespace fs = std::filesystem;

namespace
{
const std::string k_CalculatedPrefix = "Calculated_";
const std::string k_GBDistancesArrayName = "GBManhattanDistances";
const std::string k_TJDistancesArrayName = "TJManhattanDistances";
const std::string k_QPDistancesArrayName = "QPManhattanDistances";

constexpr usize k_LargeDimX = 200;
constexpr usize k_LargeDimY = 200;
constexpr usize k_LargeDimZ = 200;
constexpr usize k_LargeBlockSize = 25;
constexpr usize k_LargeSliceSize = k_LargeDimX * k_LargeDimY;

const DataPath k_LargeGeomPath({"LargeImageGeom"});
const DataPath k_LargeCellDataPath = k_LargeGeomPath.createChildPath(Constants::k_CellData);
const DataPath k_LargeFeatureIdsPath = k_LargeCellDataPath.createChildPath(Constants::k_FeatureIds);

/**
 * @brief Builds a 200-cubed block-patterned feature volume for timing tests.
 * @param dataStructure Receives the ImageGeom and FeatureIds array.
 * @param includeBlockedCells True to add a negative-feature barrier with one opening.
 */
void BuildLargeTestData(DataStructure& dataStructure, bool includeBlockedCells = false)
{
  const ShapeType cellTupleShape = {k_LargeDimZ, k_LargeDimY, k_LargeDimX};

  auto* imageGeom = ImageGeom::Create(dataStructure, k_LargeGeomPath.getTargetName());
  imageGeom->setDimensions({k_LargeDimX, k_LargeDimY, k_LargeDimZ});
  imageGeom->setSpacing({0.5F, 0.75F, 1.25F});
  imageGeom->setOrigin({0.0F, 0.0F, 0.0F});

  auto* cellData = AttributeMatrix::Create(dataStructure, Constants::k_CellData, cellTupleShape, imageGeom->getId());
  imageGeom->setCellData(*cellData);

  auto featureIdsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_LargeFeatureIdsPath, cellTupleShape, {1}, IDataAction::Mode::Execute);
  auto* featureIds = DataArray<int32>::Create(dataStructure, Constants::k_FeatureIds, featureIdsStore, cellData->getId());
  auto& featureIdsRef = featureIds->getDataStoreRef();

  constexpr usize k_BlocksX = k_LargeDimX / k_LargeBlockSize;
  constexpr usize k_BlocksY = k_LargeDimY / k_LargeBlockSize;
  std::vector<int32> sliceBuffer(k_LargeSliceSize);
  for(usize z = 0; z < k_LargeDimZ; z++)
  {
    for(usize y = 0; y < k_LargeDimY; y++)
    {
      for(usize x = 0; x < k_LargeDimX; x++)
      {
        const bool isBlocked = includeBlockedCells && x == k_LargeDimX / 2 && y > 2 && y + 3 < k_LargeDimY && z > 2 && z + 3 < k_LargeDimZ && !(y == k_LargeDimY / 2 && z == k_LargeDimZ / 2);
        if(isBlocked)
        {
          sliceBuffer[y * k_LargeDimX + x] = -1;
          continue;
        }

        const usize blockX = x / k_LargeBlockSize;
        const usize blockY = y / k_LargeBlockSize;
        const usize blockZ = z / k_LargeBlockSize;
        sliceBuffer[y * k_LargeDimX + x] = static_cast<int32>(blockZ * k_BlocksY * k_BlocksX + blockY * k_BlocksX + blockX + 1);
      }
    }
    featureIdsRef.copyFromBuffer(z * k_LargeSliceSize, nonstd::span<const int32>(sliceBuffer.data(), sliceBuffer.size()));
  }
}

/**
 * @brief Creates arguments for the large distance-map test volume.
 * @param calcManhattanDist True to calculate integer Manhattan distances.
 * @param calculateAllMaps True to calculate boundary, triple-line, and quad-point maps.
 * @return Configured filter arguments.
 */
Arguments CreateDistanceMapArguments(bool calcManhattanDist, bool calculateAllMaps)
{
  Arguments args;
  args.insertOrAssign(ComputeEuclideanDistMapFilter::k_CalcManhattanDist_Key, std::make_any<bool>(calcManhattanDist));
  args.insertOrAssign(ComputeEuclideanDistMapFilter::k_DoBoundaries_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeEuclideanDistMapFilter::k_DoTripleLines_Key, std::make_any<bool>(calculateAllMaps));
  args.insertOrAssign(ComputeEuclideanDistMapFilter::k_DoQuadPoints_Key, std::make_any<bool>(calculateAllMaps));
  args.insertOrAssign(ComputeEuclideanDistMapFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_LargeGeomPath));
  args.insertOrAssign(ComputeEuclideanDistMapFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_LargeFeatureIdsPath));
  args.insertOrAssign(ComputeEuclideanDistMapFilter::k_GBDistancesArrayName_Key, std::make_any<std::string>(k_GBDistancesArrayName));
  args.insertOrAssign(ComputeEuclideanDistMapFilter::k_TJDistancesArrayName_Key, std::make_any<std::string>(k_TJDistancesArrayName));
  args.insertOrAssign(ComputeEuclideanDistMapFilter::k_QPDistancesArrayName_Key, std::make_any<std::string>(k_QPDistancesArrayName));
  return args;
}

/**
 * @brief Preflights and times one distance-map execution.
 * @param dataStructure Contains the large test volume and receives output maps.
 * @param calcManhattanDist True to calculate integer Manhattan distances.
 * @param calculateAllMaps True to calculate all three map types.
 * @return Filter execution time in seconds.
 */
float64 ExecuteTimedDistanceMap(DataStructure& dataStructure, bool calcManhattanDist, bool calculateAllMaps)
{
  ComputeEuclideanDistMapFilter filter;
  const Arguments args = CreateDistanceMapArguments(calcManhattanDist, calculateAllMaps);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  const auto executeStart = std::chrono::steady_clock::now();
  auto executeResult = filter.execute(dataStructure, args);
  const auto executeStop = std::chrono::steady_clock::now();
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  return std::chrono::duration<float64>(executeStop - executeStart).count();
}

/**
 * @brief Computes a deterministic value hash for an int32 data store.
 * @param dataStore Store to read in bounded slices.
 * @return FNV-1a-style hash of the uint32 representation of each value.
 */
uint64 HashInt32Store(const AbstractDataStore<int32>& dataStore)
{
  constexpr uint64 k_OffsetBasis = 1469598103934665603ULL;
  constexpr uint64 k_Prime = 1099511628211ULL;

  uint64 hash = k_OffsetBasis;
  std::vector<int32> buffer(k_LargeSliceSize);
  for(usize offset = 0; offset < dataStore.getSize(); offset += buffer.size())
  {
    const usize count = std::min(buffer.size(), dataStore.getSize() - offset);
    dataStore.copyIntoBuffer(offset, nonstd::span<int32>(buffer.data(), count));
    for(usize index = 0; index < count; index++)
    {
      hash ^= static_cast<uint32>(buffer[index]);
      hash *= k_Prime;
    }
  }
  return hash;
}

/**
 * @brief Computes a bounded-memory hash of a data store's object bytes.
 * @tparam T Specifies the store element type.
 * @param dataStore Store to read in bounded slices.
 * @return FNV-1a-style hash of the host object representation.
 */
template <typename T>
uint64 HashDataStoreBytes(const AbstractDataStore<T>& dataStore)
{
  constexpr uint64 k_OffsetBasis = 1469598103934665603ULL;
  constexpr uint64 k_Prime = 1099511628211ULL;

  uint64 hash = k_OffsetBasis;
  std::vector<T> buffer(k_LargeSliceSize);
  for(usize offset = 0; offset < dataStore.getSize(); offset += buffer.size())
  {
    const usize count = std::min(buffer.size(), dataStore.getSize() - offset);
    dataStore.copyIntoBuffer(offset, nonstd::span<T>(buffer.data(), count));
    const auto* bytes = reinterpret_cast<const uint8*>(buffer.data());
    for(usize byteIndex = 0; byteIndex < count * sizeof(T); byteIndex++)
    {
      hash ^= bytes[byteIndex];
      hash *= k_Prime;
    }
  }
  return hash;
}

/**
 * @brief Hashes the distance maps that one test configuration creates.
 * @param dataStructure Contains the computed maps.
 * @param calcManhattanDist True if the maps contain int32 values.
 * @param calculateAllMaps True if all three map types exist.
 * @return Hashes in boundary, triple-line, and quad-point order. Missing maps use 0.
 */
std::array<uint64, 3> HashDistanceMaps(const DataStructure& dataStructure, bool calcManhattanDist, bool calculateAllMaps)
{
  const std::array<std::string, 3> k_OutputNames = {k_GBDistancesArrayName, k_TJDistancesArrayName, k_QPDistancesArrayName};
  std::array<uint64, 3> hashes = {};
  const usize outputCount = calculateAllMaps ? k_OutputNames.size() : 1;
  for(usize outputIndex = 0; outputIndex < outputCount; outputIndex++)
  {
    const DataPath outputPath = k_LargeCellDataPath.createChildPath(k_OutputNames[outputIndex]);
    if(calcManhattanDist)
    {
      REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(outputPath));
      hashes[outputIndex] = HashDataStoreBytes(dataStructure.getDataRefAs<Int32Array>(outputPath).getDataStoreRef());
    }
    else
    {
      REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(outputPath));
      hashes[outputIndex] = HashDataStoreBytes(dataStructure.getDataRefAs<Float32Array>(outputPath).getDataStoreRef());
    }
  }
  return hashes;
}

/**
 * @brief Builds a 24-cubed block-patterned feature volume for algorithm comparisons.
 * @param dataStructure Receives the ImageGeom and FeatureIds array.
 * @param includeBlockedCells True to add a negative-feature barrier with one opening.
 */
void BuildComparisonTestData(DataStructure& dataStructure, bool includeBlockedCells)
{
  constexpr usize k_DimX = 24;
  constexpr usize k_DimY = 24;
  constexpr usize k_DimZ = 24;
  constexpr usize k_BlockSize = 6;
  constexpr usize k_SliceSize = k_DimX * k_DimY;
  constexpr usize k_BlocksX = k_DimX / k_BlockSize;
  constexpr usize k_BlocksY = k_DimY / k_BlockSize;
  const ShapeType cellTupleShape = {k_DimZ, k_DimY, k_DimX};

  auto* imageGeom = ImageGeom::Create(dataStructure, k_LargeGeomPath.getTargetName());
  imageGeom->setDimensions({k_DimX, k_DimY, k_DimZ});
  imageGeom->setSpacing({0.5F, 0.75F, 1.25F});
  imageGeom->setOrigin({0.0F, 0.0F, 0.0F});

  auto* cellData = AttributeMatrix::Create(dataStructure, Constants::k_CellData, cellTupleShape, imageGeom->getId());
  imageGeom->setCellData(*cellData);

  auto featureIdsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_LargeFeatureIdsPath, cellTupleShape, {1}, IDataAction::Mode::Execute);
  auto* featureIds = DataArray<int32>::Create(dataStructure, Constants::k_FeatureIds, featureIdsStore, cellData->getId());
  auto& featureIdsRef = featureIds->getDataStoreRef();

  std::vector<int32> sliceBuffer(k_SliceSize);
  for(usize z = 0; z < k_DimZ; z++)
  {
    for(usize y = 0; y < k_DimY; y++)
    {
      for(usize x = 0; x < k_DimX; x++)
      {
        const bool isBlocked = includeBlockedCells && x == k_DimX / 2 && y > 2 && y + 3 < k_DimY && z > 2 && z + 3 < k_DimZ && !(y == k_DimY / 2 && z == k_DimZ / 2);
        if(isBlocked)
        {
          sliceBuffer[y * k_DimX + x] = -1;
          continue;
        }

        const usize blockX = x / k_BlockSize;
        const usize blockY = y / k_BlockSize;
        const usize blockZ = z / k_BlockSize;
        sliceBuffer[y * k_DimX + x] = static_cast<int32>(blockZ * k_BlocksY * k_BlocksX + blockY * k_BlocksX + blockX + 1);
      }
    }
    featureIdsRef.copyFromBuffer(z * k_SliceSize, nonstd::span<const int32>(sliceBuffer.data(), sliceBuffer.size()));
  }
}

/**
 * @brief Executes all Euclidean distance maps for an algorithm-comparison volume.
 * @param dataStructure Contains the input volume and receives the three maps.
 */
void ExecuteComparisonDistanceMap(DataStructure& dataStructure)
{
  ComputeEuclideanDistMapFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeEuclideanDistMapFilter::k_CalcManhattanDist_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeEuclideanDistMapFilter::k_DoBoundaries_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeEuclideanDistMapFilter::k_DoTripleLines_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeEuclideanDistMapFilter::k_DoQuadPoints_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeEuclideanDistMapFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_LargeGeomPath));
  args.insertOrAssign(ComputeEuclideanDistMapFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_LargeFeatureIdsPath));
  args.insertOrAssign(ComputeEuclideanDistMapFilter::k_GBDistancesArrayName_Key, std::make_any<std::string>(k_GBDistancesArrayName));
  args.insertOrAssign(ComputeEuclideanDistMapFilter::k_TJDistancesArrayName_Key, std::make_any<std::string>(k_TJDistancesArrayName));
  args.insertOrAssign(ComputeEuclideanDistMapFilter::k_QPDistancesArrayName_Key, std::make_any<std::string>(k_QPDistancesArrayName));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

/**
 * @brief Tests whether a calculated array exists in the Small IN100 CellData group.
 * @param dataStructure DataStructure to inspect.
 * @param name Base array name without the calculated prefix.
 * @return True if the calculated IDataArray exists.
 */
bool ArrayExists(const DataStructure& dataStructure, const std::string& name)
{
  const DataPath calculatedPath({k_DataContainer, k_CellData, std::string(k_CalculatedPrefix) + name});
  return dataStructure.getDataAs<IDataArray>(calculatedPath) != nullptr;
};
} // namespace

TEST_CASE("SimplnxCore::ComputeEuclideanDistMap", "[SimplnxCore][ComputeEuclideanDistMap]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_stats_test_v2.tar.gz", "6_6_stats_test_v2.dream3d");

  // Load the Small IN100 exemplar for the output-selection matrix.
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_stats_test_v2.dream3d", unit_test::k_TestFilesDir));
  const DataPath k_CellFeatureDataAM = k_DataContainerPath.createChildPath("CellFeatureData");

  // Each scenario creates one selected map and compares it with its exemplar.
  auto [scenarioName, doBoundaries, doTripleLines, doQuadPoints, expectedArrayName] = GENERATE(std::make_tuple("Boundaries only (GB distances)", true, false, false, k_GBDistancesArrayName),
                                                                                               std::make_tuple("Triple lines only (TJ distances)", false, true, false, k_TJDistancesArrayName),
                                                                                               std::make_tuple("Quad points only (QP distances)", false, false, true, k_QPDistancesArrayName));

  INFO("Scenario: " << scenarioName);

  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    ComputeEuclideanDistMapFilter filter;
    Arguments args;

    // Select one map type for this scenario.
    args.insert(ComputeEuclideanDistMapFilter::k_CalcManhattanDist_Key, std::make_any<bool>(true));
    args.insert(ComputeEuclideanDistMapFilter::k_DoBoundaries_Key, std::make_any<bool>(doBoundaries));
    args.insert(ComputeEuclideanDistMapFilter::k_DoTripleLines_Key, std::make_any<bool>(doTripleLines));
    args.insert(ComputeEuclideanDistMapFilter::k_DoQuadPoints_Key, std::make_any<bool>(doQuadPoints));

    args.insert(ComputeEuclideanDistMapFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insert(ComputeEuclideanDistMapFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_CellAttributeMatrix.createChildPath(k_FeatureIds)));

    args.insert(ComputeEuclideanDistMapFilter::k_GBDistancesArrayName_Key, std::make_any<std::string>(k_CalculatedPrefix + k_GBDistancesArrayName));
    args.insert(ComputeEuclideanDistMapFilter::k_TJDistancesArrayName_Key, std::make_any<std::string>(k_CalculatedPrefix + k_TJDistancesArrayName));
    args.insert(ComputeEuclideanDistMapFilter::k_QPDistancesArrayName_Key, std::make_any<std::string>(k_CalculatedPrefix + k_QPDistancesArrayName));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Only the selected map can exist after this scenario.
  for(const auto& outputName : {k_GBDistancesArrayName, k_TJDistancesArrayName, k_QPDistancesArrayName})
  {
    const bool shouldExist = (outputName == expectedArrayName);
    INFO("  Output: " << outputName << " (expected " << (shouldExist ? "present" : "absent") << ")");

    if(shouldExist)
    {
      REQUIRE(ArrayExists(dataStructure, outputName));
    }
    else
    {
      REQUIRE_FALSE(ArrayExists(dataStructure, outputName));
    }
  }

  // The selected output must match its exemplar array.
  const DataPath exemplarPath({k_DataContainer, k_CellData, expectedArrayName});
  const DataPath calculatedPath({k_DataContainer, k_CellData, k_CalculatedPrefix + expectedArrayName});
  const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
  const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
  UnitTest::CompareDataArrays<int32>(exemplarData, calculatedData);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeEuclideanDistMap: Euclidean direct and OOC equivalence", "[SimplnxCore][ComputeEuclideanDistMap]")
{
  UnitTest::LoadPlugins();
  const bool includeBlockedCells = GENERATE(false, true);

  DYNAMIC_SECTION("blocked cells: " << includeBlockedCells)
  {
    std::array<std::vector<float32>, 3> referenceOutputs;
    bool hasReferenceOutputs = false;
    const auto scenarios = UnitTest::SelectAlgorithmTestScenariosForInMemoryStores();
    for(const auto scenario : scenarios)
    {
      CAPTURE(scenario);
      UnitTest::AlgorithmTestScope scope(scenario);
      DataStructure dataStructure;
      BuildComparisonTestData(dataStructure, includeBlockedCells);
      scope.requireExpectedStore(dataStructure.getDataRefAs<Int32Array>(k_LargeFeatureIdsPath));
      scope.execute([&dataStructure] { ExecuteComparisonDistanceMap(dataStructure); });

      const std::array<std::string, 3> outputNames = {k_GBDistancesArrayName, k_TJDistancesArrayName, k_QPDistancesArrayName};
      for(usize outputIndex = 0; outputIndex < outputNames.size(); outputIndex++)
      {
        const DataPath outputPath = k_LargeCellDataPath.createChildPath(outputNames[outputIndex]);
        REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(outputPath));
        const auto& outputArray = dataStructure.getDataRefAs<Float32Array>(outputPath);
        std::vector<float32> outputValues(outputArray.getSize());
        SIMPLNX_RESULT_REQUIRE_VALID(outputArray.getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(outputValues.data(), outputValues.size())));
        if(hasReferenceOutputs)
        {
          CHECK(outputValues == referenceOutputs[outputIndex]);
        }
        else
        {
          referenceOutputs[outputIndex] = std::move(outputValues);
        }
      }

      hasReferenceOutputs = true;
      UnitTest::CheckArraysInheritTupleDims(dataStructure);
    }
  }
}

TEST_CASE("SimplnxCore::ComputeEuclideanDistMap: 200x200x200 same-storage algorithm matrix", "[SimplnxCore][ComputeEuclideanDistMap][.Performance]")
{
  UnitTest::LoadPlugins();

  auto [caseName, calcManhattanDist, includeBlockedCells, calculateAllMaps] =
      GENERATE(std::make_tuple("Manhattan, unblocked, boundaries only", true, false, false), std::make_tuple("Manhattan, unblocked, all maps", true, false, true),
               std::make_tuple("Manhattan, blocked, boundaries only", true, true, false), std::make_tuple("Manhattan, blocked, all maps", true, true, true),
               std::make_tuple("Euclidean, unblocked, boundaries only", false, false, false), std::make_tuple("Euclidean, unblocked, all maps", false, false, true),
               std::make_tuple("Euclidean, blocked, boundaries only", false, true, false), std::make_tuple("Euclidean, blocked, all maps", false, true, true));
  const bool shouldCalculateManhattanDistance = calcManhattanDist;
  const bool shouldCalculateAllMaps = calculateAllMaps;

  DYNAMIC_SECTION(caseName)
  {
    std::array<uint64, 3> directHashes = {};
    {
      UnitTest::AlgorithmTestScope directScope(UnitTest::AlgorithmTestScenario::InCoreAlgorithmOnInMemoryStore);
      DataStructure directDataStructure;
      BuildLargeTestData(directDataStructure, includeBlockedCells);
      REQUIRE_NOTHROW(directDataStructure.getDataRefAs<Int32Array>(k_LargeFeatureIdsPath));
      directScope.requireExpectedStore(directDataStructure.getDataRefAs<Int32Array>(k_LargeFeatureIdsPath));

      directScope.execute([&directDataStructure, shouldCalculateManhattanDistance, shouldCalculateAllMaps] {
        return ExecuteTimedDistanceMap(directDataStructure, shouldCalculateManhattanDistance, shouldCalculateAllMaps);
      });
      directHashes = HashDistanceMaps(directDataStructure, calcManhattanDist, calculateAllMaps);
      UnitTest::CheckArraysInheritTupleDims(directDataStructure);
    }

    std::array<uint64, 3> scanlineHashes = {};
    {
      UnitTest::AlgorithmTestScope oocScope(UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
      DataStructure scanlineDataStructure;
      BuildLargeTestData(scanlineDataStructure, includeBlockedCells);
      REQUIRE_NOTHROW(scanlineDataStructure.getDataRefAs<Int32Array>(k_LargeFeatureIdsPath));
      oocScope.requireExpectedStore(scanlineDataStructure.getDataRefAs<Int32Array>(k_LargeFeatureIdsPath));

      oocScope.execute([&scanlineDataStructure, shouldCalculateManhattanDistance, shouldCalculateAllMaps] {
        return ExecuteTimedDistanceMap(scanlineDataStructure, shouldCalculateManhattanDistance, shouldCalculateAllMaps);
      });
      scanlineHashes = HashDistanceMaps(scanlineDataStructure, calcManhattanDist, calculateAllMaps);
      UnitTest::CheckArraysInheritTupleDims(scanlineDataStructure);
    }

    CHECK(directHashes == scanlineHashes);
  }
}

TEST_CASE("SimplnxCore::ComputeEuclideanDistMapFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeEuclideanDistMapFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeEuclideanDistMapFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeEuclideanDistMapFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeEuclideanDistMapFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<bool>(ComputeEuclideanDistMapFilter::k_CalcManhattanDist_Key) == true);
      CHECK(args.value<bool>(ComputeEuclideanDistMapFilter::k_DoBoundaries_Key) == true);
      CHECK(args.value<bool>(ComputeEuclideanDistMapFilter::k_DoTripleLines_Key) == true);
      CHECK(args.value<bool>(ComputeEuclideanDistMapFilter::k_DoQuadPoints_Key) == true);
      CHECK(args.value<DataPath>(ComputeEuclideanDistMapFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ComputeEuclideanDistMapFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeEuclideanDistMapFilter::k_GBDistancesArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeEuclideanDistMapFilter::k_TJDistancesArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeEuclideanDistMapFilter::k_QPDistancesArrayName_Key) == "TestName");
    }
  }
}
