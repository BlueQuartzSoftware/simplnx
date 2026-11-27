#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/BadDataNeighborOrientationCheckFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
namespace verification_constants
{
const std::string k_ImageName = "Image Geometry";
const DataPath k_ImagePath = DataPath({k_ImageName});

const DataPath k_CellDataPath = k_ImagePath.createChildPath(constants::k_CellDataWithSpaces);
const DataPath k_CellEnsembleDataPath = k_ImagePath.createChildPath(constants::k_CellEnsembleDataWithSpaces);

const std::string k_QuatsName = "Quats";
const DataPath k_QuatsArrayPath = k_CellDataPath.createChildPath(k_QuatsName);
const std::string k_MaskName = "Mask";
const DataPath k_MaskArrayPath = k_CellDataPath.createChildPath(k_MaskName);
const std::string k_PhasesName = "Phases";
const DataPath k_PhasesArrayPath = k_CellDataPath.createChildPath(k_PhasesName);

const std::string k_CStuctsName = "Crystal Structures";
const DataPath k_CStuctsArrayPath = k_CellEnsembleDataPath.createChildPath(k_CStuctsName);
} // namespace verification_constants

namespace class_four_invariants
{
// Capture the mask before invariant checks.
std::vector<uint8> CaptureMask(const DataStructure& dataStructure)
{
  const auto& maskArray = dataStructure.getDataRefAs<UInt8ArrayType>(verification_constants::k_MaskArrayPath);
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
  const auto& maskArray = dataStructure.getDataRefAs<UInt8ArrayType>(verification_constants::k_MaskArrayPath);
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
} // namespace class_four_invariants
} // namespace

// AlgorithmTestScope forces each selected path and records its target-call
// witness.
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.1.1", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_1/case_1_1_1/case_1_1_1_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(1));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(verification_constants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(verification_constants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(verification_constants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(verification_constants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(verification_constants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const std::array<uint8, 27> expectedMask = {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const UInt8AbstractDataStoreType& maskStore = dataStructure.getDataAs<UInt8ArrayType>(verification_constants::k_MaskArrayPath)->getDataStoreRef();
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
  unit_test::WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_1_1.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.1.2", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_1/case_1_1_2/case_1_1_2_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(1));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(verification_constants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(verification_constants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(verification_constants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(verification_constants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(verification_constants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const std::array<uint8, 27> expectedMask = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const UInt8AbstractDataStoreType& maskStore = dataStructure.getDataAs<UInt8ArrayType>(verification_constants::k_MaskArrayPath)->getDataStoreRef();
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
  unit_test::WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_1_2.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.1.3", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_1/case_1_1_3/case_1_1_3_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(1));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(verification_constants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(verification_constants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(verification_constants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(verification_constants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(verification_constants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const std::array<uint8, 27> expectedMask = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const UInt8AbstractDataStoreType& maskStore = dataStructure.getDataAs<UInt8ArrayType>(verification_constants::k_MaskArrayPath)->getDataStoreRef();
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
  unit_test::WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_1_3.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.2.1", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_2/case_1_2_1/case_1_2_1_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(2));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(verification_constants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(verification_constants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(verification_constants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(verification_constants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(verification_constants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const std::array<uint8, 27> expectedMask = {1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const UInt8AbstractDataStoreType& maskStore = dataStructure.getDataAs<UInt8ArrayType>(verification_constants::k_MaskArrayPath)->getDataStoreRef();
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
  unit_test::WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_2_1.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

