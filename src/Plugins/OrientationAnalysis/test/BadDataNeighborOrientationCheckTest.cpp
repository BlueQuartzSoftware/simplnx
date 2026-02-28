#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/BadDataNeighborOrientationCheckFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

#include <cmath>
#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;

namespace VerificationConstants
{
const std::string k_ImageName = "Image Geometry";
const DataPath k_ImagePath = DataPath({k_ImageName});

const DataPath k_CellDataPath = k_ImagePath.createChildPath(Constants::k_Cell_Data);
const DataPath k_CellEnsembleDataPath = k_ImagePath.createChildPath(Constants::k_Cell_Ensemble_Data);

// Cell Data
const std::string k_QuatsName = "Quats";
const DataPath k_QuatsArrayPath = k_CellDataPath.createChildPath(k_QuatsName);
const std::string k_MaskName = "Mask";
const DataPath k_MaskArrayPath = k_CellDataPath.createChildPath(k_MaskName);
const std::string k_PhasesName = "Phases";
const DataPath k_PhasesArrayPath = k_CellDataPath.createChildPath(k_PhasesName);

const std::string k_CStuctsName = "Crystal Structures";
const DataPath k_CStuctsArrayPath = k_CellEnsembleDataPath.createChildPath(k_CStuctsName);
} // namespace VerificationConstants

