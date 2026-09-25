#include <catch2/catch.hpp>
#include <nonstd/span.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Pipeline/AbstractPipelineNode.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include "SimplnxCore/Filters/FillBadDataFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
/**
 * @brief Builds a FillBadData test dataset with block-patterned FeatureIds
 * and ~10% scattered bad voxels (FeatureId=0) using a deterministic pattern.
 */
void BuildFillBadDataTestData(DataStructure& ds, usize dimX, usize dimY, usize dimZ, usize blockSize, bool addLargeDefect = false)
{
  const ShapeType cellShape = {dimZ, dimY, dimX};
  auto* imageGeom = ImageGeom::Create(ds, "DataContainer");
  imageGeom->setDimensions({dimX, dimY, dimZ});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

  auto* cellAM = AttributeMatrix::Create(ds, "CellData", cellShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  const DataPath featureIdsPath = DataPath({"DataContainer", "CellData", "FeatureIds"});
  auto featureIdsDataStore = DataStoreUtilities::CreateDataStore<int32>(ds, featureIdsPath, cellShape, {1});
  auto* featureIdsArray = DataArray<int32>::Create(ds, "FeatureIds", featureIdsDataStore, cellAM->getId());
  auto& featureIdsStore = featureIdsArray->getDataStoreRef();

  const DataPath phasesPath = DataPath({"DataContainer", "CellData", "Phases"});
  auto phasesDataStore = DataStoreUtilities::CreateDataStore<int32>(ds, phasesPath, cellShape, {1});
  auto* phasesArray = DataArray<int32>::Create(ds, "Phases", phasesDataStore, cellAM->getId());
  auto& phasesStore = phasesArray->getDataStoreRef();

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
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "6_5_fill_bad_data.tar.gz", "6_5_fill_bad_data");
  // Load the exemplar output.
  auto exemplarFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/6_5_exemplar.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Load the Small IN100 input.
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/6_5_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    // Configure the filter arguments.
    FillBadDataFilter filter;
    Arguments args;

    args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(1000));
    args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));
    args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(Constants::k_FeatureIdsArrayPath));
    args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(Constants::k_PhasesArrayPath));

    args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
    args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));

    // Preflight the filter and check the result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  UnitTest::CompareExemplarToGeneratedData(dataStructure, exemplarDataStructure, Constants::k_CellAttributeMatrix, Constants::k_DataContainer);

