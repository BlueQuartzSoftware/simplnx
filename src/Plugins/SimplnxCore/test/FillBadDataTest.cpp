#include "SimplnxCore/Filters/FillBadDataFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include <catch2/catch.hpp>
#include <nonstd/span.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Pipeline/AbstractPipelineNode.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::constants;
using namespace nx::core::unit_test;

namespace
{
/**
 * @brief Builds a FillBadData test dataset with block-patterned FeatureIds
 * and ~10% scattered bad voxels (FeatureId=0) using a deterministic pattern.
 */
void BuildFillBadDataTestData(DataStructure& dataStructure, usize dimX, usize dimY, usize dimZ, usize blockSize, bool addLargeDefect = false)
{
  const ShapeType cellShape = {dimZ, dimY, dimX};
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "DataContainer");
  imageGeomPtr->setDimensions({dimX, dimY, dimZ});
  REQUIRE(imageGeomPtr->setSpacing({1.0f, 1.0f, 1.0f}).valid());
  REQUIRE(imageGeomPtr->setOrigin({0.0f, 0.0f, 0.0f}).valid());

  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", cellShape, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);

  const DataPath featureIdsPath = DataPath({"DataContainer", "CellData", "FeatureIds"});
  auto featureIdsDataStore = data_store_utilities::CreateDataStore<int32>(dataStructure, featureIdsPath, cellShape, {1});
  auto* featureIdsArrayPtr = DataArray<int32>::Create(dataStructure, "FeatureIds", featureIdsDataStore, cellAmPtr->getId());
  auto& featureIdsStore = featureIdsArrayPtr->getDataStoreRef();

  const DataPath phasesPath = DataPath({"DataContainer", "CellData", "Phases"});
  auto phasesDataStore = data_store_utilities::CreateDataStore<int32>(dataStructure, phasesPath, cellShape, {1});
  auto* phasesArrayPtr = DataArray<int32>::Create(dataStructure, "Phases", phasesDataStore, cellAmPtr->getId());
  auto& phasesStore = phasesArrayPtr->getDataStoreRef();

  const usize blocksPerDim = dimX / blockSize;
  const usize sliceSize = dimY * dimX;
  std::vector<int32> featureIdsSliceBuffer(sliceSize);
  std::vector<int32> phasesSliceBuffer(sliceSize, 1);

  for(usize z = 0; z < dimZ; z++)
  {
    for(usize y = 0; y < dimY; y++)
    {
      for(usize x = 0; x < dimX; x++)
      {
        usize bx = x / blockSize;
        usize by = y / blockSize;
        usize bz = z / blockSize;
        int32 blockFeatureId = static_cast<int32>(bz * blocksPerDim * blocksPerDim + by * blocksPerDim + bx + 1);

        // Scatter bad voxels: ~10% of voxels become bad (FeatureId=0)
        bool isBad = ((x * 7 + y * 13 + z * 29) % 10 == 0);
        featureIdsSliceBuffer[y * dimX + x] = isBad ? 0 : blockFeatureId;
      }
    }

    // Add a contiguous large defect: entire z=dimZ/2 plane set to FeatureId=0
    if(addLargeDefect && z == dimZ / 2)
    {
      std::fill(featureIdsSliceBuffer.begin(), featureIdsSliceBuffer.end(), 0);
    }

    auto featureIdsStoreWriteResult = featureIdsStore.copyFromBuffer(z * sliceSize, nonstd::span<const int32>(featureIdsSliceBuffer.data(), sliceSize));
    SIMPLNX_RESULT_REQUIRE_VALID(featureIdsStoreWriteResult);
    auto phasesStoreWriteResult = phasesStore.copyFromBuffer(z * sliceSize, nonstd::span<const int32>(phasesSliceBuffer.data(), sliceSize));
    SIMPLNX_RESULT_REQUIRE_VALID(phasesStoreWriteResult);
  }
}
// These paths select the FillBadData exemplar archive.
const std::string k_ArchiveName = "fill_bad_data_exemplars.tar.gz";
const std::string k_DataDirName = "fill_bad_data_exemplars";
const fs::path k_DataDir = fs::path(unit_test::k_TestFilesDir.view()) / k_DataDirName;
const fs::path k_ExemplarFile = k_DataDir / "fill_bad_data.dream3d";

// These dimensions define the large generated fixtures.
constexpr usize k_Dim = 200;
constexpr usize k_BlockSize = 25;
constexpr int32 k_MinDefectSize = 50;
} // namespace

