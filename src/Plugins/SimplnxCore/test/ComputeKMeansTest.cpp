#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include "SimplnxCore/Filters/ComputeKMeansFilter.hpp"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <memory>
#include <optional>
#include <vector>
namespace fs = std::filesystem;

using namespace nx::core;

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
const std::string k_MeansName = "ClusterMeans";
const std::string k_ClusterIdsNameNX = k_ClusterIdsName + "NX";
const std::string k_MeansNameNX = k_MeansName + "NX";

const DataPath k_ClusterIdsPathNX = k_CellPath.createChildPath(k_ClusterIdsNameNX);

const DataPath k_MaskParityGeometryPath({"KMeans Mask Parity ImageGeom"});
const DataPath k_MaskParityCellDataPath = k_MaskParityGeometryPath.createChildPath("Cell Data");
const DataPath k_MaskParityInputPath = k_MaskParityCellDataPath.createChildPath("Input");
const DataPath k_MaskParityMaskPath = k_MaskParityCellDataPath.createChildPath("Mask");
const DataPath k_MaskParityFeatureDataPath = k_MaskParityGeometryPath.createChildPath("Feature Data");
const DataPath k_MaskParityIdsPath = k_MaskParityCellDataPath.createChildPath("Ids");
const DataPath k_MaskParityMeansPath = k_MaskParityFeatureDataPath.createChildPath("Means");

/**
 * @brief Creates an in-memory or configured OOC store for a parity test.
 * @tparam T Specifies the store element type.
 * @param dataStructure Supplies the selected store factory.
 * @param path Logical array path for an OOC store.
 * @param tupleShape Store tuple shape.
 * @param componentShape Store component shape.
 * @param useOocStore True to use the configured OOC store factory.
 * @return The created abstract store.
 */
template <typename T>
std::shared_ptr<AbstractDataStore<T>> CreateKMeansStore(DataStructure& dataStructure, const DataPath& path, const ShapeType& tupleShape, const ShapeType& componentShape, bool useOocStore)
{
  if(useOocStore)
  {
    return DataStoreUtilities::CreateDataStore<T>(dataStructure, path, tupleShape, componentShape);
  }
  return std::make_shared<DataStore<T>>(tupleShape, componentShape, T{});
}

/**
 * @brief Builds deterministic two-component input and mask arrays for K-means parity tests.
 * @tparam MaskT Specifies the Boolean or uint8 mask type.
 * @param dataStructure Receives the geometry and arrays.
 * @param tupleCount Number of input tuples.
 * @param allFalseMask True to exclude every tuple.
 * @param useOocStore True to create input and mask arrays with OOC stores.
 */
template <typename MaskT>
void BuildKMeansMaskParityData(DataStructure& dataStructure, usize tupleCount, bool allFalseMask, bool useOocStore)
{
  const ShapeType tupleShape = {tupleCount, 1, 1};
  auto* imageGeom = ImageGeom::Create(dataStructure, k_MaskParityGeometryPath.getTargetName());
  REQUIRE(imageGeom != nullptr);
  imageGeom->setDimensions({tupleCount, 1, 1});
  auto* cellData = AttributeMatrix::Create(dataStructure, k_MaskParityCellDataPath.getTargetName(), tupleShape, imageGeom->getId());
  REQUIRE(cellData != nullptr);
  imageGeom->setCellData(*cellData);

  auto inputStore = CreateKMeansStore<float32>(dataStructure, k_MaskParityInputPath, tupleShape, {2}, useOocStore);
  REQUIRE(Float32Array::Create(dataStructure, k_MaskParityInputPath.getTargetName(), inputStore, cellData->getId()) != nullptr);
  std::vector<float32> inputValues(tupleCount * 2);
  for(usize tupleIndex = 0; tupleIndex < tupleCount; tupleIndex++)
  {
    inputValues[2 * tupleIndex] = static_cast<float32>(tupleIndex % 3) * 10.0F;
    inputValues[2 * tupleIndex + 1] = static_cast<float32>((tupleIndex / 3) % 3) * 10.0F;
  }
  auto inputStoreWriteResult = inputStore->copyFromBuffer(0, nonstd::span<const float32>(inputValues.data(), inputValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(inputStoreWriteResult);

  auto maskStore = CreateKMeansStore<MaskT>(dataStructure, k_MaskParityMaskPath, tupleShape, {1}, useOocStore);
  REQUIRE(DataArray<MaskT>::Create(dataStructure, k_MaskParityMaskPath.getTargetName(), maskStore, cellData->getId()) != nullptr);
  auto maskValues = std::make_unique<MaskT[]>(tupleCount);
  for(usize tupleIndex = 0; tupleIndex < tupleCount; tupleIndex++)
  {
    maskValues[tupleIndex] = allFalseMask ? static_cast<MaskT>(0) : static_cast<MaskT>(tupleIndex % 5 != 0);
  }
  auto maskStoreWriteResult = maskStore->copyFromBuffer(0, nonstd::span<const MaskT>(maskValues.get(), tupleCount));
  SIMPLNX_RESULT_REQUIRE_VALID(maskStoreWriteResult);
}

/**
 * @brief Creates seeded K-means arguments for the mask and algorithm parity tests.
 * @return Configured arguments for two output clusters.
 */
Arguments CreateKMeansMaskParityArguments()
{
  ComputeKMeansFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeKMeansFilter::k_UseSeed_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeKMeansFilter::k_SeedValue_Key, std::make_any<uint64>(5489));
  args.insertOrAssign(ComputeKMeansFilter::k_InitClusters_Key, std::make_any<uint64>(2));
  args.insertOrAssign(ComputeKMeansFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeKMeansFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(k_MaskParityMaskPath));
  args.insertOrAssign(ComputeKMeansFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(k_MaskParityInputPath));
  args.insertOrAssign(ComputeKMeansFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_MaskParityIdsPath.getTargetName()));
  args.insertOrAssign(ComputeKMeansFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_MaskParityFeatureDataPath));
  args.insertOrAssign(ComputeKMeansFilter::k_MeansArrayName_Key, std::make_any<std::string>(k_MaskParityMeansPath.getTargetName()));
  return args;
}