// Same-phase gating must not reuse a prior neighbor angle. The mixed-phase
// neighbor must not flip voxel zero. See
// vv/deviations/BadDataNeighborOrientationCheckFilter.md D2.
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.2.2", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_2/case_1_2_2/case_1_2_2_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(2));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(verification_constants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(verification_constants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(verification_constants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(verification_constants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(verification_constants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const std::array<uint8, 27> expectedMask = {0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const UInt8AbstractDataStoreType& maskStore = dataStructure.getDataAs<UInt8ArrayType>(verification_constants::k_MaskArrayPath)->getDataStoreRef();
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
  unit_test::WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_2_2.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.2.3", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_2/case_1_2_3/case_1_2_3_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(2));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(verification_constants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(verification_constants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(verification_constants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(verification_constants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(verification_constants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const std::array<uint8, 27> expectedMask = {0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const UInt8AbstractDataStoreType& maskStore = dataStructure.getDataAs<UInt8ArrayType>(verification_constants::k_MaskArrayPath)->getDataStoreRef();
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
  unit_test::WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_2_3.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.3.1", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_3/case_1_3_1/case_1_3_1_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(3));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(verification_constants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(verification_constants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(verification_constants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(verification_constants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(verification_constants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const std::array<uint8, 27> expectedMask = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const UInt8AbstractDataStoreType& maskStore = dataStructure.getDataAs<UInt8ArrayType>(verification_constants::k_MaskArrayPath)->getDataStoreRef();
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
  unit_test::WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_3_1.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.3.2", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_3/case_1_3_2/case_1_3_2_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(3));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(verification_constants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(verification_constants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(verification_constants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(verification_constants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(verification_constants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const std::array<uint8, 27> expectedMask = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const UInt8AbstractDataStoreType& maskStore = dataStructure.getDataAs<UInt8ArrayType>(verification_constants::k_MaskArrayPath)->getDataStoreRef();
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
  unit_test::WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_3_2.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.3.3", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_3/case_1_3_3/case_1_3_3_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(3));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(verification_constants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(verification_constants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(verification_constants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(verification_constants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(verification_constants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const std::array<uint8, 27> expectedMask = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const UInt8AbstractDataStoreType& maskStore = dataStructure.getDataAs<UInt8ArrayType>(verification_constants::k_MaskArrayPath)->getDataStoreRef();
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
  unit_test::WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_3_3.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.4.1", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_4/case_1_4_1/case_1_4_1_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(4));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(verification_constants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(verification_constants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(verification_constants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(verification_constants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(verification_constants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const std::array<uint8, 27> expectedMask = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const UInt8AbstractDataStoreType& maskStore = dataStructure.getDataAs<UInt8ArrayType>(verification_constants::k_MaskArrayPath)->getDataStoreRef();
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
  unit_test::WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_4_1.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.4.2", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_4/case_1_4_2/case_1_4_2_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(4));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(verification_constants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(verification_constants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(verification_constants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(verification_constants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(verification_constants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const std::array<uint8, 27> expectedMask = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const UInt8AbstractDataStoreType& maskStore = dataStructure.getDataAs<UInt8ArrayType>(verification_constants::k_MaskArrayPath)->getDataStoreRef();
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
  unit_test::WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_4_2.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.4.3", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_4/case_1_4_3/case_1_4_3_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(4));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(verification_constants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(verification_constants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(verification_constants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(verification_constants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(verification_constants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const std::array<uint8, 27> expectedMask = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const UInt8AbstractDataStoreType& maskStore = dataStructure.getDataAs<UInt8ArrayType>(verification_constants::k_MaskArrayPath)->getDataStoreRef();
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
  unit_test::WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_4_3.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.5.1", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_5/case_1_5_1/case_1_5_1_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(5));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(verification_constants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(verification_constants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(verification_constants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(verification_constants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(verification_constants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const std::array<uint8, 27> expectedMask = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0};

  const UInt8AbstractDataStoreType& maskStore = dataStructure.getDataAs<UInt8ArrayType>(verification_constants::k_MaskArrayPath)->getDataStoreRef();
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
  unit_test::WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_5_1.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.5.2", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_5/case_1_5_2/case_1_5_2_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(5));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(verification_constants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(verification_constants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(verification_constants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(verification_constants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(verification_constants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const std::array<uint8, 27> expectedMask = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0};

  const UInt8AbstractDataStoreType& maskStore = dataStructure.getDataAs<UInt8ArrayType>(verification_constants::k_MaskArrayPath)->getDataStoreRef();
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
  unit_test::WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_5_2.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.5.3", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_5/case_1_5_3/case_1_5_3_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(5));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(verification_constants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(verification_constants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(verification_constants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(verification_constants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(verification_constants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const std::array<uint8, 27> expectedMask = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0};

  const UInt8AbstractDataStoreType& maskStore = dataStructure.getDataAs<UInt8ArrayType>(verification_constants::k_MaskArrayPath)->getDataStoreRef();
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
  unit_test::WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_5_3.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.6.1", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_6/case_1_6_1/case_1_6_1_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(6));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(verification_constants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(verification_constants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(verification_constants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(verification_constants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(verification_constants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const std::array<uint8, 27> expectedMask = {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0};

  const UInt8AbstractDataStoreType& maskStore = dataStructure.getDataAs<UInt8ArrayType>(verification_constants::k_MaskArrayPath)->getDataStoreRef();
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
  unit_test::WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_6_1.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.6.2", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_6/case_1_6_2/case_1_6_2_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(6));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(verification_constants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(verification_constants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(verification_constants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(verification_constants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(verification_constants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const std::array<uint8, 27> expectedMask = {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0};

  const UInt8AbstractDataStoreType& maskStore = dataStructure.getDataAs<UInt8ArrayType>(verification_constants::k_MaskArrayPath)->getDataStoreRef();
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
  unit_test::WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_6_2.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.6.3", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_1/case_1_6/case_1_6_3/case_1_6_3_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(6));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(verification_constants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(verification_constants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(verification_constants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(verification_constants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(verification_constants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const std::array<uint8, 27> expectedMask = {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0};

  const UInt8AbstractDataStoreType& maskStore = dataStructure.getDataAs<UInt8ArrayType>(verification_constants::k_MaskArrayPath)->getDataStoreRef();
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
  unit_test::WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_1_6_3.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 2.1", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_2/case_2_1/case_2_1_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(5));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(verification_constants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(verification_constants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(verification_constants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(verification_constants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(verification_constants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const UInt8AbstractDataStoreType& maskStore = dataStructure.getDataAs<UInt8ArrayType>(verification_constants::k_MaskArrayPath)->getDataStoreRef();
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
  unit_test::WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_2_1.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 2.2", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_2/case_2_2/case_2_2_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(5));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(verification_constants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(verification_constants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(verification_constants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(verification_constants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(verification_constants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const UInt8AbstractDataStoreType& maskStore = dataStructure.getDataAs<UInt8ArrayType>(verification_constants::k_MaskArrayPath)->getDataStoreRef();
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
  unit_test::WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_2_2.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 2.3", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_2/case_2_3/case_2_3_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(5));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(verification_constants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(verification_constants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(verification_constants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(verification_constants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(verification_constants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const UInt8AbstractDataStoreType& maskStore = dataStructure.getDataAs<UInt8ArrayType>(verification_constants::k_MaskArrayPath)->getDataStoreRef();
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
  unit_test::WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_2_3.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 2.4", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_2/case_2_4/case_2_4_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(5));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(verification_constants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(verification_constants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(verification_constants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(verification_constants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(verification_constants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const UInt8AbstractDataStoreType& maskStore = dataStructure.getDataAs<UInt8ArrayType>(verification_constants::k_MaskArrayPath)->getDataStoreRef();
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
  unit_test::WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_2_4.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 2.5", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_2/case_2_5/case_2_5_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(5));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(verification_constants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(verification_constants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(verification_constants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(verification_constants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(verification_constants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const UInt8AbstractDataStoreType& maskStore = dataStructure.getDataAs<UInt8ArrayType>(verification_constants::k_MaskArrayPath)->getDataStoreRef();
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
  unit_test::WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_2_5.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 2.6", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_2/case_2_6/case_2_6_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(5));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(verification_constants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(verification_constants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(verification_constants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(verification_constants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(verification_constants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const UInt8AbstractDataStoreType& maskStore = dataStructure.getDataAs<UInt8ArrayType>(verification_constants::k_MaskArrayPath)->getDataStoreRef();
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
  unit_test::WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_2_6.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 3.1", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_3/case_3_1/case_3_1_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(1));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(verification_constants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(verification_constants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(verification_constants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(verification_constants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(verification_constants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const UInt8AbstractDataStoreType& maskStore = dataStructure.getDataAs<UInt8ArrayType>(verification_constants::k_MaskArrayPath)->getDataStoreRef();
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
  unit_test::WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_3_1.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 3.2", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_3/case_3_2/case_3_2_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(1));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(verification_constants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(verification_constants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(verification_constants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(verification_constants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(verification_constants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const UInt8AbstractDataStoreType& maskStore = dataStructure.getDataAs<UInt8ArrayType>(verification_constants::k_MaskArrayPath)->getDataStoreRef();
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
  unit_test::WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_3_2.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 4", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_4/case_4_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  {
    BadDataNeighborOrientationCheckFilter filter;
    Arguments args;

    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(4));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(verification_constants::k_ImagePath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(verification_constants::k_QuatsArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(verification_constants::k_MaskArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(verification_constants::k_PhasesArrayPath));
    args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(verification_constants::k_CStuctsArrayPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

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

  const UInt8AbstractDataStoreType& maskStore = dataStructure.getDataAs<UInt8ArrayType>(verification_constants::k_MaskArrayPath)->getDataStoreRef();
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
  unit_test::WriteTestDataStructure(dataStructure, fmt::format("{}/verification/bad_data_neighbor_orientation_check_v2/case_4.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Preflight Error - Cell array tuple count mismatch (-6809)",
          "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter][preflight]")
{
  unit_test::LoadPlugins();

  // Mask, CellPhases, and Quats have different tuple counts and must report
  // -6809.
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Image Geometry");
  imageGeomPtr->setDimensions({10, 1, 1});

  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {10}, imageGeomPtr->getId());
  unit_test::CreateTestDataArray<float32>(dataStructure, "Quats", {10}, {4}, cellAmPtr->getId());
  unit_test::CreateTestDataArray<uint8>(dataStructure, "Mask", {10}, {1}, cellAmPtr->getId());

  // The separate group gives CellPhases nine tuples instead of ten.
  auto* mismatchAmPtr = AttributeMatrix::Create(dataStructure, "MismatchData", {9}, imageGeomPtr->getId());
  unit_test::CreateTestDataArray<int32>(dataStructure, "Phases", {9}, {1}, mismatchAmPtr->getId());

  auto* ensembleAmPtr = AttributeMatrix::Create(dataStructure, "CellEnsembleData", {2}, imageGeomPtr->getId());
  unit_test::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", {2}, {1}, ensembleAmPtr->getId());

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
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.output_actions);
  REQUIRE(preflightResult.output_actions.errors()[0].code == -6809);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Preflight Error - Number of neighbors out of range (-6810)",
          "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter][preflight]")
{
  unit_test::LoadPlugins();
  const int32 numberOfNeighbors = GENERATE(7, 0, -1);
  CAPTURE(numberOfNeighbors);

  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Image Geometry");
  imageGeomPtr->setDimensions({10, 1, 1});

  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", {10}, imageGeomPtr->getId());
  unit_test::CreateTestDataArray<float32>(dataStructure, "Quats", {10}, {4}, cellAmPtr->getId());
  unit_test::CreateTestDataArray<uint8>(dataStructure, "Mask", {10}, {1}, cellAmPtr->getId());
  unit_test::CreateTestDataArray<int32>(dataStructure, "Phases", {10}, {1}, cellAmPtr->getId());

  auto* ensembleAmPtr = AttributeMatrix::Create(dataStructure, "CellEnsembleData", {2}, imageGeomPtr->getId());
  unit_test::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", {2}, {1}, ensembleAmPtr->getId());

  BadDataNeighborOrientationCheckFilter filter;
  Arguments args;
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(numberOfNeighbors));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry", "CellData", "Quats"})));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry", "CellData", "Mask"})));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry", "CellData", "Phases"})));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry", "CellEnsembleData", "CrystalStructures"})));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.output_actions);
  REQUIRE(preflightResult.output_actions.errors()[0].code == -6810);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "BadDataNeighborOrientationCheckFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "BadDataNeighborOrientationCheckFilter.json"},
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
      REQUIRE(filterPtr->uuid() == FilterTraits<BadDataNeighborOrientationCheckFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
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
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);

  /**
   * @struct Fixture
   * @brief Identifies one invariant fixture and its neighbor requirement.
   */
  struct Fixture
  {
    std::string rel_path;
    int32 number_of_neighbors;
  };

  // Case 1 covers all neighbor counts and phase configurations.
  const std::vector<Fixture> fixtures = {
      {.rel_path = "case_1/case_1_1/case_1_1_1/case_1_1_1_input.dream3d", .number_of_neighbors = 1}, {.rel_path = "case_1/case_1_1/case_1_1_2/case_1_1_2_input.dream3d", .number_of_neighbors = 1},
      {.rel_path = "case_1/case_1_1/case_1_1_3/case_1_1_3_input.dream3d", .number_of_neighbors = 1}, {.rel_path = "case_1/case_1_2/case_1_2_1/case_1_2_1_input.dream3d", .number_of_neighbors = 2},
      {.rel_path = "case_1/case_1_2/case_1_2_2/case_1_2_2_input.dream3d", .number_of_neighbors = 2}, {.rel_path = "case_1/case_1_2/case_1_2_3/case_1_2_3_input.dream3d", .number_of_neighbors = 2},
      {.rel_path = "case_1/case_1_3/case_1_3_1/case_1_3_1_input.dream3d", .number_of_neighbors = 3}, {.rel_path = "case_1/case_1_3/case_1_3_2/case_1_3_2_input.dream3d", .number_of_neighbors = 3},
      {.rel_path = "case_1/case_1_3/case_1_3_3/case_1_3_3_input.dream3d", .number_of_neighbors = 3}, {.rel_path = "case_1/case_1_4/case_1_4_1/case_1_4_1_input.dream3d", .number_of_neighbors = 4},
      {.rel_path = "case_1/case_1_4/case_1_4_2/case_1_4_2_input.dream3d", .number_of_neighbors = 4}, {.rel_path = "case_1/case_1_4/case_1_4_3/case_1_4_3_input.dream3d", .number_of_neighbors = 4},
      {.rel_path = "case_1/case_1_5/case_1_5_1/case_1_5_1_input.dream3d", .number_of_neighbors = 5}, {.rel_path = "case_1/case_1_5/case_1_5_2/case_1_5_2_input.dream3d", .number_of_neighbors = 5},
      {.rel_path = "case_1/case_1_5/case_1_5_3/case_1_5_3_input.dream3d", .number_of_neighbors = 5}, {.rel_path = "case_1/case_1_6/case_1_6_1/case_1_6_1_input.dream3d", .number_of_neighbors = 6},
      {.rel_path = "case_1/case_1_6/case_1_6_2/case_1_6_2_input.dream3d", .number_of_neighbors = 6}, {.rel_path = "case_1/case_1_6/case_1_6_3/case_1_6_3_input.dream3d", .number_of_neighbors = 6},
  };

  for(const auto& fixture : fixtures)
  {
    DYNAMIC_SECTION("Fixture: " << fixture.rel_path << " NumberOfNeighbors=" << fixture.number_of_neighbors)
    {
      auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/{}", unit_test::k_TestFilesDir, fixture.rel_path));
      DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

      const auto originalMask = class_four_invariants::CaptureMask(dataStructure);

      BadDataNeighborOrientationCheckFilter filter;
      Arguments args;
      args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
      args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(fixture.number_of_neighbors));
      args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(verification_constants::k_ImagePath));
      args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(verification_constants::k_QuatsArrayPath));
      args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(verification_constants::k_MaskArrayPath));
      args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(verification_constants::k_PhasesArrayPath));
      args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(verification_constants::k_CStuctsArrayPath));

      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
      auto executeResult = scope.executeFilter(filter, dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

      class_four_invariants::AssertInvariants(originalMask, dataStructure);
    }
  }
}