TEST_CASE("SimplnxCore::FillBadData_SmallIN100", "[Core][FillBadDataFilter]")
{
  // Load the application and plugins.
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "6_5_fill_bad_data.tar.gz", "6_5_fill_bad_data");
  // Load the exemplar output.
  auto exemplarFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/6_5_exemplar.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = unit_test::LoadDataStructure(exemplarFilePath);

  // Load the Small IN100 input.
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/6_5_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  {
    // Configure the filter arguments.
    FillBadDataFilter filter;
    Arguments args;

    args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(1000));
    args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));
    args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(constants::k_FeatureIdsArrayPath));
    args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(constants::k_PhasesArrayPath));

    args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
    args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(constants::k_DataContainerPath));

    // Preflight the filter and check the result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  unit_test::CompareExemplarToGeneratedData(dataStructure, exemplarDataStructure, constants::k_CellAttributeMatrix, constants::k_DataContainer);

// The optional output supports manual inspection.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_fill_bad_data.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData::Test01_SingleSmallDefect", "[Core][FillBadDataFilter]")
{
  unit_test::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  // Configure OOC settings for this scenario.

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_fill_bad_data.tar.gz", "6_5_fill_bad_data");

  // Load the input data.
  auto inputFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_01_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(inputFilePath);

  // Load the expected output.
  auto expectedFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_01_expected.dream3d", unit_test::k_TestFilesDir));
  DataStructure expectedDataStructure = unit_test::LoadDataStructure(expectedFilePath);

  scope.requireExpectedStore(dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"DataContainer", "CellData", "FeatureIds"})));

  // Execute the filter.
  FillBadDataFilter filter;
  Arguments args;
  args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(20));
  args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "FeatureIds"})));
  args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "Phases"})));
  args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"DataContainer"})));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Compare the generated results.
  unit_test::CompareExemplarToGeneratedData(dataStructure, expectedDataStructure, DataPath({"DataContainer", "CellData"}), "DataContainer");

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData::Test02_SingleLargeDefect", "[Core][FillBadDataFilter]")
{
  unit_test::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  // Configure OOC settings for this scenario.

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_fill_bad_data.tar.gz", "6_5_fill_bad_data");

  // Load the input data.
  auto inputFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_02_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(inputFilePath);

  // Load the expected output.
  auto expectedFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_02_expected.dream3d", unit_test::k_TestFilesDir));
  DataStructure expectedDataStructure = unit_test::LoadDataStructure(expectedFilePath);

  scope.requireExpectedStore(dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"DataContainer", "CellData", "FeatureIds"})));

  // Execute the filter.
  FillBadDataFilter filter;
  Arguments args;
  args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(20));
  args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "FeatureIds"})));
  args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "Phases"})));
  args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"DataContainer"})));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Compare the generated results.
  unit_test::CompareExemplarToGeneratedData(dataStructure, expectedDataStructure, DataPath({"DataContainer", "CellData"}), "DataContainer");

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData::Test03_ThresholdBoundary", "[Core][FillBadDataFilter]")
{
  unit_test::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  // Configure OOC settings for this scenario.

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_fill_bad_data.tar.gz", "6_5_fill_bad_data");

  auto inputFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_03_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(inputFilePath);

  auto expectedFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_03_expected.dream3d", unit_test::k_TestFilesDir));
  DataStructure expectedDataStructure = unit_test::LoadDataStructure(expectedFilePath);

  scope.requireExpectedStore(dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"DataContainer", "CellData", "FeatureIds"})));

  FillBadDataFilter filter;
  Arguments args;
  args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(25));
  args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "FeatureIds"})));
  args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "Phases"})));
  args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"DataContainer"})));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  unit_test::CompareExemplarToGeneratedData(dataStructure, expectedDataStructure, DataPath({"DataContainer", "CellData"}), "DataContainer");
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData::Test04_MultipleSmallDefects", "[Core][FillBadDataFilter]")
{
  unit_test::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  // Configure OOC settings for this scenario.

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_fill_bad_data.tar.gz", "6_5_fill_bad_data");

  auto inputFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_04_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(inputFilePath);

  auto expectedFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_04_expected.dream3d", unit_test::k_TestFilesDir));
  DataStructure expectedDataStructure = unit_test::LoadDataStructure(expectedFilePath);

  scope.requireExpectedStore(dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"DataContainer", "CellData", "FeatureIds"})));

  FillBadDataFilter filter;
  Arguments args;
  args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(50));
  args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "FeatureIds"})));
  args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "Phases"})));
  args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"DataContainer"})));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  unit_test::CompareExemplarToGeneratedData(dataStructure, expectedDataStructure, DataPath({"DataContainer", "CellData"}), "DataContainer");
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData::Test05_MixedSmallAndLarge", "[Core][FillBadDataFilter]")
{
  unit_test::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  // Configure OOC settings for this scenario.

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_fill_bad_data.tar.gz", "6_5_fill_bad_data");

  auto inputFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_05_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(inputFilePath);

  auto expectedFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_05_expected.dream3d", unit_test::k_TestFilesDir));
  DataStructure expectedDataStructure = unit_test::LoadDataStructure(expectedFilePath);

  scope.requireExpectedStore(dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"DataContainer", "CellData", "FeatureIds"})));

  FillBadDataFilter filter;
  Arguments args;
  args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(50));
  args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "FeatureIds"})));
  args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "Phases"})));
  args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"DataContainer"})));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  unit_test::CompareExemplarToGeneratedData(dataStructure, expectedDataStructure, DataPath({"DataContainer", "CellData"}), "DataContainer");
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData::Test06_SingleVoxelDefects", "[Core][FillBadDataFilter]")
{
  unit_test::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  // Configure OOC settings for this scenario.

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_fill_bad_data.tar.gz", "6_5_fill_bad_data");

  auto inputFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_06_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(inputFilePath);

  auto expectedFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_06_expected.dream3d", unit_test::k_TestFilesDir));
  DataStructure expectedDataStructure = unit_test::LoadDataStructure(expectedFilePath);

  scope.requireExpectedStore(dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"DataContainer", "CellData", "FeatureIds"})));

  FillBadDataFilter filter;
  Arguments args;
  args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(10));
  args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "FeatureIds"})));
  args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "Phases"})));
  args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"DataContainer"})));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  unit_test::CompareExemplarToGeneratedData(dataStructure, expectedDataStructure, DataPath({"DataContainer", "CellData"}), "DataContainer");
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData::Test07_DefectsAtBoundaries", "[Core][FillBadDataFilter]")
{
  unit_test::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  // Configure OOC settings for this scenario.

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_fill_bad_data.tar.gz", "6_5_fill_bad_data");

  auto inputFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_07_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(inputFilePath);

  auto expectedFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_07_expected.dream3d", unit_test::k_TestFilesDir));
  DataStructure expectedDataStructure = unit_test::LoadDataStructure(expectedFilePath);

  FillBadDataFilter filter;
  Arguments args;
  args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(20));
  args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "FeatureIds"})));
  args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "Phases"})));
  args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"DataContainer"})));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  unit_test::CompareExemplarToGeneratedData(dataStructure, expectedDataStructure, DataPath({"DataContainer", "CellData"}), "DataContainer");
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData::Test11_NeighborTieBreaking", "[Core][FillBadDataFilter]")
{
  unit_test::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  // Configure OOC settings for this scenario.

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_fill_bad_data.tar.gz", "6_5_fill_bad_data");

  // Load the input data.
  auto inputFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_11_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(inputFilePath);

  // Load the expected output.
  auto expectedFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_11_expected.dream3d", unit_test::k_TestFilesDir));
  DataStructure expectedDataStructure = unit_test::LoadDataStructure(expectedFilePath);

  // Execute the filter.
  FillBadDataFilter filter;
  Arguments args;
  args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(10));
  args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "FeatureIds"})));
  args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "Phases"})));
  args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"DataContainer"})));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Compare the generated results.
  unit_test::CompareExemplarToGeneratedData(dataStructure, expectedDataStructure, DataPath({"DataContainer", "CellData"}), "DataContainer");

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData::Test13_StoreAsNewPhase", "[Core][FillBadDataFilter]")
{
  unit_test::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  // Configure OOC settings for this scenario.

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_fill_bad_data.tar.gz", "6_5_fill_bad_data");

  // Load the input data.
  auto inputFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_13_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(inputFilePath);

  // Load the expected output.
  auto expectedFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_13_expected.dream3d", unit_test::k_TestFilesDir));
  DataStructure expectedDataStructure = unit_test::LoadDataStructure(expectedFilePath);

  // Execute the filter.
  FillBadDataFilter filter;
  Arguments args;
  args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(20));
  args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(true));
  args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "FeatureIds"})));
  args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "Phases"})));
  args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"DataContainer"})));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Compare the generated results.
  unit_test::CompareExemplarToGeneratedData(dataStructure, expectedDataStructure, DataPath({"DataContainer", "CellData"}), "DataContainer");

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadDataFilter: Preflight Feature Data Modification Warning", "[Core][FillBadDataFilter]")
{
  unit_test::LoadPlugins();

  DataStructure dataStructure;
  BuildFillBadDataTestData(dataStructure, 2, 2, 2, 1);

  FillBadDataFilter filter;
  Arguments args;
  args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(1));
  args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"DataContainer"})));
  args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "FeatureIds"})));
  args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "Phases"})));
  args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));

  const auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.output_actions.valid());
  const auto& warnings = preflightResult.output_actions.warnings();
  REQUIRE(std::any_of(warnings.begin(), warnings.end(), [](const auto& warning) { return warning.code == -14600; }));
  const auto& outputValues = preflightResult.output_values;
  REQUIRE(std::any_of(outputValues.begin(), outputValues.end(), [](const auto& value) { return value.name == "Feature Data Modification Warning"; }));

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadDataFilter:: Invalid Preflight Min Defect Size", "[Core][FillBadDataFilter]")
{
  DataStructure dataStructure;
  const DataPath kGeomPath({"DataContainer"});
  const DataPath kFeatureIdsPath({"DataContainer", "CellData", "FeatureIds"});

  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "DataContainer");
  imageGeomPtr->setDimensions({1, 1, 1});
  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, 1, 1}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellDataPtr);
  unit_test::CreateTestDataArray<int32>(dataStructure, "FeatureIds", {1, 1, 1}, {1}, cellDataPtr->getId());

  FillBadDataFilter filter;
  Arguments args;
  args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(0));
  args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(kGeomPath));
  args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(kFeatureIdsPath));
  args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.output_actions);
  REQUIRE(preflightResult.output_actions.errors().size() == 1);
  REQUIRE(preflightResult.output_actions.errors()[0].code == -16500);
}