// The optional output supports manual inspection.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_fill_bad_data.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData::Test01_SingleSmallDefect", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  // Configure OOC settings for this scenario.

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_fill_bad_data.tar.gz", "6_5_fill_bad_data");

  // Load the input data.
  auto inputFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_01_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(inputFilePath);

  // Load the expected output.
  auto expectedFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_01_expected.dream3d", unit_test::k_TestFilesDir));
  DataStructure expectedDataStructure = UnitTest::LoadDataStructure(expectedFilePath);

  scope.requireExpectedStore(dataStructure.getDataRefAs<Int32Array>(DataPath({"DataContainer", "CellData", "FeatureIds"})));

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
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Compare the generated results.
  UnitTest::CompareExemplarToGeneratedData(dataStructure, expectedDataStructure, DataPath({"DataContainer", "CellData"}), "DataContainer");

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData::Test02_SingleLargeDefect", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  // Configure OOC settings for this scenario.

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_fill_bad_data.tar.gz", "6_5_fill_bad_data");

  // Load the input data.
  auto inputFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_02_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(inputFilePath);

  // Load the expected output.
  auto expectedFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_02_expected.dream3d", unit_test::k_TestFilesDir));
  DataStructure expectedDataStructure = UnitTest::LoadDataStructure(expectedFilePath);

  scope.requireExpectedStore(dataStructure.getDataRefAs<Int32Array>(DataPath({"DataContainer", "CellData", "FeatureIds"})));

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
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Compare the generated results.
  UnitTest::CompareExemplarToGeneratedData(dataStructure, expectedDataStructure, DataPath({"DataContainer", "CellData"}), "DataContainer");

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData::Test03_ThresholdBoundary", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  // Configure OOC settings for this scenario.

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_fill_bad_data.tar.gz", "6_5_fill_bad_data");

  auto inputFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_03_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(inputFilePath);

  auto expectedFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_03_expected.dream3d", unit_test::k_TestFilesDir));
  DataStructure expectedDataStructure = UnitTest::LoadDataStructure(expectedFilePath);

  scope.requireExpectedStore(dataStructure.getDataRefAs<Int32Array>(DataPath({"DataContainer", "CellData", "FeatureIds"})));

  FillBadDataFilter filter;
  Arguments args;
  args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(25));
  args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "FeatureIds"})));
  args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "Phases"})));
  args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"DataContainer"})));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  UnitTest::CompareExemplarToGeneratedData(dataStructure, expectedDataStructure, DataPath({"DataContainer", "CellData"}), "DataContainer");
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData::Test04_MultipleSmallDefects", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  // Configure OOC settings for this scenario.

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_fill_bad_data.tar.gz", "6_5_fill_bad_data");

  auto inputFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_04_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(inputFilePath);

  auto expectedFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_04_expected.dream3d", unit_test::k_TestFilesDir));
  DataStructure expectedDataStructure = UnitTest::LoadDataStructure(expectedFilePath);

  scope.requireExpectedStore(dataStructure.getDataRefAs<Int32Array>(DataPath({"DataContainer", "CellData", "FeatureIds"})));

  FillBadDataFilter filter;
  Arguments args;
  args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(50));
  args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "FeatureIds"})));
  args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "Phases"})));
  args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"DataContainer"})));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  UnitTest::CompareExemplarToGeneratedData(dataStructure, expectedDataStructure, DataPath({"DataContainer", "CellData"}), "DataContainer");
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData::Test05_MixedSmallAndLarge", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  // Configure OOC settings for this scenario.

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_fill_bad_data.tar.gz", "6_5_fill_bad_data");

  auto inputFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_05_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(inputFilePath);

  auto expectedFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_05_expected.dream3d", unit_test::k_TestFilesDir));
  DataStructure expectedDataStructure = UnitTest::LoadDataStructure(expectedFilePath);

  scope.requireExpectedStore(dataStructure.getDataRefAs<Int32Array>(DataPath({"DataContainer", "CellData", "FeatureIds"})));

  FillBadDataFilter filter;
  Arguments args;
  args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(50));
  args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "FeatureIds"})));
  args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "Phases"})));
  args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"DataContainer"})));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  UnitTest::CompareExemplarToGeneratedData(dataStructure, expectedDataStructure, DataPath({"DataContainer", "CellData"}), "DataContainer");
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData::Test06_SingleVoxelDefects", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  // Configure OOC settings for this scenario.

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_fill_bad_data.tar.gz", "6_5_fill_bad_data");

  auto inputFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_06_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(inputFilePath);

  auto expectedFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_06_expected.dream3d", unit_test::k_TestFilesDir));
  DataStructure expectedDataStructure = UnitTest::LoadDataStructure(expectedFilePath);

  scope.requireExpectedStore(dataStructure.getDataRefAs<Int32Array>(DataPath({"DataContainer", "CellData", "FeatureIds"})));

  FillBadDataFilter filter;
  Arguments args;
  args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(10));
  args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "FeatureIds"})));
  args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "Phases"})));
  args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"DataContainer"})));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  UnitTest::CompareExemplarToGeneratedData(dataStructure, expectedDataStructure, DataPath({"DataContainer", "CellData"}), "DataContainer");
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData::Test07_DefectsAtBoundaries", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  // Configure OOC settings for this scenario.

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_fill_bad_data.tar.gz", "6_5_fill_bad_data");

  auto inputFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_07_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(inputFilePath);

  auto expectedFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_07_expected.dream3d", unit_test::k_TestFilesDir));
  DataStructure expectedDataStructure = UnitTest::LoadDataStructure(expectedFilePath);

  FillBadDataFilter filter;
  Arguments args;
  args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(20));
  args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "FeatureIds"})));
  args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "Phases"})));
  args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"DataContainer"})));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  UnitTest::CompareExemplarToGeneratedData(dataStructure, expectedDataStructure, DataPath({"DataContainer", "CellData"}), "DataContainer");
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData::Test11_NeighborTieBreaking", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  // Configure OOC settings for this scenario.

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_fill_bad_data.tar.gz", "6_5_fill_bad_data");

  // Load the input data.
  auto inputFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_11_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(inputFilePath);

  // Load the expected output.
  auto expectedFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_11_expected.dream3d", unit_test::k_TestFilesDir));
  DataStructure expectedDataStructure = UnitTest::LoadDataStructure(expectedFilePath);

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
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Compare the generated results.
  UnitTest::CompareExemplarToGeneratedData(dataStructure, expectedDataStructure, DataPath({"DataContainer", "CellData"}), "DataContainer");

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData::Test13_StoreAsNewPhase", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  // Configure OOC settings for this scenario.

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_fill_bad_data.tar.gz", "6_5_fill_bad_data");

  // Load the input data.
  auto inputFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_13_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(inputFilePath);

  // Load the expected output.
  auto expectedFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_13_expected.dream3d", unit_test::k_TestFilesDir));
  DataStructure expectedDataStructure = UnitTest::LoadDataStructure(expectedFilePath);

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
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Compare the generated results.
  UnitTest::CompareExemplarToGeneratedData(dataStructure, expectedDataStructure, DataPath({"DataContainer", "CellData"}), "DataContainer");

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadDataFilter:: Invalid Preflight Min Defect Size", "[Core][FillBadDataFilter]")
{
  DataStructure dataStructure;
  const DataPath k_GeomPath({"DataContainer"});
  const DataPath k_FeatureIdsPath({"DataContainer", "CellData", "FeatureIds"});

  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "DataContainer");
  imageGeomPtr->setDimensions({1, 1, 1});
  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, 1, 1}, imageGeomPtr->getId());
  UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", {1, 1, 1}, {1}, cellDataPtr->getId());

  FillBadDataFilter filter;
  Arguments args;
  args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(0));
  args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
  args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -16500);
}