// A second run must preserve the fixed-point result from the nontrivial Case 4
// fixture.
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Class 4 Idempotence", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "bad_data_neighbor_orientation_check_v2.tar.gz", "bad_data_neighbor_orientation_check_v2", true, true);
  auto baseDataFilePath = fs::path(fmt::format("{}/bad_data_neighbor_orientation_check_v2/case_4/case_4_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  BadDataNeighborOrientationCheckFilter filter;
  Arguments args;
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(4));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(verification_constants::k_ImagePath));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(verification_constants::k_QuatsArrayPath));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(verification_constants::k_MaskArrayPath));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(verification_constants::k_PhasesArrayPath));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(verification_constants::k_CStuctsArrayPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

  auto executeResult1 = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult1.result);
  const auto maskAfterRun1 = class_four_invariants::CaptureMask(dataStructure);

  auto executeResult2 = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult2.result);

  const auto& maskArray = dataStructure.getDataRefAs<UInt8ArrayType>(verification_constants::k_MaskArrayPath);
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
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  ImageGeom* imageGeomPtr = ImageGeom::Create(dataStructure, verification_constants::k_ImageName);
  REQUIRE(imageGeomPtr->setSpacing({1.0f, 1.0f, 1.0f}).valid());
  REQUIRE(imageGeomPtr->setOrigin({0.0f, 0.0f, 0.0f}).valid());
  imageGeomPtr->setDimensions({3, 3, 1});

  AttributeMatrix* cellDataPtr = AttributeMatrix::Create(dataStructure, constants::k_CellDataWithSpaces, ShapeType{1, 3, 3}, imageGeomPtr->getId());
  AttributeMatrix* cellEnsemblePtr = AttributeMatrix::Create(dataStructure, constants::k_CellEnsembleDataWithSpaces, ShapeType{2}, imageGeomPtr->getId());

  UInt8ArrayType* maskArrayPtr = unit_test::CreateTestDataArray<uint8>(dataStructure, verification_constants::k_MaskName, {1, 3, 3}, {1}, cellDataPtr->getId());
  const std::array<uint8, 9> inputMask = {0, 1, 0, 1, 0, 1, 0, 1, 0};
  for(usize i = 0; i < 9; ++i)
  {
    (*maskArrayPtr)[i] = inputMask[i];
  }

  Int32ArrayType* phasesArrayPtr = unit_test::CreateTestDataArray<int32>(dataStructure, verification_constants::k_PhasesName, {1, 3, 3}, {1}, cellDataPtr->getId());
  phasesArrayPtr->fill(1);

  // Identical quaternions give zero misorientation.
  Float32ArrayType* quatsArrayPtr = unit_test::CreateTestDataArray<float32>(dataStructure, verification_constants::k_QuatsName, {1, 3, 3}, {4}, cellDataPtr->getId());
  const float32 qZ = 0.00872654f;
  const float32 qW = 0.99996191f;
  for(usize i = 0; i < 9; ++i)
  {
    (*quatsArrayPtr)[i * 4 + 0] = 0.0f;
    (*quatsArrayPtr)[i * 4 + 1] = 0.0f;
    (*quatsArrayPtr)[i * 4 + 2] = qZ;
    (*quatsArrayPtr)[i * 4 + 3] = qW;
  }

  // CrystalStructures retains the phase-zero unknown sentinel.
  UInt32ArrayType* crystalStructuresPtr = unit_test::CreateTestDataArray<uint32>(dataStructure, verification_constants::k_CStuctsName, {2}, {1}, cellEnsemblePtr->getId());
  (*crystalStructuresPtr)[0] = 999u;
  (*crystalStructuresPtr)[1] = 1u;

  BadDataNeighborOrientationCheckFilter filter;
  Arguments args;
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(4));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(verification_constants::k_ImagePath));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(verification_constants::k_QuatsArrayPath));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(verification_constants::k_MaskArrayPath));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(verification_constants::k_PhasesArrayPath));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(verification_constants::k_CStuctsArrayPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const std::array<uint8, 9> expectedMask = {0, 1, 0, 1, 1, 1, 0, 1, 0};
  const auto& maskStore = dataStructure.getDataAs<UInt8ArrayType>(verification_constants::k_MaskArrayPath)->getDataStoreRef();
  for(usize i = 0; i < 9; ++i)
  {
    INFO("2D fixture: index " << i);
    REQUIRE(maskStore.getValue(i) == expectedMask[i]);
  }
}

