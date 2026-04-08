#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/BadDataNeighborOrientationCheckFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

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

namespace
{
constexpr usize k_BenchDim = 200;
constexpr usize k_BenchTotalVoxels = k_BenchDim * k_BenchDim * k_BenchDim;
constexpr usize k_BenchHalf = k_BenchDim / 2;
const float32 k_BaseAngles[8] = {0.0f, 15.0f, 30.0f, 45.0f, 60.0f, 75.0f, 90.0f, 105.0f};

void BuildOrientationOctantDataset(DataStructure& ds)
{
  const ShapeType cellTupleShape = {k_BenchDim, k_BenchDim, k_BenchDim};

  ImageGeom* imageGeom = ImageGeom::Create(ds, VerificationConstants::k_ImageName);
  imageGeom->setDimensions({k_BenchDim, k_BenchDim, k_BenchDim});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

  AttributeMatrix* cellAM = AttributeMatrix::Create(ds, Constants::k_Cell_Data, cellTupleShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  AttributeMatrix* ensembleAM = AttributeMatrix::Create(ds, Constants::k_Cell_Ensemble_Data, {2}, imageGeom->getId());

  // Crystal Structures: [999 (Unknown), 1 (Cubic_High)]
  auto csStore = DataStoreUtilities::CreateDataStore<uint32>({2}, {1}, IDataAction::Mode::Execute);
  auto* crystalStructures = DataArray<uint32>::Create(ds, VerificationConstants::k_CStuctsName, csStore, ensembleAM->getId());
  auto& csRef = crystalStructures->getDataStoreRef();
  csRef.setValue(0, 999);
  csRef.setValue(1, 1);

  // Phases: all phase 1 — bulk-write per Z-slice to avoid per-element OOC overhead
  auto phasesStore = DataStoreUtilities::CreateDataStore<int32>(cellTupleShape, {1}, IDataAction::Mode::Execute);
  auto* phases = DataArray<int32>::Create(ds, VerificationConstants::k_PhasesName, phasesStore, cellAM->getId());
  auto& phasesRef = phases->getDataStoreRef();
  {
    const usize sliceSize = k_BenchDim * k_BenchDim;
    std::vector<int32> phaseSlice(sliceSize, 1);
    for(usize iz = 0; iz < k_BenchDim; iz++)
    {
      phasesRef.copyFromBuffer(iz * sliceSize, nonstd::span<const int32>(phaseSlice.data(), sliceSize));
    }
  }

  // Mask: ~40% bad voxels (mask=0) — bulk-write per Z-slice
  auto maskStore = DataStoreUtilities::CreateDataStore<uint8>(cellTupleShape, {1}, IDataAction::Mode::Execute);
  auto* mask = DataArray<uint8>::Create(ds, VerificationConstants::k_MaskName, maskStore, cellAM->getId());
  auto& maskRef = mask->getDataStoreRef();
  {
    const usize sliceSize = k_BenchDim * k_BenchDim;
    std::vector<uint8> maskSlice(sliceSize);
    for(usize iz = 0; iz < k_BenchDim; iz++)
    {
      for(usize iy = 0; iy < k_BenchDim; iy++)
      {
        for(usize ix = 0; ix < k_BenchDim; ix++)
        {
          const usize inSlice = iy * k_BenchDim + ix;
          const usize globalIdx = iz * sliceSize + inSlice;
          maskSlice[inSlice] = static_cast<uint8>(((globalIdx * 7 + 13) % 100) >= 40 ? 1 : 0);
        }
      }
      maskRef.copyFromBuffer(iz * sliceSize, nonstd::span<const uint8>(maskSlice.data(), sliceSize));
    }
  }

  // Quats: 8 octants with distinct base quaternions (15 deg apart about Z axis) — bulk-write per Z-slice
  auto quatsStore = DataStoreUtilities::CreateDataStore<float32>(cellTupleShape, {4}, IDataAction::Mode::Execute);
  auto* quats = DataArray<float32>::Create(ds, VerificationConstants::k_QuatsName, quatsStore, cellAM->getId());
  auto& quatsRef = quats->getDataStoreRef();
  {
    const usize sliceSize = k_BenchDim * k_BenchDim;
    const usize quatSliceElems = sliceSize * 4;
    std::vector<float32> quatSlice(quatSliceElems);
    for(usize iz = 0; iz < k_BenchDim; iz++)
    {
      for(usize iy = 0; iy < k_BenchDim; iy++)
      {
        for(usize ix = 0; ix < k_BenchDim; ix++)
        {
          const usize inSlice = iy * k_BenchDim + ix;
          const usize octant = (iz >= k_BenchHalf ? 4 : 0) + (iy >= k_BenchHalf ? 2 : 0) + (ix >= k_BenchHalf ? 1 : 0);
          const usize globalIdx = iz * sliceSize + inSlice;

          const float32 perturbDeg = 0.5f * static_cast<float32>(globalIdx % 10);
          const float32 angleDeg = k_BaseAngles[octant] + perturbDeg;
          const float32 halfAngle = angleDeg * 3.14159265f / 360.0f;

          const float32 w = std::cos(halfAngle);
          const float32 zVal = std::sin(halfAngle);
          quatSlice[inSlice * 4 + 0] = w;
          quatSlice[inSlice * 4 + 1] = 0.0f;
          quatSlice[inSlice * 4 + 2] = 0.0f;
          quatSlice[inSlice * 4 + 3] = zVal;
        }
      }
      quatsRef.copyFromBuffer(iz * quatSliceElems, nonstd::span<const float32>(quatSlice.data(), quatSliceElems));
    }
  }
}
} // namespace

// Case 1.1.1: Base Case | 2 phase | Tolerance 5 | 1 Min Neighbors
TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.1.1", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  UnitTest::LoadPlugins();
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 140, true);

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
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 140, true);

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
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 140, true);

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
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 140, true);

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
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 140, true);

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
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 140, true);

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
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 140, true);

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
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 140, true);

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
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 140, true);

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
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 140, true);

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
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 140, true);

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
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 140, true);

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
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 140, true);

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
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 140, true);

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
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 140, true);

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
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 140, true);

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
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 140, true);

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
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 140, true);

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
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 400, true);

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
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 400, true);

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
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 400, true);

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
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 400, true);

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
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 400, true);

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
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 400, true);

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
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 400, true);

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
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 400, true);

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
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 400, true);

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

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Generate Large Test Dataset", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter][.GenerateTestData]")
{
  UnitTest::LoadPlugins();

  DataStructure buildDS;
  BuildOrientationOctantDataset(buildDS);

  const auto outputDir = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "generated_test_data";
  fs::create_directories(outputDir);
  const auto outputFile = outputDir / "bad_data_neighbor_orientation_check_data.dream3d";
  UnitTest::WriteTestDataStructure(buildDS, outputFile);
  fmt::print("Wrote test data to: {}\n", outputFile.string());
}

