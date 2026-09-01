#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/BadDataNeighborOrientationCheckFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
namespace VerificationConstants
{
const std::string k_ImageName = "Image Geometry";
const DataPath k_ImagePath = DataPath({k_ImageName});

const DataPath k_CellDataPath = k_ImagePath.createChildPath(Constants::k_Cell_Data);
const DataPath k_CellEnsembleDataPath = k_ImagePath.createChildPath(Constants::k_Cell_Ensemble_Data);

const std::string k_QuatsName = "Quats";
const DataPath k_QuatsArrayPath = k_CellDataPath.createChildPath(k_QuatsName);
const std::string k_MaskName = "Mask";
const DataPath k_MaskArrayPath = k_CellDataPath.createChildPath(k_MaskName);
const std::string k_PhasesName = "Phases";
const DataPath k_PhasesArrayPath = k_CellDataPath.createChildPath(k_PhasesName);

const std::string k_CStuctsName = "Crystal Structures";
const DataPath k_CStuctsArrayPath = k_CellEnsembleDataPath.createChildPath(k_CStuctsName);
} // namespace VerificationConstants

namespace ClassFourInvariants
{
// Capture the mask before invariant checks.
std::vector<uint8> CaptureMask(const DataStructure& dataStructure)
{
  const auto& maskArray = dataStructure.getDataRefAs<UInt8Array>(VerificationConstants::k_MaskArrayPath);
  const auto& store = maskArray.getDataStoreRef();
  std::vector<uint8> snapshot(store.getSize());
  for(usize i = 0; i < store.getSize(); ++i)
  {
    snapshot[i] = store.getValue(i);
  }
  return snapshot;
}

// The filter can change false mask values to true. It cannot degrade a true
// value or reduce the true-value count.
void AssertInvariants(const std::vector<uint8>& originalMask, const DataStructure& dataStructure)
{
  const auto& maskArray = dataStructure.getDataRefAs<UInt8Array>(VerificationConstants::k_MaskArrayPath);
  const auto& store = maskArray.getDataStoreRef();
  REQUIRE(originalMask.size() == store.getSize());

  usize countBefore = 0;
  usize countAfter = 0;
  for(usize i = 0; i < store.getSize(); ++i)
  {
    if(originalMask[i] == 1)
    {
      ++countBefore;
      REQUIRE(store.getValue(i) == 1);
    }
    if(store.getValue(i) == 1)
    {
      ++countAfter;
    }
  }
  REQUIRE(countAfter >= countBefore);
}
} // namespace ClassFourInvariants
} // namespace