/**
 * @brief Builds an in-memory fixture from explicit tuple values and an optional Boolean mask.
 * @tparam T Input element type.
 * @param dataStructure Receives the geometry and arrays.
 * @param values Input values in tuple order.
 * @param comps Number of components per tuple.
 * @param mask Optional selection flag for each tuple.
 */
template <typename T>
void BuildKMeansFixture(DataStructure& dataStructure, const std::vector<T>& values, usize comps, std::optional<std::vector<bool>> mask = std::nullopt)
{
  REQUIRE(comps > 0);
  REQUIRE_FALSE(values.empty());
  REQUIRE(values.size() % comps == 0);
  const usize tupleCount = values.size() / comps;
  const ShapeType tupleShape = {1, 1, tupleCount};
  auto* imageGeom = ImageGeom::Create(dataStructure, k_MaskParityGeometryPath.getTargetName());
  REQUIRE(imageGeom != nullptr);
  imageGeom->setDimensions({tupleCount, 1, 1});
  auto* cellData = AttributeMatrix::Create(dataStructure, k_MaskParityCellDataPath.getTargetName(), tupleShape, imageGeom->getId());
  REQUIRE(cellData != nullptr);
  imageGeom->setCellData(*cellData);

  auto inputStore = CreateKMeansStore<T>(dataStructure, k_MaskParityInputPath, tupleShape, {comps}, false);
  REQUIRE(DataArray<T>::Create(dataStructure, k_MaskParityInputPath.getTargetName(), inputStore, cellData->getId()) != nullptr);
  auto inputWriteResult = inputStore->copyFromBuffer(0, nonstd::span<const T>(values.data(), values.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(inputWriteResult);

  if(mask.has_value())
  {
    REQUIRE(mask->size() == tupleCount);
    auto maskStore = CreateKMeansStore<bool>(dataStructure, k_MaskParityMaskPath, tupleShape, {1}, false);
    REQUIRE(BoolArray::Create(dataStructure, k_MaskParityMaskPath.getTargetName(), maskStore, cellData->getId()) != nullptr);
    auto maskValues = std::make_unique<bool[]>(tupleCount);
    for(usize tupleIdx = 0; tupleIdx < tupleCount; ++tupleIdx)
    {
      maskValues[tupleIdx] = (*mask)[tupleIdx];
    }
    auto maskWriteResult = maskStore->copyFromBuffer(0, nonstd::span<const bool>(maskValues.get(), tupleCount));
    SIMPLNX_RESULT_REQUIRE_VALID(maskWriteResult);
  }
}

/**
 * @brief Reads all values from one K-means output array.
 * @tparam T Specifies the array element type.
 * @param dataStructure Contains the output array.
 * @param path Output array path.
 * @return A contiguous copy of the array values.
 */
template <typename T>
std::vector<T> ReadKMeansValues(const DataStructure& dataStructure, const DataPath& path)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(path));
  const auto& array = dataStructure.getDataRefAs<DataArray<T>>(path);
  std::vector<T> values(array.getSize());
  auto arrayReadResult = array.getDataStoreRef().copyIntoBuffer(0, nonstd::span<T>(values.data(), values.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(arrayReadResult);
  return values;
}

/**
 * @brief Reads means without requiring one output storage type.
 * @param dataStructure Contains the output array.
 * @param path Path to a float32, float64, or uint8 means array.
 * @return Means converted to float64.
 */
std::vector<float64> ReadMeansAsFloat64(const DataStructure& dataStructure, const DataPath& path)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(path));
  const auto& array = dataStructure.getDataRefAs<IDataArray>(path);
  switch(array.getDataType())
  {
  case DataType::float32: {
    const auto values = ReadKMeansValues<float32>(dataStructure, path);
    return std::vector<float64>(values.begin(), values.end());
  }
  case DataType::float64:
    return ReadKMeansValues<float64>(dataStructure, path);
  case DataType::uint8: {
    const auto values = ReadKMeansValues<uint8>(dataStructure, path);
    return std::vector<float64>(values.begin(), values.end());
  }
  default:
    FAIL("Expected float32, float64, or uint8 means.");
    return {};
  }
}
} // namespace