TEMPLATE_TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: real HDF5 reverse-slice cascade oracle",
                   "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter][.OocStoreContract]", uint8, bool)
{
  unit_test::LoadPlugins();
  REQUIRE(Application::Instance()->getIOManager("HDF5-OOC") != nullptr);
  const unit_test::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceOutOfCore, 1);
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, verification_constants::k_ImageName);
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({3, 3, 5});
  REQUIRE(imageGeomPtr->setOrigin({0.0F, 0.0F, 0.0F}).valid());
  REQUIRE(imageGeomPtr->setSpacing({1.0F, 1.0F, 1.0F}).valid());
  auto* cellsPtr = AttributeMatrix::Create(dataStructure, constants::k_CellDataWithSpaces, {5, 3, 3}, imageGeomPtr->getId());
  auto* ensemblesPtr = AttributeMatrix::Create(dataStructure, constants::k_CellEnsembleDataWithSpaces, {2}, imageGeomPtr->getId());
  REQUIRE(cellsPtr != nullptr);
  REQUIRE(ensemblesPtr != nullptr);
  imageGeomPtr->setCellData(*cellsPtr);
  auto maskStore = data_store_utilities::CreateDataStore<TestType>(dataStructure, verification_constants::k_MaskArrayPath, {5, 3, 3}, {1});
  auto phasesStore = data_store_utilities::CreateDataStore<int32>(dataStructure, verification_constants::k_PhasesArrayPath, {5, 3, 3}, {1});
  auto quatsStore = data_store_utilities::CreateDataStore<float32>(dataStructure, verification_constants::k_QuatsArrayPath, {5, 3, 3}, {4});
  auto crystalsStore = data_store_utilities::CreateDataStore<uint32>(dataStructure, verification_constants::k_CStuctsArrayPath, {2}, {1});
  auto* maskPtr = DataArray<TestType>::Create(dataStructure, verification_constants::k_MaskName, maskStore, cellsPtr->getId());
  auto* phasesPtr = Int32ArrayType::Create(dataStructure, verification_constants::k_PhasesName, phasesStore, cellsPtr->getId());
  auto* quatsPtr = Float32ArrayType::Create(dataStructure, verification_constants::k_QuatsName, quatsStore, cellsPtr->getId());
  auto* crystalsPtr = UInt32ArrayType::Create(dataStructure, verification_constants::k_CStuctsName, crystalsStore, ensemblesPtr->getId());
  REQUIRE(maskPtr != nullptr);
  REQUIRE(phasesPtr != nullptr);
  REQUIRE(quatsPtr != nullptr);
  REQUIRE(crystalsPtr != nullptr);
  maskPtr->fill(0);
  phasesPtr->fill(0);
  std::array<float32, 45 * 4> quaternionValues{};
  for(usize voxelIdx = 0; voxelIdx < 45; voxelIdx++)
  {
    quaternionValues[voxelIdx * 4 + 3] = 1.0F;
  }
  for(usize z = 0; z < 5; z++)
  {
    (*phasesPtr)[z * 9 + 4] = 1;
  }
  (*maskPtr)[40] = 1;
  // A 30-degree rotation about Z exceeds the one-degree matching tolerance.
  (*phasesPtr)[23] = 1;
  quaternionValues[23 * 4 + 2] = 0.2588190451F;
  quaternionValues[23 * 4 + 3] = 0.9659258263F;
  auto writeResult = quatsStore->copyFromBuffer(0, nonstd::span<const float32>(quaternionValues.data(), quaternionValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  (*crystalsPtr)[0] = 999;
  (*crystalsPtr)[1] = 1;
  REQUIRE(maskStore->getDataFormat() == "HDF5-OOC");
  REQUIRE(phasesStore->getDataFormat() == "HDF5-OOC");
  REQUIRE(quatsStore->getDataFormat() == "HDF5-OOC");
  REQUIRE(crystalsStore->getDataFormat() == "HDF5-OOC");

  BadDataNeighborOrientationCheckFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(1.0F));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(1));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(verification_constants::k_ImagePath));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(verification_constants::k_QuatsArrayPath));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(verification_constants::k_MaskArrayPath));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(verification_constants::k_PhasesArrayPath));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(verification_constants::k_CStuctsArrayPath));
  const auto before = GetAlgorithmPathExecutionCounts();
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const auto after = GetAlgorithmPathExecutionCounts();
  REQUIRE(after.out_of_core_on_out_of_core_store == before.out_of_core_on_out_of_core_store + 1);
  REQUIRE(after.in_core == before.in_core);
  REQUIRE(maskPtr->getDataStoreRef().getDataFormat() == "HDF5-OOC");
  for(usize voxelIdx = 0; voxelIdx < 45; voxelIdx++)
  {
    INFO("voxel " << voxelIdx);
    REQUIRE(static_cast<bool>((*maskPtr)[voxelIdx]) == (voxelIdx % 9 == 4));
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Phase Index Bounds", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  const bool invalidBadVoxel = GENERATE(false, true);
  CAPTURE(scenario, invalidBadVoxel);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, verification_constants::k_ImageName);
  REQUIRE(imageGeomPtr->setSpacing({1.0F, 1.0F, 1.0F}).valid());
  REQUIRE(imageGeomPtr->setOrigin({0.0F, 0.0F, 0.0F}).valid());
  imageGeomPtr->setDimensions({3, 3, 1});

  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, constants::k_CellDataWithSpaces, ShapeType{1, 3, 3}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellDataPtr);
  auto* cellEnsemblePtr = AttributeMatrix::Create(dataStructure, constants::k_CellEnsembleDataWithSpaces, ShapeType{2}, imageGeomPtr->getId());

  auto* maskArrayPtr = unit_test::CreateTestDataArray<uint8>(dataStructure, verification_constants::k_MaskName, {1, 3, 3}, {1}, cellDataPtr->getId());
  maskArrayPtr->fill(1);
  (*maskArrayPtr)[4] = 0;

  auto* cellPhasesArrayPtr = unit_test::CreateTestDataArray<int32>(dataStructure, verification_constants::k_PhasesName, {1, 3, 3}, {1}, cellDataPtr->getId());
  cellPhasesArrayPtr->fill(1);
  (*cellPhasesArrayPtr)[invalidBadVoxel ? 4 : 0] = 2;

  auto* quatsArrayPtr = unit_test::CreateTestDataArray<float32>(dataStructure, verification_constants::k_QuatsName, {1, 3, 3}, {4}, cellDataPtr->getId());
  quatsArrayPtr->fill(0.0F);
  for(usize voxelIdx = 0; voxelIdx < 9; ++voxelIdx)
  {
    (*quatsArrayPtr)[voxelIdx * 4 + 3] = 1.0F;
  }

  auto* crystalStructuresArrayPtr = unit_test::CreateTestDataArray<uint32>(dataStructure, verification_constants::k_CStuctsName, {2}, {1}, cellEnsemblePtr->getId());
  (*crystalStructuresArrayPtr)[0] = 999U;
  (*crystalStructuresArrayPtr)[1] = 1U;

  BadDataNeighborOrientationCheckFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_NumberOfNeighbors_Key, std::make_any<int32>(4));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(verification_constants::k_ImagePath));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(verification_constants::k_QuatsArrayPath));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(verification_constants::k_MaskArrayPath));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(verification_constants::k_PhasesArrayPath));
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(verification_constants::k_CStuctsArrayPath));

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  if(invalidBadVoxel)
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == -54902);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Geometry cell tuple counts", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter][TupleCount]")
{
  // Cases: all short, all long, all matching outside Cell Data, then each input short.
  const int testCase = GENERATE(0, 1, 2, 3, 4, 5);
  CAPTURE(testCase);
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "TupleGeometry");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({2, 2, 2});
  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, "Cell Data", {2, 2, 2}, imageGeomPtr->getId());
  REQUIRE(cellDataPtr != nullptr);
  imageGeomPtr->setCellData(*cellDataPtr);

  BadDataNeighborOrientationCheckFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"TupleGeometry"})));
  const usize commonCount = testCase == 0 ? 4 : (testCase == 1 ? 12 : 8);
  const usize count0 = testCase == 3 ? 4 : commonCount;
  auto* inputAM0Ptr = AttributeMatrix::Create(dataStructure, "Input0", {count0});
  REQUIRE(inputAM0Ptr != nullptr);
  REQUIRE(DataArray<float32>::CreateWithStore<DataStore<float32>>(dataStructure, "Quats", {count0}, {4}, inputAM0Ptr->getId()) != nullptr);
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(DataPath({"Input0", "Quats"})));
  const usize count1 = testCase == 4 ? 4 : commonCount;
  auto* inputAM1Ptr = AttributeMatrix::Create(dataStructure, "Input1", {count1});
  REQUIRE(inputAM1Ptr != nullptr);
  REQUIRE(DataArray<int32>::CreateWithStore<DataStore<int32>>(dataStructure, "Phases", {count1}, {1}, inputAM1Ptr->getId()) != nullptr);
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"Input1", "Phases"})));
  const usize count2 = testCase == 5 ? 4 : commonCount;
  auto* inputAM2Ptr = AttributeMatrix::Create(dataStructure, "Input2", {count2});
  REQUIRE(inputAM2Ptr != nullptr);
  REQUIRE(DataArray<uint8>::CreateWithStore<DataStore<uint8>>(dataStructure, "Mask", {count2}, {1}, inputAM2Ptr->getId()) != nullptr);
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Input2", "Mask"})));
  REQUIRE(UInt32ArrayType::CreateWithStore<DataStore<uint32>>(dataStructure, "CrystalStructures", {2}, {1}) != nullptr);
  args.insertOrAssign(BadDataNeighborOrientationCheckFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"CrystalStructures"})));

  const auto result = filter.preflight(dataStructure, args);
  if(testCase == 2)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
    REQUIRE(result.output_actions.errors().size() == 1);
    const auto& errorRef = result.output_actions.errors()[0];
    const std::vector<DataPath> inputPaths = {DataPath({"Input0", "Quats"}), DataPath({"Input1", "Phases"}), DataPath({"Input2", "Mask"})};
    if(testCase >= 3)
    {
      CHECK(errorRef.code == -6809);
      CHECK(errorRef.message.find(fmt::format("DataPath: {}    | Tuple Count: 4", inputPaths[static_cast<usize>(testCase - 3)].toString())) != std::string::npos);
    }
    else
    {
      CHECK(errorRef.code == -6810);
      CHECK(errorRef.message.find(inputPaths[0].toString()) != std::string::npos);
      CHECK(errorRef.message.find(testCase == 1 ? "12 tuples" : "4 tuples") != std::string::npos);
      CHECK(errorRef.message.find("8 cells") != std::string::npos);
      CHECK(errorRef.message.find("TupleGeometry") != std::string::npos);
    }
  }
}