// A volume with no good neighbor cannot be filled.
// The no-progress guard must return and leave those voxels unchanged.
TEST_CASE("SimplnxCore::FillBadData::AllBadData_TerminatesWithoutHang", "[Core][FillBadDataFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure;
  const DataPath kGeomPath({"DataContainer"});
  const DataPath kCellDataPath = kGeomPath.createChildPath("CellData");
  const DataPath kFeatureIdsPath = kCellDataPath.createChildPath("FeatureIds");
  const DataPath kPhasesPath = kCellDataPath.createChildPath("Phases");

  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "DataContainer");
  imageGeomPtr->setDimensions({3, 3, 1});
  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, 3, 3}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellDataPtr);
  auto* featureIdsPtr = unit_test::CreateTestDataArray<int32>(dataStructure, "FeatureIds", {1, 3, 3}, {1}, cellDataPtr->getId());
  auto* phasesPtr = unit_test::CreateTestDataArray<int32>(dataStructure, "Phases", {1, 3, 3}, {1}, cellDataPtr->getId());
  // Every voxel is bad data (featureId 0 -> marked for fill); there is no good neighbor to fill from.
  featureIdsPtr->fill(0);
  phasesPtr->fill(1);

  FillBadDataFilter filter;
  Arguments args;
  args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(1));
  args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(kGeomPath));
  args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(kFeatureIdsPath));
  args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(kPhasesPath));
  args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

  // The call must return because the no-progress guard stops the fill loop.
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData: real HDF5 cross-slice defect oracle", "[SimplnxCore][FillBadDataFilter][.OocStoreContract]")
{
  unit_test::LoadPlugins();
  REQUIRE(Application::Instance()->getIOManager("HDF5-OOC") != nullptr);
  const bool newPhase = GENERATE(false, true);
  const PreferencesSentinel preferencesSentinel(DataStorageMode::ForceOutOfCore, 1);
  DataStructure dataStructure;
  BuildFillBadDataTestData(dataStructure, 5, 5, 5, 5);
  const DataPath featureIdsPath({"DataContainer", "CellData", "FeatureIds"});
  const DataPath phasesPath({"DataContainer", "CellData", "Phases"});
  const DataPath payloadPath({"DataContainer", "CellData", "Payload"});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32ArrayType>(featureIdsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32ArrayType>(phasesPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(payloadPath.getParent()));
  auto& featureIds = dataStructure.getDataRefAs<Int32ArrayType>(featureIdsPath);
  auto& phases = dataStructure.getDataRefAs<Int32ArrayType>(phasesPath);
  auto& cells = dataStructure.getDataRefAs<AttributeMatrix>(payloadPath.getParent());
  auto payloadStore = data_store_utilities::CreateDataStore<float32>(dataStructure, payloadPath, cells.getShape(), {2});
  auto* payloadPtr = Float32ArrayType::Create(dataStructure, "Payload", payloadStore, cells.getId());
  REQUIRE(payloadPtr != nullptr);
  featureIds.fill(1);
  phases.fill(1);
  for(usize voxelIdx = 0; voxelIdx < 125; voxelIdx++)
  {
    (*payloadPtr)[voxelIdx * 2] = 7.0F;
    (*payloadPtr)[voxelIdx * 2 + 1] = -2.0F;
  }
  // The three-cell component spans Z slices and meets the retention threshold.
  // The isolated diagonal cell has only one voxel and must copy a good tuple.
  for(usize voxelIdx : {37ULL, 62ULL, 87ULL, 56ULL})
  {
    featureIds[voxelIdx] = 0;
    phases[voxelIdx] = 0;
    (*payloadPtr)[voxelIdx * 2] = -5.0F;
    (*payloadPtr)[voxelIdx * 2 + 1] = -6.0F;
  }
  REQUIRE(featureIds.getDataStoreRef().getDataFormat() == "HDF5-OOC");
  REQUIRE(phases.getDataStoreRef().getDataFormat() == "HDF5-OOC");
  REQUIRE(payloadStore->getDataFormat() == "HDF5-OOC");
  FillBadDataFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(3));
  args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(newPhase));
  args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsPath));
  args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(phasesPath));
  args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"DataContainer"})));
  const auto before = GetAlgorithmPathExecutionCounts();
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const auto after = GetAlgorithmPathExecutionCounts();
  REQUIRE(after.out_of_core_on_out_of_core_store == before.out_of_core_on_out_of_core_store + 1);
  REQUIRE(after.in_core == before.in_core);
  for(usize voxelIdx = 0; voxelIdx < 125; voxelIdx++)
  {
    const bool retainedDefect = voxelIdx == 37 || voxelIdx == 62 || voxelIdx == 87;
    INFO("voxel " << voxelIdx);
    REQUIRE(featureIds[voxelIdx] == (retainedDefect ? 0 : 1));
    REQUIRE(phases[voxelIdx] == (retainedDefect ? (newPhase ? 2 : 0) : 1));
    REQUIRE((*payloadPtr)[voxelIdx * 2] == (retainedDefect ? -5.0F : 7.0F));
    REQUIRE((*payloadPtr)[voxelIdx * 2 + 1] == (retainedDefect ? -6.0F : -2.0F));
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadDataFilter: SIMPL Backwards Compatibility", "[SimplnxCore][FillBadDataFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "FillBadDataFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "FillBadDataFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterListPtr);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilterPtr = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilterPtr != nullptr);

      const IFilter* filterPtr = pipelineFilterPtr->getFilter();
      REQUIRE(filterPtr != nullptr);
      REQUIRE(filterPtr->uuid() == FilterTraits<FillBadDataFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
      CHECK(args.value<int32>(FillBadDataFilter::k_MinAllowedDefectSize_Key) == 5);
      CHECK(args.value<bool>(FillBadDataFilter::k_StoreAsNewPhase_Key) == true);
      CHECK(args.value<DataPath>(FillBadDataFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(FillBadDataFilter::k_SelectedCellDataGroup_Key) == DataPath({"DataContainer", "CellData"}));
      CHECK(args.value<DataPath>(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(FillBadDataFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      // Successful pipeline loading verifies the MultiDataArraySelectionFilterParameterConverter value.
    }
  }
}

TEST_CASE("SimplnxCore::FillBadData: 200x200x200 Correctness", "[Core][FillBadDataFilter]")
{
  unit_test::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  // int32 1-comp => 200*200*4 = 160,000 bytes/slice

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, k_ArchiveName, k_DataDirName);
  DataStructure exemplarDS = unit_test::LoadDataStructure(k_ExemplarFile);

  std::string testName = GENERATE("NoNewPhase", "NewPhase");
  DYNAMIC_SECTION("Variant: " << testName)
  {
    const bool storeAsNewPhase = (testName == "NewPhase");

    DataStructure dataStructure;
    BuildFillBadDataTestData(dataStructure, k_Dim, k_Dim, k_Dim, k_BlockSize, true);

    scope.requireExpectedStore(dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"DataContainer", "CellData", "FeatureIds"})));

    FillBadDataFilter filter;
    Arguments args;
    args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(k_MinDefectSize));
    args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(storeAsNewPhase));
    args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "FeatureIds"})));
    args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "Phases"})));
    args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
    args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"DataContainer"})));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    // Compare against exemplar
    const std::string exemplarGeomName = "DataContainer_" + testName + "_Exemplar";
    const DataPath exemplarFeatureIdsPath({exemplarGeomName, "CellData", "FeatureIds"});
    const DataPath exemplarPhasesPath({exemplarGeomName, "CellData", "Phases"});

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"DataContainer", "CellData", "FeatureIds"})));
    REQUIRE_NOTHROW(exemplarDS.getDataRefAs<Int32ArrayType>(exemplarFeatureIdsPath));
    CompareDataArrays<int32>(exemplarDS.getDataRefAs<Int32ArrayType>(exemplarFeatureIdsPath), dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"DataContainer", "CellData", "FeatureIds"})));

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"DataContainer", "CellData", "Phases"})));
    REQUIRE_NOTHROW(exemplarDS.getDataRefAs<Int32ArrayType>(exemplarPhasesPath));
    CompareDataArrays<int32>(exemplarDS.getDataRefAs<Int32ArrayType>(exemplarPhasesPath), dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"DataContainer", "CellData", "Phases"})));

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::FillBadData: 200x200x200 Ignored Arrays", "[Core][FillBadDataFilter]")
{
  unit_test::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  // int32 1-comp => 200*200*4 = 160,000 bytes/slice

  constexpr int32 k_Sentinel = -999;

  DataStructure dataStructure;
  BuildFillBadDataTestData(dataStructure, k_Dim, k_Dim, k_Dim, k_BlockSize, false);

  scope.requireExpectedStore(dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"DataContainer", "CellData", "FeatureIds"})));

  // Add an extra "IgnoredArray" filled with a sentinel value
  auto& cellAM = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"DataContainer", "CellData"}));
  const DataPath ignoredArrayPath = DataPath({"DataContainer", "CellData", "IgnoredArray"});
  auto ignoredDataStore = data_store_utilities::CreateDataStore<int32>(dataStructure, ignoredArrayPath, cellAM.getShape(), {1});
  auto* ignoredArrayPtr = DataArray<int32>::Create(dataStructure, "IgnoredArray", ignoredDataStore, cellAM.getId());
  auto& ignoredStore = ignoredArrayPtr->getDataStoreRef();
  ignoredStore.fill(k_Sentinel);

  // Record bad-voxel count before fill using bulk reads
  const auto& featureIdsBefore = dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"DataContainer", "CellData", "FeatureIds"}));
  const auto& fidsStore = featureIdsBefore.getDataStoreRef();
  const usize totalTuples = fidsStore.getNumberOfTuples();
  usize badCount = 0;
  {
    constexpr usize k_Chunk = 40000;
    auto buf = std::make_unique<int32[]>(k_Chunk);
    for(usize off = 0; off < totalTuples; off += k_Chunk)
    {
      const usize count = std::min(k_Chunk, totalTuples - off);
      auto fidsStoreReadResult = fidsStore.copyIntoBuffer(off, nonstd::span<int32>(buf.get(), count));
      SIMPLNX_RESULT_REQUIRE_VALID(fidsStoreReadResult);
      for(usize i = 0; i < count; i++)
      {
        if(buf[i] == 0)
        {
          badCount++;
        }
      }
    }
  }
  REQUIRE(badCount > 0);

  {
    FillBadDataFilter filter;
    Arguments args;
    args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(50));
    args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));
    args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "FeatureIds"})));
    args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "Phases"})));
    args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"DataContainer"})));

    // Include the IgnoredArray in the ignored paths
    MultiArraySelectionParameter::ValueType ignoredPaths = {DataPath({"DataContainer", "CellData", "IgnoredArray"})};
    args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(ignoredPaths));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Verify: FeatureIds has no zeros (all scattered bad voxels filled) — bulk read
  const auto& featureIdsAfter = dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"DataContainer", "CellData", "FeatureIds"}));
  {
    const auto& fidsAfterStore = featureIdsAfter.getDataStoreRef();
    constexpr usize k_Chunk = 40000;
    auto buf = std::make_unique<int32[]>(k_Chunk);
    for(usize off = 0; off < totalTuples; off += k_Chunk)
    {
      const usize count = std::min(k_Chunk, totalTuples - off);
      auto fidsAfterStoreReadResult = fidsAfterStore.copyIntoBuffer(off, nonstd::span<int32>(buf.get(), count));
      SIMPLNX_RESULT_REQUIRE_VALID(fidsAfterStoreReadResult);
      for(usize i = 0; i < count; i++)
      {
        if(buf[i] == 0)
        {
          UNSCOPED_INFO(fmt::format("FeatureIds still zero at index {}", off + i));
          REQUIRE(false);
        }
      }
    }
  }

  // Verify: IgnoredArray is completely unchanged (sentinel at every voxel) — bulk read
  const auto& ignoredAfter = dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"DataContainer", "CellData", "IgnoredArray"}));
  {
    const auto& ignoredAfterStore = ignoredAfter.getDataStoreRef();
    constexpr usize k_Chunk = 40000;
    auto buf = std::make_unique<int32[]>(k_Chunk);
    for(usize off = 0; off < totalTuples; off += k_Chunk)
    {
      const usize count = std::min(k_Chunk, totalTuples - off);
      auto ignoredAfterStoreReadResult = ignoredAfterStore.copyIntoBuffer(off, nonstd::span<int32>(buf.get(), count));
      SIMPLNX_RESULT_REQUIRE_VALID(ignoredAfterStoreReadResult);
      for(usize i = 0; i < count; i++)
      {
        if(buf[i] != k_Sentinel)
        {
          UNSCOPED_INFO(fmt::format("IgnoredArray changed at index {}: expected {} got {}", off + i, k_Sentinel, buf[i]));
          REQUIRE(false);
        }
      }
    }
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadDataFilter: Strict Feature Ids", "[SimplnxCore][FillBadDataFilter][StrictFeatureIds]")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure;
  const DataPath geometryPath({"Strict Geometry"});
  const DataPath cellPath = geometryPath.createChildPath("Cell Data");
  const DataPath featureIdsPath = cellPath.createChildPath("Feature Ids");
  const DataPath otherPath = geometryPath.createChildPath("Other Data").createChildPath("Other Values");
  const ShapeType shape{1, 2, 2};
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, geometryPath.getTargetName());
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({2, 2, 1});
  auto* cellsPtr = AttributeMatrix::Create(dataStructure, "Cell Data", shape, imageGeomPtr->getId());
  REQUIRE(cellsPtr != nullptr);
  imageGeomPtr->setCellData(*cellsPtr);
  REQUIRE(AttributeMatrix::Create(dataStructure, "Feature Data", ShapeType{3}, imageGeomPtr->getId()) != nullptr);
  auto* otherPtr = AttributeMatrix::Create(dataStructure, "Other Data", shape, imageGeomPtr->getId());
  REQUIRE(otherPtr != nullptr);
  REQUIRE(Int32ArrayType::Create(dataStructure, "Feature Ids", std::make_shared<DataStore<int32>>(shape, ShapeType{1}, 1), cellsPtr->getId()) != nullptr);
  REQUIRE(Int32ArrayType::Create(dataStructure, "Other Values", std::make_shared<DataStore<int32>>(shape, ShapeType{1}, 1), otherPtr->getId()) != nullptr);

  FillBadDataFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(geometryPath));
  args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsPath));
  args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));

  int32 expectedCode = 0;
  SECTION("Correct Cell Attribute Matrix")
  {
    args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{featureIdsPath}));
  }
  SECTION("Feature Ids in another Attribute Matrix with the same tuple shape")
  {
    args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(otherPath));
    expectedCode = -16501;
  }
  SECTION("Ignored array in another Attribute Matrix")
  {
    args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{otherPath}));
    expectedCode = -16503;
  }
  SECTION("Modified Cell Phases in another Attribute Matrix")
  {
    args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(true));
    args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(otherPath));
    expectedCode = -16502;
  }
  SECTION("Unused Cell Phases in another Attribute Matrix")
  {
    args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(otherPath));
  }
  SECTION("Modified Cell Phases in the Cell Attribute Matrix")
  {
    const auto phasesPath = cellPath.createChildPath("Phases");
    REQUIRE(Int32ArrayType::Create(dataStructure, "Phases", std::make_shared<DataStore<int32>>(shape, ShapeType{1}, 1), cellsPtr->getId()) != nullptr);
    args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(true));
    args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(phasesPath));
  }
  const auto result = filter.preflight(dataStructure, args);
  if(expectedCode == 0)
  {
    REQUIRE(result.output_actions.valid());
  }
  else
  {
    REQUIRE(result.output_actions.invalid());
    REQUIRE(result.output_actions.errors().size() == 1);
    CHECK(result.output_actions.errors().front().code == expectedCode);
    CHECK(result.output_actions.errors().front().message.find(otherPath.toString()) != std::string::npos);
    CHECK(result.output_actions.errors().front().message.find(cellPath.toString()) != std::string::npos);
    CHECK(result.output_actions.errors().front().message.find(geometryPath.toString()) != std::string::npos);
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadDataFilter: Ignored Array Cell AM Alias", "[SimplnxCore][FillBadDataFilter][StrictFeatureIds]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  const DataPath geometryPath({"Alias Test Geometry"});
  const DataPath cellPath = geometryPath.createChildPath("Cells");
  const DataPath ignoredPath = cellPath.createChildPath("Ignored");
  const DataPath aliasCellPath({"Alternate Parent", "Cells"});
  const ShapeType shape{1, 1, 4};
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, geometryPath.getTargetName());
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({4, 1, 1});
  auto* cellsPtr = AttributeMatrix::Create(dataStructure, "Cells", shape, imageGeomPtr->getId());
  REQUIRE(cellsPtr != nullptr);
  imageGeomPtr->setCellData(*cellsPtr);
  auto* alternateParentPtr = DataGroup::Create(dataStructure, "Alternate Parent");
  REQUIRE(alternateParentPtr != nullptr);
  REQUIRE(dataStructure.setAdditionalParent(cellsPtr->getId(), alternateParentPtr->getId()));
  REQUIRE(imageGeomPtr->getCellDataPath() == cellPath);
  auto* idsPtr = Int32ArrayType::Create(dataStructure, "Ids", std::make_shared<DataStore<int32>>(shape, ShapeType{1}, 0), cellsPtr->getId());
  auto* ignoredPtr = Int32ArrayType::Create(dataStructure, "Ignored", std::make_shared<DataStore<int32>>(shape, ShapeType{1}, 0), cellsPtr->getId());
  REQUIRE(idsPtr != nullptr);
  REQUIRE(ignoredPtr != nullptr);
  const std::vector<int32> originalIds{1, 0, 0, 1};
  for(usize index = 0; index < 4; index++)
  {
    (*idsPtr)[index] = originalIds[index];
    (*ignoredPtr)[index] = static_cast<int32>(10 * (index + 1));
  }
  FillBadDataFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(geometryPath));
  args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(aliasCellPath.createChildPath("Ids")));
  args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{aliasCellPath.createChildPath("Ignored")}));
  args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(10));
  const auto preflight = filter.preflight(dataStructure, args);
  REQUIRE(preflight.output_actions.valid());
  for(const auto& modification : preflight.output_actions.value().modified_actions)
  {
    CHECK(modification.modified_path != ignoredPath);
  }
  const auto execute = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(execute.result);
  bool idsChanged = false;
  for(usize index = 0; index < 4; index++)
  {
    CHECK((*ignoredPtr)[index] == static_cast<int32>(10 * (index + 1)));
    idsChanged = idsChanged || (*idsPtr)[index] != originalIds[index];
  }
  CHECK(idsChanged);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData: Generate Test Data", "[Core][FillBadDataFilter][.GenerateTestData]")
{
  unit_test::LoadPlugins();

  const auto outputDir = fs::path(fmt::format("{}/generated_test_data/fill_bad_data", unit_test::k_BinaryTestOutputDir));
  fs::create_directories(outputDir);

  // 200^3 input data with large defect (full z=k_Dim/2 plane)
  {
    DataStructure dataStructure;
    BuildFillBadDataTestData(dataStructure, k_Dim, k_Dim, k_Dim, k_BlockSize, true);
    unit_test::WriteTestDataStructure(dataStructure, outputDir / "input.dream3d");
  }
}