// A volume with no good neighbor cannot be filled.
// The no-progress guard must return and leave those voxels unchanged.
TEST_CASE("SimplnxCore::FillBadData::AllBadData_TerminatesWithoutHang", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure;
  const DataPath k_GeomPath({"DataContainer"});
  const DataPath k_CellDataPath = k_GeomPath.createChildPath("CellData");
  const DataPath k_FeatureIdsPath = k_CellDataPath.createChildPath("FeatureIds");
  const DataPath k_PhasesPath = k_CellDataPath.createChildPath("Phases");

  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "DataContainer");
  imageGeomPtr->setDimensions({3, 3, 1});
  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, 3, 3}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellDataPtr);
  auto* featureIds = UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", {1, 3, 3}, {1}, cellDataPtr->getId());
  auto* phases = UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", {1, 3, 3}, {1}, cellDataPtr->getId());
  // Every voxel is bad data (featureId 0 -> marked for fill); there is no good neighbor to fill from.
  featureIds->fill(0);
  phases->fill(1);

  FillBadDataFilter filter;
  Arguments args;
  args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(1));
  args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
  args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesPath));
  args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // The call must return because the no-progress guard stops the fill loop.
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData: real HDF5 cross-slice defect oracle", "[SimplnxCore][FillBadDataFilter][.OocStoreContract]")
{
  UnitTest::LoadPlugins();
  REQUIRE(Application::Instance()->getIOManager("HDF5-OOC") != nullptr);
  const bool newPhase = GENERATE(false, true);
  const PreferencesSentinel preferencesSentinel(DataStorageMode::ForceOutOfCore, 1);
  DataStructure dataStructure;
  BuildFillBadDataTestData(dataStructure, 5, 5, 5, 5);
  const DataPath featureIdsPath({"DataContainer", "CellData", "FeatureIds"});
  const DataPath phasesPath({"DataContainer", "CellData", "Phases"});
  const DataPath payloadPath({"DataContainer", "CellData", "Payload"});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(featureIdsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(phasesPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(payloadPath.getParent()));
  auto& featureIds = dataStructure.getDataRefAs<Int32Array>(featureIdsPath);
  auto& phases = dataStructure.getDataRefAs<Int32Array>(phasesPath);
  auto& cells = dataStructure.getDataRefAs<AttributeMatrix>(payloadPath.getParent());
  auto payloadStore = DataStoreUtilities::CreateDataStore<float32>(dataStructure, payloadPath, cells.getShape(), {2});
  auto* payloadPtr = Float32Array::Create(dataStructure, "Payload", payloadStore, cells.getId());
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
  REQUIRE(after.OutOfCoreOnOutOfCoreStore == before.OutOfCoreOnOutOfCoreStore + 1);
  REQUIRE(after.InCore == before.InCore);
  for(usize voxelIdx = 0; voxelIdx < 125; voxelIdx++)
  {
    const bool retainedDefect = voxelIdx == 37 || voxelIdx == 62 || voxelIdx == 87;
    INFO("voxel " << voxelIdx);
    REQUIRE(featureIds[voxelIdx] == (retainedDefect ? 0 : 1));
    REQUIRE(phases[voxelIdx] == (retainedDefect ? (newPhase ? 2 : 0) : 1));
    REQUIRE((*payloadPtr)[voxelIdx * 2] == (retainedDefect ? -5.0F : 7.0F));
    REQUIRE((*payloadPtr)[voxelIdx * 2 + 1] == (retainedDefect ? -6.0F : -2.0F));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadDataFilter: SIMPL Backwards Compatibility", "[SimplnxCore][FillBadDataFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "FillBadDataFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "FillBadDataFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<FillBadDataFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
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
  UnitTest::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  // int32 1-comp => 200*200*4 = 160,000 bytes/slice

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, k_ArchiveName, k_DataDirName);
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);

  std::string testName = GENERATE("NoNewPhase", "NewPhase");
  DYNAMIC_SECTION("Variant: " << testName)
  {
    const bool storeAsNewPhase = (testName == "NewPhase");

    DataStructure dataStructure;
    BuildFillBadDataTestData(dataStructure, k_Dim, k_Dim, k_Dim, k_BlockSize, true);

    scope.requireExpectedStore(dataStructure.getDataRefAs<Int32Array>(DataPath({"DataContainer", "CellData", "FeatureIds"})));

    FillBadDataFilter filter;
    Arguments args;
    args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(k_MinDefectSize));
    args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(storeAsNewPhase));
    args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "FeatureIds"})));
    args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "Phases"})));
    args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
    args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"DataContainer"})));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    // Compare against exemplar
    const std::string exemplarGeomName = "DataContainer_" + testName + "_Exemplar";
    const DataPath exemplarFeatureIdsPath({exemplarGeomName, "CellData", "FeatureIds"});
    const DataPath exemplarPhasesPath({exemplarGeomName, "CellData", "Phases"});

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"DataContainer", "CellData", "FeatureIds"})));
    REQUIRE_NOTHROW(exemplarDS.getDataRefAs<Int32Array>(exemplarFeatureIdsPath));
    CompareDataArrays<int32>(exemplarDS.getDataRefAs<Int32Array>(exemplarFeatureIdsPath), dataStructure.getDataRefAs<Int32Array>(DataPath({"DataContainer", "CellData", "FeatureIds"})));

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"DataContainer", "CellData", "Phases"})));
    REQUIRE_NOTHROW(exemplarDS.getDataRefAs<Int32Array>(exemplarPhasesPath));
    CompareDataArrays<int32>(exemplarDS.getDataRefAs<Int32Array>(exemplarPhasesPath), dataStructure.getDataRefAs<Int32Array>(DataPath({"DataContainer", "CellData", "Phases"})));

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::FillBadData: 200x200x200 Ignored Arrays", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  // int32 1-comp => 200*200*4 = 160,000 bytes/slice

  constexpr int32 k_Sentinel = -999;

  DataStructure dataStructure;
  BuildFillBadDataTestData(dataStructure, k_Dim, k_Dim, k_Dim, k_BlockSize, false);

  scope.requireExpectedStore(dataStructure.getDataRefAs<Int32Array>(DataPath({"DataContainer", "CellData", "FeatureIds"})));

  // Add an extra "IgnoredArray" filled with a sentinel value
  auto& cellAM = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"DataContainer", "CellData"}));
  const DataPath ignoredArrayPath = DataPath({"DataContainer", "CellData", "IgnoredArray"});
  auto ignoredDataStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, ignoredArrayPath, cellAM.getShape(), {1});
  auto* ignoredArray = DataArray<int32>::Create(dataStructure, "IgnoredArray", ignoredDataStore, cellAM.getId());
  auto& ignoredStore = ignoredArray->getDataStoreRef();
  ignoredStore.fill(k_Sentinel);

  // Record bad-voxel count before fill using bulk reads
  const auto& featureIdsBefore = dataStructure.getDataRefAs<Int32Array>(DataPath({"DataContainer", "CellData", "FeatureIds"}));
  const auto& fidsStore = featureIdsBefore.getDataStoreRef();
  const usize totalTuples = fidsStore.getNumberOfTuples();
  usize badCount = 0;
  {
    constexpr usize kChunk = 40000;
    auto buf = std::make_unique<int32[]>(kChunk);
    for(usize off = 0; off < totalTuples; off += kChunk)
    {
      const usize count = std::min(kChunk, totalTuples - off);
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
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Verify: FeatureIds has no zeros (all scattered bad voxels filled) — bulk read
  const auto& featureIdsAfter = dataStructure.getDataRefAs<Int32Array>(DataPath({"DataContainer", "CellData", "FeatureIds"}));
  {
    const auto& fidsAfterStore = featureIdsAfter.getDataStoreRef();
    constexpr usize kChunk = 40000;
    auto buf = std::make_unique<int32[]>(kChunk);
    for(usize off = 0; off < totalTuples; off += kChunk)
    {
      const usize count = std::min(kChunk, totalTuples - off);
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
  const auto& ignoredAfter = dataStructure.getDataRefAs<Int32Array>(DataPath({"DataContainer", "CellData", "IgnoredArray"}));
  {
    const auto& ignoredAfterStore = ignoredAfter.getDataStoreRef();
    constexpr usize kChunk = 40000;
    auto buf = std::make_unique<int32[]>(kChunk);
    for(usize off = 0; off < totalTuples; off += kChunk)
    {
      const usize count = std::min(kChunk, totalTuples - off);
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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData: Generate Test Data", "[Core][FillBadDataFilter][.GenerateTestData]")
{
  UnitTest::LoadPlugins();

  const auto outputDir = fs::path(fmt::format("{}/generated_test_data/fill_bad_data", unit_test::k_BinaryTestOutputDir));
  fs::create_directories(outputDir);

  // 200^3 input data with large defect (full z=k_Dim/2 plane)
  {
    DataStructure ds;
    BuildFillBadDataTestData(ds, k_Dim, k_Dim, k_Dim, k_BlockSize, true);
    UnitTest::WriteTestDataStructure(ds, outputDir / "input.dream3d");
  }
}