TEST_CASE("SimplnxCore::ComputeKMeans: Multi-component input iterates to convergence", "[SimplnxCore][ComputeKMeans]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure;
  BuildKMeansFixture<float32>(dataStructure, {21, 42, 63, 1, 2, 3, 22, 44, 66, 2, 4, 6, 23, 46, 69, 3, 6, 9, 24, 48, 72, 4, 8, 12}, 3);
  ComputeKMeansFilter filter;
  Arguments args = CreateKMeansMaskParityArguments();
  args.insertOrAssign(ComputeKMeansFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeKMeansFilter::k_DistanceMetric_Key, std::make_any<ChoicesParameter::ValueType>(0));
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto ids = ReadKMeansValues<int32>(dataStructure, k_MaskParityIdsPath);
  const auto means = ReadMeansAsFloat64(dataStructure, k_MaskParityMeansPath);
  REQUIRE(ids.size() == 8);
  REQUIRE(means.size() == 9);
  const int32 a = ids[1];
  const int32 b = ids[0];
  REQUIRE(a != b);
  REQUIRE(a >= 1);
  REQUIRE(a <= 2);
  REQUIRE(b >= 1);
  REQUIRE(b <= 2);
  for(usize tupleIdx = 0; tupleIdx < ids.size(); ++tupleIdx)
  {
    CAPTURE(tupleIdx);
    REQUIRE(ids[tupleIdx] == (tupleIdx % 2 == 1 ? a : b));
  }
  // (1 + 2 + 3 + 4) / 4 = 2.5; (21 + 22 + 23 + 24) / 4 = 22.5.
  // The other components are twice and three times these means. Bucket zero is empty.
  const std::array<float64, 3> lowMeans = {2.5, 5.0, 7.5};
  const std::array<float64, 3> highMeans = {22.5, 45.0, 67.5};
  for(usize compIdx = 0; compIdx < 3; ++compIdx)
  {
    REQUIRE(means[compIdx] == 0.0);
    REQUIRE(means[3 * static_cast<usize>(a) + compIdx] == lowMeans[compIdx]);
    REQUIRE(means[3 * static_cast<usize>(b) + compIdx] == highMeans[compIdx]);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeKMeans: Mask selecting only the final tuple initializes", "[SimplnxCore][ComputeKMeans][ComputeKMeansF2]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);

  // Before the initialization fix, use [ComputeKMeansF2] --section Scanline: Direct cannot cancel its draw loop.
  DYNAMIC_SECTION((scenario == UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore ? "Scanline" : "Direct"))
  {
    UnitTest::AlgorithmTestScope scope(scenario);
    DataStructure dataStructure;
    BuildKMeansFixture<float32>(dataStructure, {5, 9}, 1, std::vector<bool>{false, true});
    ComputeKMeansFilter filter;
    Arguments args = CreateKMeansMaskParityArguments();
    args.insertOrAssign(ComputeKMeansFilter::k_InitClusters_Key, std::make_any<uint64>(1));

    std::atomic_bool cancel = false;
    std::promise<void> finished;
    auto watchdog = std::async(std::launch::async, [&cancel, completion = finished.get_future()]() mutable {
      if(completion.wait_for(std::chrono::seconds(10)) == std::future_status::timeout)
      {
        cancel.store(true);
      }
    });
    auto executeResult = scope.executeFilter(filter, dataStructure, args, nullptr, IFilter::MessageHandler{}, cancel);
    finished.set_value();
    watchdog.get();
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const auto ids = ReadKMeansValues<int32>(dataStructure, k_MaskParityIdsPath);
    const auto means = ReadMeansAsFloat64(dataStructure, k_MaskParityMeansPath);
    const std::vector<int32> expectedIds = {0, 1};
    // The excluded value 5 belongs to bucket zero; the selected value 9 belongs to cluster one.
    const std::vector<float64> expectedMeans = {5.0, 9.0};
    REQUIRE(ids == expectedIds);
    REQUIRE(means == expectedMeans);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ComputeKMeans: Integer input means are exact", "[SimplnxCore][ComputeKMeans]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure;
  BuildKMeansFixture<uint8>(dataStructure, {100, 101, 102, 103}, 1);
  ComputeKMeansFilter filter;
  Arguments args = CreateKMeansMaskParityArguments();
  args.insertOrAssign(ComputeKMeansFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeKMeansFilter::k_InitClusters_Key, std::make_any<uint64>(1));
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(k_MaskParityMeansPath));
  const auto& meansArray = dataStructure.getDataRefAs<IDataArray>(k_MaskParityMeansPath);
  REQUIRE(meansArray.getDataType() == DataType::float64);
  const auto ids = ReadKMeansValues<int32>(dataStructure, k_MaskParityIdsPath);
  const auto means = ReadMeansAsFloat64(dataStructure, k_MaskParityMeansPath);
  const std::vector<int32> expectedIds = {1, 1, 1, 1};
  // The selected sum is 406, so its mean is 406 / 4 = 101.5. Bucket zero is empty.
  const std::vector<float64> expectedMeans = {0.0, 101.5};
  REQUIRE(ids == expectedIds);
  REQUIRE(means == expectedMeans);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeKMeans: Large integer clusters divide by the true count", "[SimplnxCore][ComputeKMeans]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure;
  std::vector<uint8> values(257, 0);
  values[0] = 255;
  BuildKMeansFixture<uint8>(dataStructure, values, 1);
  ComputeKMeansFilter filter;
  Arguments args = CreateKMeansMaskParityArguments();
  args.insertOrAssign(ComputeKMeansFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeKMeansFilter::k_InitClusters_Key, std::make_any<uint64>(1));
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto ids = ReadKMeansValues<int32>(dataStructure, k_MaskParityIdsPath);
  const auto means = ReadMeansAsFloat64(dataStructure, k_MaskParityMeansPath);
  const std::vector<int32> expectedIds(257, 1);
  REQUIRE(ids == expectedIds);
  REQUIRE(means.size() == 2);
  // One value of 255 and 256 zeros give a sum of 255 across 257 members.
  REQUIRE(means[1] == Approx(255.0 / 257.0));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeKMeans: Valid Filter Execution", "[SimplnxCore][ComputeKMeans]")
{
  UnitTest::LoadPlugins();

  // SIMPLNX_TEST_ALGORITHM_PATH selects the Direct and Scanline scenarios.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "k_files_v2.tar.gz", "k_files_v2");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/k_files_v2/7_0_means_exemplar.dream3d", unit_test::k_TestFilesDir)));

  {
    ComputeKMeansFilter filter;
    Arguments args;

    // Use a fixed seed so both algorithm paths receive the same initial clusters.
    args.insertOrAssign(ComputeKMeansFilter::k_UseSeed_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeKMeansFilter::k_SeedValue_Key, std::make_any<uint64>(5489)); // Default Seed
    args.insertOrAssign(ComputeKMeansFilter::k_InitClusters_Key, std::make_any<uint64>(3));
    args.insertOrAssign(ComputeKMeansFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(ComputeKMeansFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(k_CellPath.createChildPath("DAMAGE")));
    args.insertOrAssign(ComputeKMeansFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_ClusterIdsNameNX));
    args.insertOrAssign(ComputeKMeansFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_ClusterDataPathNX));
    args.insertOrAssign(ComputeKMeansFilter::k_MeansArrayName_Key, std::make_any<std::string>(k_MeansNameNX));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(dataStructure.getData(DataPath({"temp_mask"})) == nullptr);
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
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_k_means_0_test.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeKMeans: Direct and Scanline masked parity", "[SimplnxCore][ComputeKMeans]")
{
  UnitTest::LoadPlugins();

  const auto checkParity = [](auto maskTag) {
    using MaskT = decltype(maskTag);

    std::vector<int32> expectedIds;
    std::vector<float64> expectedMeans;
    bool hasExpectedValues = false;
    const auto scenarios = UnitTest::SelectAlgorithmTestScenariosForInMemoryStores();
    for(const auto scenario : scenarios)
    {
      CAPTURE(scenario);
      UnitTest::AlgorithmTestScope scope(scenario);
      DataStructure dataStructure;
      BuildKMeansMaskParityData<MaskT>(dataStructure, 24, false, false);
      ComputeKMeansFilter filter;
      Arguments args = CreateKMeansMaskParityArguments();
      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
      auto executeResult = scope.executeFilter(filter, dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
      REQUIRE(dataStructure.getData(DataPath({"temp_mask"})) == nullptr);
      const auto ids = ReadKMeansValues<int32>(dataStructure, k_MaskParityIdsPath);
      const auto means = ReadKMeansValues<float64>(dataStructure, k_MaskParityMeansPath);
      for(usize tupleIndex = 0; tupleIndex < ids.size(); tupleIndex++)
      {
        if(tupleIndex % 5 == 0)
        {
          CHECK(ids[tupleIndex] == 0);
        }
      }
      // Centroids are the component means of their participating members. This
      // oracle does not depend on platform-specific random cluster labels.
      REQUIRE(ids.size() == 24);
      REQUIRE(means.size() == 6);
      std::array<usize, 3> counts = {};
      std::array<std::array<float64, 2>, 3> sums = {};
      for(usize tupleIdx = 0; tupleIdx < ids.size(); ++tupleIdx)
      {
        if(tupleIdx % 5 == 0)
        {
          continue;
        }
        REQUIRE(ids[tupleIdx] >= 1);
        REQUIRE(ids[tupleIdx] <= 2);
        const usize clusterIdx = static_cast<usize>(ids[tupleIdx]);
        ++counts[clusterIdx];
        sums[clusterIdx][0] += static_cast<float64>(tupleIdx % 3) * 10.0;
        sums[clusterIdx][1] += static_cast<float64>((tupleIdx / 3) % 3) * 10.0;
      }
      CHECK(counts[1] + counts[2] == 19);
      for(usize clusterIdx = 1; clusterIdx <= 2; ++clusterIdx)
      {
        // Duplicate seeds can leave an empty cluster, whose mean is zero.
        if(counts[clusterIdx] != 0)
        {
          for(usize compIdx = 0; compIdx < 2; ++compIdx)
          {
            CHECK(means[2 * clusterIdx + compIdx] == Approx(sums[clusterIdx][compIdx] / static_cast<float64>(counts[clusterIdx])).margin(1.0E-5));
          }
        }
        else
        {
          CHECK(means[2 * clusterIdx] == 0.0F);
          CHECK(means[2 * clusterIdx + 1] == 0.0F);
        }
      }
      if(hasExpectedValues)
      {
        CHECK(ids == expectedIds);
        CHECK(means == expectedMeans);
      }
      else
      {
        expectedIds = ids;
        expectedMeans = means;
        hasExpectedValues = true;
      }
      UnitTest::CheckArraysInheritTupleDims(dataStructure);
    }
  };

  SECTION("Bool mask")
  {
    checkParity(bool{});
  }
  SECTION("UInt8 mask")
  {
    checkParity(uint8{});
  }
}

TEST_CASE("SimplnxCore::ComputeKMeans: Scanline rejects an all-false mask", "[SimplnxCore][ComputeKMeans]")
{
  UnitTest::LoadPlugins();

  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  BuildKMeansMaskParityData<bool>(dataStructure, 24, true, false);
  ComputeKMeansFilter filter;
  Arguments args = CreateKMeansMaskParityArguments();
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().at(0).code == -54063);
  REQUIRE(dataStructure.getData(DataPath({"temp_mask"})) == nullptr);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeKMeansFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeKMeansFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeKMeansFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeKMeansFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeKMeansFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      // Successful pipeline loading verifies the AMPathBuilderFilterParameterConverter value.
      CHECK(args.value<uint64>(ComputeKMeansFilter::k_InitClusters_Key) == 5);
      CHECK(args.value<ChoicesParameter::ValueType>(ComputeKMeansFilter::k_DistanceMetric_Key) == 0);
      CHECK(args.value<bool>(ComputeKMeansFilter::k_UseMask_Key) == true);
      CHECK(args.value<bool>(ComputeKMeansFilter::k_UseSeed_Key) == true);
      CHECK(args.value<uint64>(ComputeKMeansFilter::k_SeedValue_Key) == 5);
      CHECK(args.value<DataPath>(ComputeKMeansFilter::k_SelectedArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeKMeansFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeKMeansFilter::k_FeatureIdsArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeKMeansFilter::k_MeansArrayName_Key) == "TestName");
    }
  }
}
