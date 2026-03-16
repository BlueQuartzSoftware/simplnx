#include <catch2/catch.hpp>

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
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

  auto featureIdsDataStore = DataStoreUtilities::CreateDataStore<int32>(cellShape, {1}, IDataAction::Mode::Execute);
  auto* featureIdsArray = DataArray<int32>::Create(ds, "FeatureIds", featureIdsDataStore, cellAM->getId());
  auto& featureIdsStore = featureIdsArray->getDataStoreRef();

  auto phasesDataStore = DataStoreUtilities::CreateDataStore<int32>(cellShape, {1}, IDataAction::Mode::Execute);
  auto* phasesArray = DataArray<int32>::Create(ds, "Phases", phasesDataStore, cellAM->getId());
  auto& phasesStore = phasesArray->getDataStoreRef();

  const usize blocksPerDim = dimX / blockSize;
  for(usize z = 0; z < dimZ; z++)
  {
    for(usize y = 0; y < dimY; y++)
    {
      for(usize x = 0; x < dimX; x++)
      {
        const usize idx = z * dimX * dimY + y * dimX + x;
        phasesStore[idx] = 1;

        usize bx = x / blockSize;
        usize by = y / blockSize;
        usize bz = z / blockSize;
        int32 blockFeatureId = static_cast<int32>(bz * blocksPerDim * blocksPerDim + by * blocksPerDim + bx + 1);

        // Scatter bad voxels: ~10% of voxels become bad (FeatureId=0)
        bool isBad = ((x * 7 + y * 13 + z * 29) % 10 == 0);
        featureIdsStore[idx] = isBad ? 0 : blockFeatureId;
      }
    }
  }

  // Add a contiguous large defect: entire z=dimZ/2 plane set to FeatureId=0
  if(addLargeDefect)
  {
    const usize z = dimZ / 2;
    for(usize y = 0; y < dimY; y++)
    {
      for(usize x = 0; x < dimX; x++)
      {
        featureIdsStore[z * dimX * dimY + y * dimX + x] = 0;
      }
    }
  }
}
// Exemplar archive
const std::string k_ArchiveName = "fill_bad_data_exemplars.tar.gz";
const std::string k_DataDirName = "fill_bad_data_exemplars";
const fs::path k_DataDir = fs::path(unit_test::k_TestFilesDir.view()) / k_DataDirName;
const fs::path k_ExemplarFile = k_DataDir / "fill_bad_data.dream3d";

// Test dimensions for 200^3 tests
constexpr usize k_Dim = 200;
constexpr usize k_BlockSize = 25;
constexpr int32 k_MinDefectSize = 50;
} // namespace