// AlgorithmTestScope forces each selected path and records its target-call
// witness.
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.1.1", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_1/case_1_1_1/case_1_1_1_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(1));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

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
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_1_1.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.1.2", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_1/case_1_1_2/case_1_1_2_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(1));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

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
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_1_2.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.1.3", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_1/case_1_1_3/case_1_1_3_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(1));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

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
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_1_3.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.2.1", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_2/case_1_2_1/case_1_2_1_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(2));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

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
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_2_1.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Same-phase gating must not reuse a prior neighbor angle. The mixed-phase
// neighbor must not flip voxel zero. See
// vv/deviations/BadDataNeighborOrientationCheckFilter.md D2.
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.2.2", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_2/case_1_2_2/case_1_2_2_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(2));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

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
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_2_2.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.2.3", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_2/case_1_2_3/case_1_2_3_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(2));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

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
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_2_3.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.3.1", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_3/case_1_3_1/case_1_3_1_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(3));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

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
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_3_1.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.3.2", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_3/case_1_3_2/case_1_3_2_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(3));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

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
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_3_2.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.3.3", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_3/case_1_3_3/case_1_3_3_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(3));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

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
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_3_3.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.4.1", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_4/case_1_4_1/case_1_4_1_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(4));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

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
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_4_1.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.4.2", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_4/case_1_4_2/case_1_4_2_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(4));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

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
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_4_2.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.4.3", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_4/case_1_4_3/case_1_4_3_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(4));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

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
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_4_3.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.5.1", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_5/case_1_5_1/case_1_5_1_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(5));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

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
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_5_1.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.5.2", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_5/case_1_5_2/case_1_5_2_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(5));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

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
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_5_2.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.5.3", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_5/case_1_5_3/case_1_5_3_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(5));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

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
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_5_3.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.6.1", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_6/case_1_6_1/case_1_6_1_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(6));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

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
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_6_1.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.6.2", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_6/case_1_6_2/case_1_6_2_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(6));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

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
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_6_2.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.6.3", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_6/case_1_6_3/case_1_6_3_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(6));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

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
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_6_3.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 2.1", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_2/case_2_1/case_2_1_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(5));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

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
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_2_1.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 2.2", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_2/case_2_2/case_2_2_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(5));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

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
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_2_2.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 2.3", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_2/case_2_3/case_2_3_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(5));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

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
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_2_3.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 2.4", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_2/case_2_4/case_2_4_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(5));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

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
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_2_4.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 2.5", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_2/case_2_5/case_2_5_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(5));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

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
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_2_5.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 2.6", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_2/case_2_6/case_2_6_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(5));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

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
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_2_6.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 3.1", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_3/case_3_1/case_3_1_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(1));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

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
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_3_1.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 3.2", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_3/case_3_2/case_3_2_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(1));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

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
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_3_2.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 4", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_4/case_4_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(4));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // clang-format off
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
  WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_4.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Preflight Error - Cell array tuple count mismatch (-6809)",
          "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter][preflight]")
{
  UnitTest::LoadPlugins();

  // Mask, CellPhases, and Quats have different tuple counts and must report
  // -6809.
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "Image Geometry");
  imageGeom->setDimensions({10, 1, 1});

  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", {10}, imageGeom->getId());
  UnitTest::CreateTestDataArray<float32>(dataStructure, "Quats", {10}, {4}, cellAM->getId());
  UnitTest::CreateTestDataArray<uint8>(dataStructure, "Mask", {10}, {1}, cellAM->getId());

  // The separate group gives CellPhases nine tuples instead of ten.
  auto* mismatchAM = AttributeMatrix::Create(dataStructure, "MismatchData", {9}, imageGeom->getId());
  UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", {9}, {1}, mismatchAM->getId());

  auto* ensembleAM = AttributeMatrix::Create(dataStructure, "CellEnsembleData", {2}, imageGeom->getId());
  UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", {2}, {1}, ensembleAM->getId());

  BadDataNeighborOrientationCheckFilter filter;
  Arguments args;
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(1));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry", "CellData", "Quats"})));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry", "CellData", "Mask"})));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry", "MismatchData", "Phases"})));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry", "CellEnsembleData", "CrystalStructures"})));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -6809);
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

// Class 4 checks monotonicity and no-degrade across base fixtures. See
// vv/BadDataNeighborOrientationCheckFilter.md.
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Class 4 Invariants Sweep", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  /**
   * @struct Fixture
   * @brief Identifies one invariant fixture and its neighbor requirement.
   */
  struct Fixture
  {
    std::string relPath;
    int32 numberOfNeighbors;
  };

  // Case 1 covers all neighbor counts and phase configurations.
  const std::vector<Fixture> fixtures = {
      {"case_1/case_1_1/case_1_1_1/case_1_1_1_input.dream3d", 1}, {"case_1/case_1_1/case_1_1_2/case_1_1_2_input.dream3d", 1}, {"case_1/case_1_1/case_1_1_3/case_1_1_3_input.dream3d", 1},
      {"case_1/case_1_2/case_1_2_1/case_1_2_1_input.dream3d", 2}, {"case_1/case_1_2/case_1_2_2/case_1_2_2_input.dream3d", 2}, {"case_1/case_1_2/case_1_2_3/case_1_2_3_input.dream3d", 2},
      {"case_1/case_1_3/case_1_3_1/case_1_3_1_input.dream3d", 3}, {"case_1/case_1_3/case_1_3_2/case_1_3_2_input.dream3d", 3}, {"case_1/case_1_3/case_1_3_3/case_1_3_3_input.dream3d", 3},
      {"case_1/case_1_4/case_1_4_1/case_1_4_1_input.dream3d", 4}, {"case_1/case_1_4/case_1_4_2/case_1_4_2_input.dream3d", 4}, {"case_1/case_1_4/case_1_4_3/case_1_4_3_input.dream3d", 4},
      {"case_1/case_1_5/case_1_5_1/case_1_5_1_input.dream3d", 5}, {"case_1/case_1_5/case_1_5_2/case_1_5_2_input.dream3d", 5}, {"case_1/case_1_5/case_1_5_3/case_1_5_3_input.dream3d", 5},
      {"case_1/case_1_6/case_1_6_1/case_1_6_1_input.dream3d", 6}, {"case_1/case_1_6/case_1_6_2/case_1_6_2_input.dream3d", 6}, {"case_1/case_1_6/case_1_6_3/case_1_6_3_input.dream3d", 6},
  };

  for(const auto& fixture : fixtures)
  {
    DYNAMIC_SECTION("Fixture: " << fixture.relPath << " NumberOfNeighbors=" << fixture.numberOfNeighbors)
    {
      auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/{}", unit_test::k_TestFilesDir, fixture.relPath));
      DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

      const auto originalMask = ClassFourInvariants::CaptureMask(dataStructure);

      BadDataNeighborOrientationCheckFilter filter;
      Arguments args;
      args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
      args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(fixture.numberOfNeighbors));
      args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
      args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
      args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
      args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
      args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
      auto executeResult = scope.executeFilter(filter, dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

      ClassFourInvariants::AssertInvariants(originalMask, dataStructure);
    }
  }
}

