#include <catch2/catch.hpp>

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Pipeline/AbstractPipelineNode.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

#include "SimplnxCore/Filters/FillBadDataFilter.hpp"
#include "SimplnxCore/Filters/ReadDREAM3DFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

TEST_CASE("SimplnxCore::FillBadData_SmallIN100", "[Core][FillBadDataFilter]")
{
  // Load the Simplnx Application instance and load the plugins
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  // FillBadData SmallIN100: 117x201x189, EulerAngles (float32, 3-comp) => 201*189*3*4 = 455,868 bytes/slice
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 455868, true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_fill_bad_data.tar.gz", "6_5_fill_bad_data");
  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/6_5_exemplar.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/6_5_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    FillBadDataFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(1000));
    args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));
    args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsArrayPath));
    args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesArrayPath));

    args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
    args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));

    // Preflight the filter and check the result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args); //, nullptr, IFilter::MessageHandler{[](const IFilter::Message& message) { fmt::print("{}\n", message.message); }});
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  UnitTest::CompareExemplarToGeneratedData(dataStructure, exemplarDataStructure, k_CellAttributeMatrix, k_DataContainer);

  // Write the DataStructure out to the file system
  // #ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_fill_bad_data.dream3d", unit_test::k_BinaryTestOutputDir)));
  // #endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

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

TEST_CASE("SimplnxCore::FillBadData: Benchmark 200x200x200", "[Core][FillBadDataFilter][Benchmark]")
{
  UnitTest::LoadPlugins();
  // 200x200x200, FeatureIds int32 1-comp => 200*200*4 = 160,000 bytes/slice
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 160000, true);

  constexpr usize kDimX = 200;
  constexpr usize kDimY = 200;
  constexpr usize kDimZ = 200;
  constexpr usize kTotalVoxels = kDimX * kDimY * kDimZ;
  const ShapeType cellTupleShape = {kDimZ, kDimY, kDimX};
  const auto benchmarkFile = fs::path(fmt::format("{}/fill_bad_data_benchmark.dream3d", unit_test::k_BinaryTestOutputDir));

  // Stage 1: Build data programmatically and write to .dream3d
  {
    DataStructure buildDS;
    auto* imageGeom = ImageGeom::Create(buildDS, "DataContainer");
    imageGeom->setDimensions({kDimX, kDimY, kDimZ});
    imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
    imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

    auto* cellAM = AttributeMatrix::Create(buildDS, "CellData", cellTupleShape, imageGeom->getId());
    imageGeom->setCellData(*cellAM);

    // Create FeatureIds array (int32, 1-component) - grid of features with scattered bad voxels
    auto* featureIdsArray = UnitTest::CreateTestDataArray<int32>(buildDS, "FeatureIds", cellTupleShape, {1}, cellAM->getId());
    auto& featureIdsStore = featureIdsArray->getDataStoreRef();

    // Create Phases array (int32, 1-component) - all phase 1
    auto* phasesArray = UnitTest::CreateTestDataArray<int32>(buildDS, "Phases", cellTupleShape, {1}, cellAM->getId());
    auto& phasesStore = phasesArray->getDataStoreRef();

    // Fill: divide into 25-voxel blocks, each block = one feature (1-based).
    // Scatter ~10% bad voxels (FeatureId=0) using a deterministic pattern.
    constexpr usize kBlockSize = 25;
    constexpr usize kBlocksPerDim = kDimX / kBlockSize; // 8
    for(usize z = 0; z < kDimZ; z++)
    {
      for(usize y = 0; y < kDimY; y++)
      {
        for(usize x = 0; x < kDimX; x++)
        {
          const usize idx = z * kDimX * kDimY + y * kDimX + x;
          phasesStore[idx] = 1;

          usize bx = x / kBlockSize;
          usize by = y / kBlockSize;
          usize bz = z / kBlockSize;
          int32 blockFeatureId = static_cast<int32>(bz * kBlocksPerDim * kBlocksPerDim + by * kBlocksPerDim + bx + 1);

          // Scatter bad voxels: ~10% of voxels become bad (FeatureId=0)
          bool isBad = ((x * 7 + y * 13 + z * 29) % 10 == 0);
          featureIdsStore[idx] = isBad ? 0 : blockFeatureId;
        }
      }
    }

    UnitTest::WriteTestDataStructure(buildDS, benchmarkFile);
  }

  // Stage 2: Reload (arrays become ZarrStore in OOC) and run filter
  DataStructure dataStructure = UnitTest::LoadDataStructure(benchmarkFile);

  {
    FillBadDataFilter filter;
    Arguments args;

    args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(50));
    args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));
    args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "FeatureIds"})));
    args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "Phases"})));
    args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
    args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"DataContainer"})));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args, nullptr, IFilter::MessageHandler{[](const IFilter::Message& message) { fmt::print("{}\n", message.message); }});
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  fs::remove(benchmarkFile);
}