TEST_CASE("SimplnxCore::FillBadData::Test01_SingleSmallDefect", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  // Configure out-of-core settings (automatically restored on scope exit)
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 100, true); // 100 bytes - force very small arrays to OOC

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_fill_bad_data.tar.gz", "6_5_fill_bad_data");

  // Read input data
  auto inputFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_01_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(inputFilePath);

  // Read the expected output
  auto expectedFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_01_expected.dream3d", unit_test::k_TestFilesDir));
  DataStructure expectedDataStructure = UnitTest::LoadDataStructure(expectedFilePath);

  // Run filter
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

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Compare results
  UnitTest::CompareExemplarToGeneratedData(dataStructure, expectedDataStructure, DataPath({"DataContainer", "CellData"}), "DataContainer");

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData::Test02_SingleLargeDefect", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);

  // Configure out-of-core settings (automatically restored on scope exit)
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 100, true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_fill_bad_data.tar.gz", "6_5_fill_bad_data");

  // Read input data
  auto inputFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_02_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(inputFilePath);

  // Read the expected output
  auto expectedFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_02_expected.dream3d", unit_test::k_TestFilesDir));
  DataStructure expectedDataStructure = UnitTest::LoadDataStructure(expectedFilePath);

  // Run filter
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

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Compare results
  UnitTest::CompareExemplarToGeneratedData(dataStructure, expectedDataStructure, DataPath({"DataContainer", "CellData"}), "DataContainer");

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData::Test03_ThresholdBoundary", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);

  // Configure out-of-core settings (automatically restored on scope exit)
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 100, true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_fill_bad_data.tar.gz", "6_5_fill_bad_data");

  auto inputFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_03_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(inputFilePath);

  auto expectedFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_03_expected.dream3d", unit_test::k_TestFilesDir));
  DataStructure expectedDataStructure = UnitTest::LoadDataStructure(expectedFilePath);

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

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  UnitTest::CompareExemplarToGeneratedData(dataStructure, expectedDataStructure, DataPath({"DataContainer", "CellData"}), "DataContainer");
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData::Test04_MultipleSmallDefects", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);

  // Configure out-of-core settings (automatically restored on scope exit)
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 500, true); // Slightly larger for 10x10x10

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_fill_bad_data.tar.gz", "6_5_fill_bad_data");

  auto inputFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_04_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(inputFilePath);

  auto expectedFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_04_expected.dream3d", unit_test::k_TestFilesDir));
  DataStructure expectedDataStructure = UnitTest::LoadDataStructure(expectedFilePath);

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

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  UnitTest::CompareExemplarToGeneratedData(dataStructure, expectedDataStructure, DataPath({"DataContainer", "CellData"}), "DataContainer");
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData::Test05_MixedSmallAndLarge", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);

  // Configure out-of-core settings (automatically restored on scope exit)
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 500, true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_fill_bad_data.tar.gz", "6_5_fill_bad_data");

  auto inputFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_05_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(inputFilePath);

  auto expectedFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_05_expected.dream3d", unit_test::k_TestFilesDir));
  DataStructure expectedDataStructure = UnitTest::LoadDataStructure(expectedFilePath);

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

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  UnitTest::CompareExemplarToGeneratedData(dataStructure, expectedDataStructure, DataPath({"DataContainer", "CellData"}), "DataContainer");
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData::Test06_SingleVoxelDefects", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);

  // Configure out-of-core settings (automatically restored on scope exit)
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 100, true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_fill_bad_data.tar.gz", "6_5_fill_bad_data");

  auto inputFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_06_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(inputFilePath);

  auto expectedFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_06_expected.dream3d", unit_test::k_TestFilesDir));
  DataStructure expectedDataStructure = UnitTest::LoadDataStructure(expectedFilePath);

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

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  UnitTest::CompareExemplarToGeneratedData(dataStructure, expectedDataStructure, DataPath({"DataContainer", "CellData"}), "DataContainer");
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData::Test07_DefectsAtBoundaries", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);

  // Configure out-of-core settings (automatically restored on scope exit)
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 100, true);

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

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  UnitTest::CompareExemplarToGeneratedData(dataStructure, expectedDataStructure, DataPath({"DataContainer", "CellData"}), "DataContainer");
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData::Test11_NeighborTieBreaking", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);

  // Configure out-of-core settings (automatically restored on scope exit)
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 50, true); // Very small for 3x3x3

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_fill_bad_data.tar.gz", "6_5_fill_bad_data");

  // Read input data
  auto inputFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_11_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(inputFilePath);

  // Read the expected output
  auto expectedFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_11_expected.dream3d", unit_test::k_TestFilesDir));
  DataStructure expectedDataStructure = UnitTest::LoadDataStructure(expectedFilePath);

  // Run filter
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

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Compare results
  UnitTest::CompareExemplarToGeneratedData(dataStructure, expectedDataStructure, DataPath({"DataContainer", "CellData"}), "DataContainer");

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData::Test13_StoreAsNewPhase", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);

  // Configure out-of-core settings (automatically restored on scope exit)
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 100, true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_fill_bad_data.tar.gz", "6_5_fill_bad_data");

  // Read input data
  auto inputFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_13_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(inputFilePath);

  // Read the expected output
  auto expectedFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/test_13_expected.dream3d", unit_test::k_TestFilesDir));
  DataStructure expectedDataStructure = UnitTest::LoadDataStructure(expectedFilePath);

  // Run filter
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

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Compare results
  UnitTest::CompareExemplarToGeneratedData(dataStructure, expectedDataStructure, DataPath({"DataContainer", "CellData"}), "DataContainer");

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData: 200x200x200 Correctness", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  // int32 1-comp => 200*200*4 = 160,000 bytes/slice
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 160000, true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, k_ArchiveName, k_DataDirName);
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);

  std::string testName = GENERATE("NoNewPhase", "NewPhase");
  DYNAMIC_SECTION("Variant: " << testName)
  {
    const bool storeAsNewPhase = (testName == "NewPhase");

    DataStructure dataStructure;
    BuildFillBadDataTestData(dataStructure, k_Dim, k_Dim, k_Dim, k_BlockSize, true);

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
    auto executeResult = filter.execute(dataStructure, args);
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
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  // int32 1-comp => 200*200*4 = 160,000 bytes/slice
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 160000, true);

  constexpr int32 k_Sentinel = -999;

  DataStructure dataStructure;
  BuildFillBadDataTestData(dataStructure, k_Dim, k_Dim, k_Dim, k_BlockSize, false);

  // Add an extra "IgnoredArray" filled with a sentinel value
  auto& cellAM = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"DataContainer", "CellData"}));
  auto ignoredDataStore = DataStoreUtilities::CreateDataStore<int32>(cellAM.getShape(), {1}, IDataAction::Mode::Execute);
  auto* ignoredArray = DataArray<int32>::Create(dataStructure, "IgnoredArray", ignoredDataStore, cellAM.getId());
  auto& ignoredStore = ignoredArray->getDataStoreRef();
  for(usize i = 0; i < ignoredStore.getNumberOfTuples(); i++)
  {
    ignoredStore[i] = k_Sentinel;
  }

  // Record which voxels are bad before fill
  const auto& featureIdsBefore = dataStructure.getDataRefAs<Int32Array>(DataPath({"DataContainer", "CellData", "FeatureIds"}));
  std::vector<bool> wasBad(featureIdsBefore.getNumberOfTuples(), false);
  usize badCount = 0;
  for(usize i = 0; i < featureIdsBefore.getNumberOfTuples(); i++)
  {
    if(featureIdsBefore.getDataStoreRef().getValue(i) == 0)
    {
      wasBad[i] = true;
      badCount++;
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
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Verify: FeatureIds has no zeros (all scattered bad voxels filled)
  const auto& featureIdsAfter = dataStructure.getDataRefAs<Int32Array>(DataPath({"DataContainer", "CellData", "FeatureIds"}));
  for(usize i = 0; i < featureIdsAfter.getNumberOfTuples(); i++)
  {
    REQUIRE(featureIdsAfter.getDataStoreRef().getValue(i) != 0);
  }

  // Verify: IgnoredArray is completely unchanged (sentinel at every voxel)
  const auto& ignoredAfter = dataStructure.getDataRefAs<Int32Array>(DataPath({"DataContainer", "CellData", "IgnoredArray"}));
  for(usize i = 0; i < ignoredAfter.getNumberOfTuples(); i++)
  {
    REQUIRE(ignoredAfter.getDataStoreRef().getValue(i) == k_Sentinel);
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