// A second run must preserve the fixed-point result from the nontrivial Case 4
// fixture.
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Class 4 Idempotence", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);
  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_4/case_4_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  BadDataNeighborOrientationCheckFilter filter;
  Arguments args;
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(4));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult1 = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult1.result);
  const auto maskAfterRun1 = ClassFourInvariants::CaptureMask(dataStructure);

  auto executeResult2 = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult2.result);

  const auto& maskArray = dataStructure.getDataRefAs<UInt8Array>(VerificationConstants::k_MaskArrayPath);
  const auto& store = maskArray.getDataStoreRef();
  REQUIRE(maskAfterRun1.size() == store.getSize());
  for(usize i = 0; i < store.getSize(); ++i)
  {
    INFO("Idempotence violated at index " << i);
    REQUIRE(store.getValue(i) == maskAfterRun1[i]);
  }
}

// A 3x3x1 fixture has four face neighbors. It verifies dimension-aware neighbor
// enumeration without Z neighbors.
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: 2D Image Fixture (3x3x1)", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, VerificationConstants::k_ImageName);
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});
  imageGeom->setDimensions({3, 3, 1});

  AttributeMatrix* cellData = AttributeMatrix::Create(dataStructure, Constants::k_Cell_Data, ShapeType{1, 3, 3}, imageGeom->getId());
  AttributeMatrix* cellEnsemble = AttributeMatrix::Create(dataStructure, Constants::k_Cell_Ensemble_Data, ShapeType{2}, imageGeom->getId());

  UInt8Array* maskArray = UnitTest::CreateTestDataArray<uint8>(dataStructure, VerificationConstants::k_MaskName, {1, 3, 3}, {1}, cellData->getId());
  const std::array<uint8, 9> inputMask = {0, 1, 0, 1, 0, 1, 0, 1, 0};
  for(usize i = 0; i < 9; ++i)
  {
    (*maskArray)[i] = inputMask[i];
  }

  Int32Array* phasesArray = UnitTest::CreateTestDataArray<int32>(dataStructure, VerificationConstants::k_PhasesName, {1, 3, 3}, {1}, cellData->getId());
  phasesArray->fill(1);

  // Identical quaternions give zero misorientation.
  Float32Array* quatsArray = UnitTest::CreateTestDataArray<float32>(dataStructure, VerificationConstants::k_QuatsName, {1, 3, 3}, {4}, cellData->getId());
  const float32 q_z = 0.00872654f;
  const float32 q_w = 0.99996191f;
  for(usize i = 0; i < 9; ++i)
  {
    (*quatsArray)[i * 4 + 0] = 0.0f;
    (*quatsArray)[i * 4 + 1] = 0.0f;
    (*quatsArray)[i * 4 + 2] = q_z;
    (*quatsArray)[i * 4 + 3] = q_w;
  }

  // CrystalStructures retains the phase-zero unknown sentinel.
  UInt32Array* crystalStructures = UnitTest::CreateTestDataArray<uint32>(dataStructure, VerificationConstants::k_CStuctsName, {2}, {1}, cellEnsemble->getId());
  (*crystalStructures)[0] = 999u;
  (*crystalStructures)[1] = 1u;

  BadDataNeighborOrientationCheckFilter filter;
  Arguments args;
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(4));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(VerificationConstants::k_ImagePath));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_QuatsArrayPath));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_MaskArrayPath));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_PhasesArrayPath));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(VerificationConstants::k_CStuctsArrayPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const std::array<uint8, 9> expectedMask = {0, 1, 0, 1, 1, 1, 0, 1, 0};
  const auto& maskStore = dataStructure.getDataAs<UInt8Array>(VerificationConstants::k_MaskArrayPath)->getDataStoreRef();
  for(usize i = 0; i < 9; ++i)
  {
    INFO("2D fixture: index " << i);
    REQUIRE(maskStore.getValue(i) == expectedMask[i]);
  }
}
