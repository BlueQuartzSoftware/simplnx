#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/BadDataNeighborOrientationCheckFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>
#include <fstream>

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
  // EbsdLib 2.4.1 CubicOps precision fix (`2*atan2(|v|, w)` with explicit reduced-quaternion components)
  // shifts cubic misorientations on/near a sym op by ~0.02 deg; the center voxel (index 13) of this case
  // now falls inside the 5 deg tolerance and is flagged. The hard-coded expectedMask was updated 2026-05-29.
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
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check/case_1_3_3.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 1.4.1: Base Case | 1 phase | Tolerance 5 | 4 Min Neighbors
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.4.1", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
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
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check/case_1_4_3.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 1.5.1: Base Case | 1 phase | Tolerance 5 | 5 Min Neighbors
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.5.1", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
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
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check/case_1_5_3.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 1.6.1: Base Case | 1 phase | Tolerance 5 | 6 Min Neighbors
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.6.1", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
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
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check/case_1_6_3.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case 2.1: X+ Dim Case (Sequential) | Valid | 1 phase | Tolerance 5 | 5 Min Neighbors
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 2.1", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
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

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "BadDataNeighborOrientationCheckFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "BadDataNeighborOrientationCheckFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<BadDataNeighborOrientationCheckFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<float32>(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key) == 2.5f);
      CHECK(args.value<int32>(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key) == 5);
      CHECK(args.value<DataPath>(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
    }
  }
}