TEST_CASE("OrientationAnalysis::BadDataNeighborOrientationCheckFilter: 200x200x200 octant orientations", "[OrientationAnalysis][BadDataNeighborOrientationCheckFilter]")
{
  UnitTest::LoadPlugins();
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOoc = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOoc);
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 10000, true);

  DataStructure dataStructure;
  BuildOrientationOctantDataset(dataStructure);
  UnitTest::RequireExpectedStoreType(dataStructure.getDataRefAs<IDataArray>(VerificationConstants::k_QuatsArrayPath));

  // Count initial bad voxels
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<uint8>>(VerificationConstants::k_MaskArrayPath));
  const auto& maskBefore = dataStructure.getDataRefAs<DataArray<uint8>>(VerificationConstants::k_MaskArrayPath).getDataStoreRef();
  usize badBefore = 0;
  for(usize i = 0; i < k_BenchTotalVoxels; i++)
  {
    if(maskBefore[i] == 0)
    {
      badBefore++;
    }
  }

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

  // Verify: Some bad voxels should have been corrected (mask flipped 0->1)
  const auto& maskAfter = dataStructure.getDataRefAs<DataArray<uint8>>(VerificationConstants::k_MaskArrayPath).getDataStoreRef();
  usize badAfter = 0;
  for(usize i = 0; i < k_BenchTotalVoxels; i++)
  {
    if(maskAfter[i] == 0)
    {
      badAfter++;
    }
  }
  REQUIRE(badAfter < badBefore);
}