// Case 1.1.1: Base Case | 2 phase | Tolerance 5 | 1 Min Neighbors
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.1.1", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 140, true);

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "7_bad_data_neighbor_orientation_check.tar.gz", "bad_data_neighbor_orientation_check");

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check/case_1/case_1_1/case_1_1_1/case_1_1_1_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Bad Data Neighbor Orientation Check Filter
  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(1));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  /**
   * Expected Output:
   * Mask Array
   * 1 1 0 | 0 0 0 | 0 0 0 |
   * 0 0 0 | 0 0 0 | 0 0 0 |
   * 0 0 0 | 0 0 0 | 0 0 0 |
   */
  const std::array<uint8, 27> expectedMask = {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const UInt8AbstractDataStore& maskStore = dataStructure.getDataAs<UInt8Array>(VerificationConstants::k_MaskArrayPath)->getDataStoreRef();
  for(usize i = 0; i < maskStore.getSize(); ++i)
  {
    if(maskStore.getValue(i) != expectedMask[i])
    {
      const std::string errorMsg = fmt::format("Values diverged at index {}. Expected: {} | Actual: {}", i, expectedMask.at(i), maskStore.getValue(i));
      CAPTURE(errorMsg);
      REQUIRE(false);
    }
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check/case_1_1_1.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 1.1.2: Invalid Base Case | 3 phase | Tolerance 5 | 1 Min Neighbors
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.1.2", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 140, true);

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "7_bad_data_neighbor_orientation_check.tar.gz", "bad_data_neighbor_orientation_check");

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check/case_1/case_1_1/case_1_1_2/case_1_1_2_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Bad Data Neighbor Orientation Check Filter
  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(1));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  /**
   * Expected Output:
   * Mask Array
   * 0 1 0 | 0 0 0 | 0 0 0 |
   * 0 0 0 | 0 0 0 | 0 0 0 |
   * 0 0 0 | 0 0 0 | 0 0 0 |
   */
  const std::array<uint8, 27> expectedMask = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const UInt8AbstractDataStore& maskStore = dataStructure.getDataAs<UInt8Array>(VerificationConstants::k_MaskArrayPath)->getDataStoreRef();
  for(usize i = 0; i < maskStore.getSize(); ++i)
  {
    if(maskStore.getValue(i) != expectedMask[i])
    {
      const std::string errorMsg = fmt::format("Values diverged at index {}. Expected: {} | Actual: {}", i, expectedMask.at(i), maskStore.getValue(i));
      CAPTURE(errorMsg);
      REQUIRE(false);
    }
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check/case_1_1_2.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 1.1.3: Invalid Base Case | 2 phase | Tolerance 5 | 1 Min Neighbors
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.1.3", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 140, true);

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "7_bad_data_neighbor_orientation_check.tar.gz", "bad_data_neighbor_orientation_check");

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check/case_1/case_1_1/case_1_1_3/case_1_1_3_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Bad Data Neighbor Orientation Check Filter
  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(1));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  /**
   * Expected Output:
   * Mask Array
   * 0 1 0 | 0 0 0 | 0 0 0 |
   * 0 0 0 | 0 0 0 | 0 0 0 |
   * 0 0 0 | 0 0 0 | 0 0 0 |
   */
  const std::array<uint8, 27> expectedMask = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const UInt8AbstractDataStore& maskStore = dataStructure.getDataAs<UInt8Array>(VerificationConstants::k_MaskArrayPath)->getDataStoreRef();
  for(usize i = 0; i < maskStore.getSize(); ++i)
  {
    if(maskStore.getValue(i) != expectedMask[i])
    {
      const std::string errorMsg = fmt::format("Values diverged at index {}. Expected: {} | Actual: {}", i, expectedMask.at(i), maskStore.getValue(i));
      CAPTURE(errorMsg);
      REQUIRE(false);
    }
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check/case_1_1_3.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 1.2.1: Base Case | 2 phase | Tolerance 5 | 2 Min Neighbors
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.2.1", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 140, true);

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "7_bad_data_neighbor_orientation_check.tar.gz", "bad_data_neighbor_orientation_check");

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check/case_1/case_1_2/case_1_2_1/case_1_2_1_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Bad Data Neighbor Orientation Check Filter
  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(2));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  /**
   * Expected Output:
   * Mask Array
   * 1 1 0 | 0 0 0 | 0 0 0 |
   * 1 0 0 | 0 0 0 | 0 0 0 |
   * 0 0 0 | 0 0 0 | 0 0 0 |
   */
  const std::array<uint8, 27> expectedMask = {1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const UInt8AbstractDataStore& maskStore = dataStructure.getDataAs<UInt8Array>(VerificationConstants::k_MaskArrayPath)->getDataStoreRef();
  for(usize i = 0; i < maskStore.getSize(); ++i)
  {
    if(maskStore.getValue(i) != expectedMask[i])
    {
      const std::string errorMsg = fmt::format("Values diverged at index {}. Expected: {} | Actual: {}", i, expectedMask.at(i), maskStore.getValue(i));
      CAPTURE(errorMsg);
      REQUIRE(false);
    }
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check/case_1_2_1.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 1.2.2: Invalid Base Case | 2 phase | Tolerance 5 | 2 Min Neighbors
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.2.2", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 140, true);

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "7_bad_data_neighbor_orientation_check.tar.gz", "bad_data_neighbor_orientation_check");

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check/case_1/case_1_2/case_1_2_2/case_1_2_2_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Bad Data Neighbor Orientation Check Filter
  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(2));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  /**
   * Expected Output:
   * Mask Array
   * 0 1 0 | 0 0 0 | 0 0 0 |
   * 1 0 0 | 0 0 0 | 0 0 0 |
   * 0 0 0 | 0 0 0 | 0 0 0 |
   */
  const std::array<uint8, 27> expectedMask = {0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const UInt8AbstractDataStore& maskStore = dataStructure.getDataAs<UInt8Array>(VerificationConstants::k_MaskArrayPath)->getDataStoreRef();
  for(usize i = 0; i < maskStore.getSize(); ++i)
  {
    if(maskStore.getValue(i) != expectedMask[i])
    {
      const std::string errorMsg = fmt::format("Values diverged at index {}. Expected: {} | Actual: {}", i, expectedMask.at(i), maskStore.getValue(i));
      CAPTURE(errorMsg);
      REQUIRE(false);
    }
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check/case_1_2_2.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 1.2.3: Invalid Base Case | 2 phase | Tolerance 5 | 2 Min Neighbors
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.2.3", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 140, true);

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "7_bad_data_neighbor_orientation_check.tar.gz", "bad_data_neighbor_orientation_check");

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check/case_1/case_1_2/case_1_2_3/case_1_2_3_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Bad Data Neighbor Orientation Check Filter
  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(2));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  /**
   * Expected Output:
   * Mask Array
   * 0 1 0 | 0 0 0 | 0 0 0 |
   * 1 0 0 | 0 0 0 | 0 0 0 |
   * 0 0 0 | 0 0 0 | 0 0 0 |
   */
  const std::array<uint8, 27> expectedMask = {0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const UInt8AbstractDataStore& maskStore = dataStructure.getDataAs<UInt8Array>(VerificationConstants::k_MaskArrayPath)->getDataStoreRef();
  for(usize i = 0; i < maskStore.getSize(); ++i)
  {
    if(maskStore.getValue(i) != expectedMask[i])
    {
      const std::string errorMsg = fmt::format("Values diverged at index {}. Expected: {} | Actual: {}", i, expectedMask.at(i), maskStore.getValue(i));
      CAPTURE(errorMsg);
      REQUIRE(false);
    }
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check/case_1_2_3.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 1.3.1: Base Case | 1 phase | Tolerance 5 | 3 Min Neighbors
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.3.1", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 140, true);

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "7_bad_data_neighbor_orientation_check.tar.gz", "bad_data_neighbor_orientation_check");

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check/case_1/case_1_3/case_1_3_1/case_1_3_1_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Bad Data Neighbor Orientation Check Filter
  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(3));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  /**
   * Expected Output:
   * Mask Array
   * 0 0 0 | 0 1 0 | 0 0 0 |
   * 0 0 0 | 1 1 1 | 0 0 0 |
   * 0 0 0 | 0 0 0 | 0 0 0 |
   */
  const std::array<uint8, 27> expectedMask = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const UInt8AbstractDataStore& maskStore = dataStructure.getDataAs<UInt8Array>(VerificationConstants::k_MaskArrayPath)->getDataStoreRef();
  for(usize i = 0; i < maskStore.getSize(); ++i)
  {
    if(maskStore.getValue(i) != expectedMask[i])
    {
      const std::string errorMsg = fmt::format("Values diverged at index {}. Expected: {} | Actual: {}", i, expectedMask.at(i), maskStore.getValue(i));
      CAPTURE(errorMsg);
      REQUIRE(false);
    }
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check/case_1_3_1.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 1.3.2: Invalid Base Case | 2 phase | Tolerance 5 | 3 Min Neighbors
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.3.2", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 140, true);

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "7_bad_data_neighbor_orientation_check.tar.gz", "bad_data_neighbor_orientation_check");

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check/case_1/case_1_3/case_1_3_2/case_1_3_2_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Bad Data Neighbor Orientation Check Filter
  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(3));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  /**
   * Expected Output:
   * Mask Array
   * 0 0 0 | 0 1 0 | 0 0 0 |
   * 0 0 0 | 1 0 1 | 0 0 0 |
   * 0 0 0 | 0 0 0 | 0 0 0 |
   */
  const std::array<uint8, 27> expectedMask = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const UInt8AbstractDataStore& maskStore = dataStructure.getDataAs<UInt8Array>(VerificationConstants::k_MaskArrayPath)->getDataStoreRef();
  for(usize i = 0; i < maskStore.getSize(); ++i)
  {
    if(maskStore.getValue(i) != expectedMask[i])
    {
      const std::string errorMsg = fmt::format("Values diverged at index {}. Expected: {} | Actual: {}", i, expectedMask.at(i), maskStore.getValue(i));
      CAPTURE(errorMsg);
      REQUIRE(false);
    }
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check/case_1_3_2.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 1.3.3: Invalid Base Case | 1 phase | Tolerance 5 | 3 Min Neighbors
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.3.3", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 140, true);

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "7_bad_data_neighbor_orientation_check.tar.gz", "bad_data_neighbor_orientation_check");

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check/case_1/case_1_3/case_1_3_3/case_1_3_3_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Bad Data Neighbor Orientation Check Filter
  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(3));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  /**
   * Expected Output:
   * Mask Array
   * 0 0 0 | 0 1 0 | 0 0 0 |
   * 0 0 0 | 1 0 1 | 0 0 0 |
   * 0 0 0 | 0 0 0 | 0 0 0 |
   */
  const std::array<uint8, 27> expectedMask = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const UInt8AbstractDataStore& maskStore = dataStructure.getDataAs<UInt8Array>(VerificationConstants::k_MaskArrayPath)->getDataStoreRef();
  for(usize i = 0; i < maskStore.getSize(); ++i)
  {
    if(maskStore.getValue(i) != expectedMask[i])
    {
      const std::string errorMsg = fmt::format("Values diverged at index {}. Expected: {} | Actual: {}", i, expectedMask.at(i), maskStore.getValue(i));
      CAPTURE(errorMsg);
      REQUIRE(false);
    }
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check/case_1_3_3.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 1.4.1: Base Case | 1 phase | Tolerance 5 | 4 Min Neighbors
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.4.1", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 140, true);

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "7_bad_data_neighbor_orientation_check.tar.gz", "bad_data_neighbor_orientation_check");

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check/case_1/case_1_4/case_1_4_1/case_1_4_1_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Bad Data Neighbor Orientation Check Filter
  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(4));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  /**
   * Expected Output:
   * Mask Array
   * 0 0 0 | 0 1 0 | 0 0 0 |
   * 0 0 0 | 1 1 1 | 0 0 0 |
   * 0 0 0 | 0 1 0 | 0 0 0 |
   */
  const std::array<uint8, 27> expectedMask = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const UInt8AbstractDataStore& maskStore = dataStructure.getDataAs<UInt8Array>(VerificationConstants::k_MaskArrayPath)->getDataStoreRef();
  for(usize i = 0; i < maskStore.getSize(); ++i)
  {
    if(maskStore.getValue(i) != expectedMask[i])
    {
      const std::string errorMsg = fmt::format("Values diverged at index {}. Expected: {} | Actual: {}", i, expectedMask.at(i), maskStore.getValue(i));
      CAPTURE(errorMsg);
      REQUIRE(false);
    }
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check/case_1_4_1.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 1.4.2: Invalid Base Case | 2 phase | Tolerance 5 | 4 Min Neighbors
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.4.2", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 140, true);

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "7_bad_data_neighbor_orientation_check.tar.gz", "bad_data_neighbor_orientation_check");

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check/case_1/case_1_4/case_1_4_2/case_1_4_2_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Bad Data Neighbor Orientation Check Filter
  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(4));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  /**
   * Expected Output:
   * Mask Array
   * 0 0 0 | 0 1 0 | 0 0 0 |
   * 0 0 0 | 1 0 1 | 0 0 0 |
   * 0 0 0 | 0 1 0 | 0 0 0 |
   */
  const std::array<uint8, 27> expectedMask = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const UInt8AbstractDataStore& maskStore = dataStructure.getDataAs<UInt8Array>(VerificationConstants::k_MaskArrayPath)->getDataStoreRef();
  for(usize i = 0; i < maskStore.getSize(); ++i)
  {
    if(maskStore.getValue(i) != expectedMask[i])
    {
      const std::string errorMsg = fmt::format("Values diverged at index {}. Expected: {} | Actual: {}", i, expectedMask.at(i), maskStore.getValue(i));
      CAPTURE(errorMsg);
      REQUIRE(false);
    }
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check/case_1_4_2.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 1.4.3: Invalid Base Case | 1 phase | Tolerance 5 | 4 Min Neighbors
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.4.3", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 140, true);

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "7_bad_data_neighbor_orientation_check.tar.gz", "bad_data_neighbor_orientation_check");

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check/case_1/case_1_4/case_1_4_3/case_1_4_3_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Bad Data Neighbor Orientation Check Filter
  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(4));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  /**
   * Expected Output:
   * Mask Array
   * 0 0 0 | 0 1 0 | 0 0 0 |
   * 0 0 0 | 1 0 1 | 0 0 0 |
   * 0 0 0 | 0 1 0 | 0 0 0 |
   */
  const std::array<uint8, 27> expectedMask = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const UInt8AbstractDataStore& maskStore = dataStructure.getDataAs<UInt8Array>(VerificationConstants::k_MaskArrayPath)->getDataStoreRef();
  for(usize i = 0; i < maskStore.getSize(); ++i)
  {
    if(maskStore.getValue(i) != expectedMask[i])
    {
      const std::string errorMsg = fmt::format("Values diverged at index {}. Expected: {} | Actual: {}", i, expectedMask.at(i), maskStore.getValue(i));
      CAPTURE(errorMsg);
      REQUIRE(false);
    }
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check/case_1_4_3.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 1.5.1: Base Case | 1 phase | Tolerance 5 | 5 Min Neighbors
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.5.1", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 140, true);

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "7_bad_data_neighbor_orientation_check.tar.gz", "bad_data_neighbor_orientation_check");

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check/case_1/case_1_5/case_1_5_1/case_1_5_1_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Bad Data Neighbor Orientation Check Filter
  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(5));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  /**
   * Expected Output:
   * Mask Array
   * 0 0 0 | 0 1 0 | 0 0 0 |
   * 0 0 0 | 1 1 1 | 0 1 0 |
   * 0 0 0 | 0 1 0 | 0 0 0 |
   */
  const std::array<uint8, 27> expectedMask = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0};

  const UInt8AbstractDataStore& maskStore = dataStructure.getDataAs<UInt8Array>(VerificationConstants::k_MaskArrayPath)->getDataStoreRef();
  for(usize i = 0; i < maskStore.getSize(); ++i)
  {
    if(maskStore.getValue(i) != expectedMask[i])
    {
      const std::string errorMsg = fmt::format("Values diverged at index {}. Expected: {} | Actual: {}", i, expectedMask.at(i), maskStore.getValue(i));
      CAPTURE(errorMsg);
      REQUIRE(false);
    }
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check/case_1_5_1.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 1.5.2: Invalid Base Case | 2 phase | Tolerance 5 | 5 Min Neighbors
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.5.2", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 140, true);

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "7_bad_data_neighbor_orientation_check.tar.gz", "bad_data_neighbor_orientation_check");

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check/case_1/case_1_5/case_1_5_2/case_1_5_2_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Bad Data Neighbor Orientation Check Filter
  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(5));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  /**
   * Expected Output:
   * Mask Array
   * 0 0 0 | 0 1 0 | 0 0 0 |
   * 0 0 0 | 1 0 1 | 0 1 0 |
   * 0 0 0 | 0 1 0 | 0 0 0 |
   */
  const std::array<uint8, 27> expectedMask = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0};

  const UInt8AbstractDataStore& maskStore = dataStructure.getDataAs<UInt8Array>(VerificationConstants::k_MaskArrayPath)->getDataStoreRef();
  for(usize i = 0; i < maskStore.getSize(); ++i)
  {
    if(maskStore.getValue(i) != expectedMask[i])
    {
      const std::string errorMsg = fmt::format("Values diverged at index {}. Expected: {} | Actual: {}", i, expectedMask.at(i), maskStore.getValue(i));
      CAPTURE(errorMsg);
      REQUIRE(false);
    }
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check/case_1_5_2.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 1.5.3: Invalid Base Case | 1 phase | Tolerance 5 | 5 Min Neighbors
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.5.3", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 140, true);

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "7_bad_data_neighbor_orientation_check.tar.gz", "bad_data_neighbor_orientation_check");

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check/case_1/case_1_5/case_1_5_3/case_1_5_3_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Bad Data Neighbor Orientation Check Filter
  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(5));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  /**
   * Expected Output:
   * Mask Array
   * 0 0 0 | 0 1 0 | 0 0 0 |
   * 0 0 0 | 1 0 1 | 0 1 0 |
   * 0 0 0 | 0 1 0 | 0 0 0 |
   */
  const std::array<uint8, 27> expectedMask = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0};

  const UInt8AbstractDataStore& maskStore = dataStructure.getDataAs<UInt8Array>(VerificationConstants::k_MaskArrayPath)->getDataStoreRef();
  for(usize i = 0; i < maskStore.getSize(); ++i)
  {
    if(maskStore.getValue(i) != expectedMask[i])
    {
      const std::string errorMsg = fmt::format("Values diverged at index {}. Expected: {} | Actual: {}", i, expectedMask.at(i), maskStore.getValue(i));
      CAPTURE(errorMsg);
      REQUIRE(false);
    }
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check/case_1_5_3.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 1.6.1: Base Case | 1 phase | Tolerance 5 | 6 Min Neighbors
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.6.1", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 140, true);

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "7_bad_data_neighbor_orientation_check.tar.gz", "bad_data_neighbor_orientation_check");

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check/case_1/case_1_6/case_1_6_1/case_1_6_1_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Bad Data Neighbor Orientation Check Filter
  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(6));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  /**
   * Expected Output:
   * Mask Array
   * 0 0 0 | 0 1 0 | 0 0 0 |
   * 0 1 0 | 1 1 1 | 0 1 0 |
   * 0 0 0 | 0 1 0 | 0 0 0 |
   */
  const std::array<uint8, 27> expectedMask = {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0};

  const UInt8AbstractDataStore& maskStore = dataStructure.getDataAs<UInt8Array>(VerificationConstants::k_MaskArrayPath)->getDataStoreRef();
  for(usize i = 0; i < maskStore.getSize(); ++i)
  {
    if(maskStore.getValue(i) != expectedMask[i])
    {
      const std::string errorMsg = fmt::format("Values diverged at index {}. Expected: {} | Actual: {}", i, expectedMask.at(i), maskStore.getValue(i));
      CAPTURE(errorMsg);
      REQUIRE(false);
    }
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check/case_1_6_1.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 1.6.2: Invalid Base Case | 2 phase | Tolerance 5 | 6 Min Neighbors
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.6.2", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 140, true);

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "7_bad_data_neighbor_orientation_check.tar.gz", "bad_data_neighbor_orientation_check");

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check/case_1/case_1_6/case_1_6_2/case_1_6_2_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Bad Data Neighbor Orientation Check Filter
  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(6));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  /**
   * Expected Output:
   * Mask Array
   * 0 0 0 | 0 1 0 | 0 0 0 |
   * 0 1 0 | 1 0 1 | 0 1 0 |
   * 0 0 0 | 0 1 0 | 0 0 0 |
   */
  const std::array<uint8, 27> expectedMask = {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0};

  const UInt8AbstractDataStore& maskStore = dataStructure.getDataAs<UInt8Array>(VerificationConstants::k_MaskArrayPath)->getDataStoreRef();
  for(usize i = 0; i < maskStore.getSize(); ++i)
  {
    if(maskStore.getValue(i) != expectedMask[i])
    {
      const std::string errorMsg = fmt::format("Values diverged at index {}. Expected: {} | Actual: {}", i, expectedMask.at(i), maskStore.getValue(i));
      CAPTURE(errorMsg);
      REQUIRE(false);
    }
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check/case_1_6_2.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 1.6.3: Invalid Base Case | 1 phase | Tolerance 5 | 6 Min Neighbors
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.6.3", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 140, true);

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "7_bad_data_neighbor_orientation_check.tar.gz", "bad_data_neighbor_orientation_check");

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check/case_1/case_1_6/case_1_6_3/case_1_6_3_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Bad Data Neighbor Orientation Check Filter
  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(6));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  /**
   * Expected Output:
   * Mask Array
   * 0 0 0 | 0 1 0 | 0 0 0 |
   * 0 1 0 | 1 0 1 | 0 1 0 |
   * 0 0 0 | 0 1 0 | 0 0 0 |
   */
  const std::array<uint8, 27> expectedMask = {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0};

  const UInt8AbstractDataStore& maskStore = dataStructure.getDataAs<UInt8Array>(VerificationConstants::k_MaskArrayPath)->getDataStoreRef();
  for(usize i = 0; i < maskStore.getSize(); ++i)
  {
    if(maskStore.getValue(i) != expectedMask[i])
    {
      const std::string errorMsg = fmt::format("Values diverged at index {}. Expected: {} | Actual: {}", i, expectedMask.at(i), maskStore.getValue(i));
      CAPTURE(errorMsg);
      REQUIRE(false);
    }
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check/case_1_6_3.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 2.1: X+ Dim Case (Sequential) | Valid | 1 phase | Tolerance 5 | 5 Min Neighbors
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 2.1", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 400, true);

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "7_bad_data_neighbor_orientation_check.tar.gz", "bad_data_neighbor_orientation_check");

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check/case_2/case_2_1/case_2_1_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Bad Data Neighbor Orientation Check Filter
  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(5));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  /**
   * Expected Output:
   * Mask Array
   * All `1`
   */
  const UInt8AbstractDataStore& maskStore = dataStructure.getDataAs<UInt8Array>(VerificationConstants::k_MaskArrayPath)->getDataStoreRef();
  for(usize i = 0; i < maskStore.getSize(); ++i)
  {
    if(maskStore.getValue(i) != 1)
    {
      const std::string errorMsg = fmt::format("Values diverged at index {}. Expected: {} | Actual: {}", i, 1, maskStore.getValue(i));
      CAPTURE(errorMsg);
      REQUIRE(false);
    }
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check/case_2_1.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 2.2: Y+ Dim Case (Sequential) | Valid | 1 phase | Tolerance 5 | 5 Min Neighbors
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 2.2", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 400, true);

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "7_bad_data_neighbor_orientation_check.tar.gz", "bad_data_neighbor_orientation_check");

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check/case_2/case_2_2/case_2_2_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Bad Data Neighbor Orientation Check Filter
  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(5));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  /**
   * Expected Output:
   * Mask Array
   * All `1`
   */
  const UInt8AbstractDataStore& maskStore = dataStructure.getDataAs<UInt8Array>(VerificationConstants::k_MaskArrayPath)->getDataStoreRef();
  for(usize i = 0; i < maskStore.getSize(); ++i)
  {
    if(maskStore.getValue(i) != 1)
    {
      const std::string errorMsg = fmt::format("Values diverged at index {}. Expected: {} | Actual: {}", i, 1, maskStore.getValue(i));
      CAPTURE(errorMsg);
      REQUIRE(false);
    }
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check/case_2_2.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 2.3: Z+ Dim Case (Sequential) | Valid | 1 phase | Tolerance 5 | 5 Min Neighbors
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 2.3", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 400, true);

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "7_bad_data_neighbor_orientation_check.tar.gz", "bad_data_neighbor_orientation_check");

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check/case_2/case_2_3/case_2_3_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Bad Data Neighbor Orientation Check Filter
  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(5));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  /**
   * Expected Output:
   * Mask Array
   * All `1`
   */
  const UInt8AbstractDataStore& maskStore = dataStructure.getDataAs<UInt8Array>(VerificationConstants::k_MaskArrayPath)->getDataStoreRef();
  for(usize i = 0; i < maskStore.getSize(); ++i)
  {
    if(maskStore.getValue(i) != 1)
    {
      const std::string errorMsg = fmt::format("Values diverged at index {}. Expected: {} | Actual: {}", i, 1, maskStore.getValue(i));
      CAPTURE(errorMsg);
      REQUIRE(false);
    }
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check/case_2_3.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 2.4: X- Dim Case (Recursive) | Valid | 1 phase | Tolerance 5 | 5 Min Neighbors
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 2.4", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 400, true);

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "7_bad_data_neighbor_orientation_check.tar.gz", "bad_data_neighbor_orientation_check");

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check/case_2/case_2_4/case_2_4_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Bad Data Neighbor Orientation Check Filter
  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(5));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  /**
   * Expected Output:
   * Mask Array
   * All `1`
   */
  const UInt8AbstractDataStore& maskStore = dataStructure.getDataAs<UInt8Array>(VerificationConstants::k_MaskArrayPath)->getDataStoreRef();
  for(usize i = 0; i < maskStore.getSize(); ++i)
  {
    if(maskStore.getValue(i) != 1)
    {
      const std::string errorMsg = fmt::format("Values diverged at index {}. Expected: {} | Actual: {}", i, 1, maskStore.getValue(i));
      CAPTURE(errorMsg);
      REQUIRE(false);
    }
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check/case_2_4.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 2.5: Y- Dim Case (Recursive) | Valid | 1 phase | Tolerance 5 | 5 Min Neighbors
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 2.5", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 400, true);

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "7_bad_data_neighbor_orientation_check.tar.gz", "bad_data_neighbor_orientation_check");

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check/case_2/case_2_5/case_2_5_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Bad Data Neighbor Orientation Check Filter
  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(5));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  /**
   * Expected Output:
   * Mask Array
   * All `1`
   */
  const UInt8AbstractDataStore& maskStore = dataStructure.getDataAs<UInt8Array>(VerificationConstants::k_MaskArrayPath)->getDataStoreRef();
  for(usize i = 0; i < maskStore.getSize(); ++i)
  {
    if(maskStore.getValue(i) != 1)
    {
      const std::string errorMsg = fmt::format("Values diverged at index {}. Expected: {} | Actual: {}", i, 1, maskStore.getValue(i));
      CAPTURE(errorMsg);
      REQUIRE(false);
    }
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check/case_2_5.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 2.6: Z- Dim Case (Recursive) | Valid | 1 phase | Tolerance 5 | 5 Min Neighbors
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 2.6", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 400, true);

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "7_bad_data_neighbor_orientation_check.tar.gz", "bad_data_neighbor_orientation_check");

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check/case_2/case_2_6/case_2_6_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Bad Data Neighbor Orientation Check Filter
  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(5));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  /**
   * Expected Output:
   * Mask Array
   * All `1`
   */
  const UInt8AbstractDataStore& maskStore = dataStructure.getDataAs<UInt8Array>(VerificationConstants::k_MaskArrayPath)->getDataStoreRef();
  for(usize i = 0; i < maskStore.getSize(); ++i)
  {
    if(maskStore.getValue(i) != 1)
    {
      const std::string errorMsg = fmt::format("Values diverged at index {}. Expected: {} | Actual: {}", i, 1, maskStore.getValue(i));
      CAPTURE(errorMsg);
      REQUIRE(false);
    }
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check/case_2_6.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 3.1: Long Sequential | Valid | 1 phase | Tolerance 5 | 1 Min Neighbors
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 3.1", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 400, true);

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "7_bad_data_neighbor_orientation_check.tar.gz", "bad_data_neighbor_orientation_check");

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check/case_3/case_3_1/case_3_1_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Bad Data Neighbor Orientation Check Filter
  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(1));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  /**
   * Expected Output:
   * Mask Array
   * All `1`
   */
  const UInt8AbstractDataStore& maskStore = dataStructure.getDataAs<UInt8Array>(VerificationConstants::k_MaskArrayPath)->getDataStoreRef();
  for(usize i = 0; i < maskStore.getSize(); ++i)
  {
    if(maskStore.getValue(i) != 1)
    {
      const std::string errorMsg = fmt::format("Values diverged at index {}. Expected: {} | Actual: {}", i, 1, maskStore.getValue(i));
      CAPTURE(errorMsg);
      REQUIRE(false);
    }
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check/case_3_1.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 3.2: Long Recursive | Valid | 1 phase | Tolerance 5 | 1 Min Neighbors
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 3.2", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 400, true);

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "7_bad_data_neighbor_orientation_check.tar.gz", "bad_data_neighbor_orientation_check");

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check/case_3/case_3_2/case_3_2_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Bad Data Neighbor Orientation Check Filter
  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(1));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  /**
   * Expected Output:
   * Mask Array
   * All `1`
   */
  const UInt8AbstractDataStore& maskStore = dataStructure.getDataAs<UInt8Array>(VerificationConstants::k_MaskArrayPath)->getDataStoreRef();
  for(usize i = 0; i < maskStore.getSize(); ++i)
  {
    if(maskStore.getValue(i) != 1)
    {
      const std::string errorMsg = fmt::format("Values diverged at index {}. Expected: {} | Actual: {}", i, 1, maskStore.getValue(i));
      CAPTURE(errorMsg);
      REQUIRE(false);
    }
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check/case_3_2.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 4: Semi-Complex Synthetic Structure | Valid | 3 phase | Tolerance 5 | 4 Min Neighbors
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 4", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 400, true);

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "7_bad_data_neighbor_orientation_check.tar.gz", "bad_data_neighbor_orientation_check");

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check/case_4/case_4_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Bad Data Neighbor Orientation Check Filter
  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(4));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // clang-format off
  /**
   * Expected Output:
   * Mask Array
   *
   * 0 0 0 0 0 | 1 1 1 1 1 | 0 0 0 0 1 | 0 0 0 1 1 | 0 1 0 1 1 |
   * 1 1 1 0 0 | 1 1 1 1 1 | 1 0 1 0 0 | 0 0 1 1 1 | 1 1 1 1 1 |
   * 0 1 0 0 0 | 1 1 1 1 1 | 0 0 1 1 1 | 0 1 1 1 1 | 0 1 1 1 0 |
   * 0 0 1 1 0 | 1 1 1 1 1 | 0 0 1 1 0 | 0 0 1 0 1 | 1 1 1 0 1 |
   * 0 0 0 0 0 | 1 1 1 1 1 | 0 0 1 0 0 | 0 0 0 0 0 | 0 1 0 1 0 |
   */
  const std::array<uint8, 125> expectedMask = {
    0, 0, 0, 0, 0,
    1, 1, 1, 0, 0,
    0, 1, 0, 0, 0,
    0, 0, 1, 1, 0,
    0, 0, 0, 0, 0,

    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1,

    0, 0, 0, 0, 1,
    1, 0, 1, 0, 0,
    0, 0, 1, 1, 1,
    0, 0, 1, 1, 0,
    0, 0, 1, 0, 0,

    0, 0, 0, 1, 1,
    0, 0, 1, 1, 1,
    0, 1, 1, 1, 1,
    0, 0, 1, 0, 1,
    0, 0, 0, 0, 0,

    0, 1, 0, 1, 1,
    1, 1, 1, 1, 1,
    0, 1, 1, 1, 0,
    1, 1, 1, 0, 1,
    0, 1, 0, 1, 0
  };
  // clang-format on

  const UInt8AbstractDataStore& maskStore = dataStructure.getDataAs<UInt8Array>(VerificationConstants::k_MaskArrayPath)->getDataStoreRef();
  for(usize i = 0; i < maskStore.getSize(); ++i)
  {
    if(maskStore.getValue(i) != expectedMask[i])
    {
      const std::string errorMsg = fmt::format("Values diverged at index {}. Expected: {} | Actual: {}", i, expectedMask[i], maskStore.getValue(i));
      CAPTURE(errorMsg);
      REQUIRE(false);
    }
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check/case_4.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Benchmark: 200x200x200 programmatic dataset (no exemplar — correctness proven by the 27 tests above)
// Writes data to a .dream3d file and reloads it so arrays use disk-backed ZarrStore in OOC mode.
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Benchmark 200x200x200", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter][Benchmark]")
{
  UnitTest::LoadPlugins();
  // 1 Z-slice of quats: 200*200*4*4 = 640000 bytes → ~200 chunks in OOC
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 10000, true);

  constexpr usize kDimX = 200;
  constexpr usize kDimY = 200;
  constexpr usize kDimZ = 200;
  constexpr usize kTotalVoxels = kDimX * kDimY * kDimZ;

  const auto benchmarkFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_benchmark.dream3d", unit_test::k_BinaryTestOutputDir));

  // ===== Stage 1: Build the dataset programmatically and write to .dream3d =====
  {
    DataStructure buildDS;

    ImageGeom* imageGeom = ImageGeom::Create(buildDS, VerificationConstants::k_ImageName);
    imageGeom->setDimensions({kDimX, kDimY, kDimZ});
    imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
    imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

    AttributeMatrix* cellAM = AttributeMatrix::Create(buildDS, Constants::k_Cell_Data, {kDimZ, kDimY, kDimX}, imageGeom->getId());
    imageGeom->setCellData(*cellAM);

    AttributeMatrix* ensembleAM = AttributeMatrix::Create(buildDS, Constants::k_Cell_Ensemble_Data, {2}, imageGeom->getId());

    // Crystal Structures: [999 (Unknown), 1 (Cubic_High)]
    auto* crystalStructures = UnitTest::CreateTestDataArray<uint32>(buildDS, VerificationConstants::k_CStuctsName, {2}, {1}, ensembleAM->getId());
    auto& csStore = crystalStructures->getDataStoreRef();
    csStore.setValue(0, 999);
    csStore.setValue(1, 1);

    const ShapeType cellTupleShape = {kDimZ, kDimY, kDimX};

    // Phases: all phase 1
    auto* phases = UnitTest::CreateTestDataArray<int32>(buildDS, VerificationConstants::k_PhasesName, cellTupleShape, {1}, cellAM->getId());
    auto& phasesStore = phases->getDataStoreRef();
    for(usize i = 0; i < kTotalVoxels; i++)
    {
      phasesStore.setValue(i, 1);
    }

    // Mask: ~40% bad voxels (mask=0) deterministically scattered to stress Phase 2
    auto* mask = UnitTest::CreateTestDataArray<uint8>(buildDS, VerificationConstants::k_MaskName, cellTupleShape, {1}, cellAM->getId());
    auto& maskStore = mask->getDataStoreRef();
    for(usize i = 0; i < kTotalVoxels; i++)
    {
      maskStore.setValue(i, static_cast<uint8>(((i * 7 + 13) % 100) >= 40 ? 1 : 0));
    }

    // Quats: 4-component float32 with spatially-varying orientations.
    // 8 octants with distinct base quaternions (15 deg apart about Z axis).
    // Intra-octant perturbations are <5 deg. Cross-boundary neighbors differ by >5 deg,
    // creating varied neighbor counts (1-6) that exercise Phase 2 cascading.
    auto* quats = UnitTest::CreateTestDataArray<float32>(buildDS, VerificationConstants::k_QuatsName, cellTupleShape, {4}, cellAM->getId());
    auto& quatsStore = quats->getDataStoreRef();

    constexpr usize kHalfX = kDimX / 2;
    constexpr usize kHalfY = kDimY / 2;
    constexpr usize kHalfZ = kDimZ / 2;
    const float32 baseAngles[8] = {0.0f, 15.0f, 30.0f, 45.0f, 60.0f, 75.0f, 90.0f, 105.0f};

    for(usize i = 0; i < kTotalVoxels; i++)
    {
      const usize ix = i % kDimX;
      const usize iy = (i / kDimX) % kDimY;
      const usize iz = i / (kDimX * kDimY);
      const usize octant = (iz >= kHalfZ ? 4 : 0) + (iy >= kHalfY ? 2 : 0) + (ix >= kHalfX ? 1 : 0);

      const float32 perturbDeg = 0.5f * static_cast<float32>(i % 10);
      const float32 angleDeg = baseAngles[octant] + perturbDeg;
      const float32 halfAngle = angleDeg * 3.14159265f / 360.0f;

      const float32 w = std::cos(halfAngle);
      const float32 z = std::sin(halfAngle);
      quatsStore.setValue(i * 4 + 0, w);
      quatsStore.setValue(i * 4 + 1, 0.0f);
      quatsStore.setValue(i * 4 + 2, 0.0f);
      quatsStore.setValue(i * 4 + 3, z);
    }

    UnitTest::WriteTestDataStructure(buildDS, benchmarkFilePath);
  }

  // ===== Stage 2: Reload from .dream3d (arrays become ZarrStore in OOC mode) and run filter =====
  DataStructure dataStructure = UnitTest::LoadDataStructure(benchmarkFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(1));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Clean up the temporary benchmark file
  fs::remove(benchmarkFilePath);
}
