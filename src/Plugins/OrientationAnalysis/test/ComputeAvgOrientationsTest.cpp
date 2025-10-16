/*
# Test Plan

Input Files:
DREAM3D_Data/TestFiles/ASCIIData/FeatureIds.csv (int32, 1 component)
DREAM3D_Data/TestFiles/ASCIIData/Quats.csv (float32, 4 component)
DREAM3D_Data/TestFiles/ASCIIData/Phases.csv (int32, 1 component)

Output DataArrays:
AvgEulerAngles  (float32, 3 component)
AvgQuats  (float32, 4 component)

Comparison Files:
DREAM3D_Data/TestFiles/ASCIIData/AvgEulerAngles.csv
DREAM3D_Data/TestFiles/ASCIIData/AvgQuats.csv

You will need to create a UInt32 DataArray with 2 values in it: [ 999, 1 ]. This will
be the input 'k_CrystalStructuresArrayPath_Key' path and data.


Compare the data sets. Due to going back and forth between ASCII and Binary you will
probably have to compare using a tolerance of about .0001. Look at the 'ConvertOrientationsTest' at the bottom for an example
of doing that.

*/
#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/ComputeAvgOrientationsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumericTypeParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;

namespace compute_avg_orientation
{
const std::string k_ImageDataContainerName = "ImageGeom";
const DataPath k_ImageDataContainerPath({k_ImageDataContainerName});
const DataPath k_CellDataPath = k_ImageDataContainerPath.createChildPath("Cell Data");
const DataPath k_FeatureIdsPath = k_CellDataPath.createChildPath("FeatureIds");
const DataPath k_PhasesPath = k_CellDataPath.createChildPath("Phases");
const DataPath k_QuatsPath = k_CellDataPath.createChildPath("Quats");

const DataPath k_CrystalStructuresPath = k_ImageDataContainerPath.createChildPath("Cell Ensemble Data").createChildPath("CrystalStructures");

const DataPath k_FeatureAttrMatPath = k_ImageDataContainerPath.createChildPath("Cell Feature Data");

const std::string k_AvgQuatsName = "Computed AvgQuats";
const std::string k_AvgEulersName = "Computed AvgEulerAngles";

const std::string k_Exemplar_AvgQuatsName = "AvgQuats";
const std::string k_Exemplar_AvgEulersName = "AvgEulerAngles";

} // namespace compute_avg_orientation

TEST_CASE("OrientationAnalysis::ComputeAvgOrientations", "[OrientationAnalysis][ComputeAvgOrientations]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "7_ComputeAvgOrientation.tar.gz", "7_ComputeAvgOrientation");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/7_ComputeAvgOrientation/7_ComputeAvgOrientation.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeAvgOrientationsFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(compute_avg_orientation::k_FeatureIdsPath));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(compute_avg_orientation::k_PhasesPath));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_CellQuatsArrayPath_Key, std::make_any<DataPath>(compute_avg_orientation::k_QuatsPath));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(compute_avg_orientation::k_CrystalStructuresPath));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_AvgQuatsArrayName_Key, std::make_any<std::string>(compute_avg_orientation::k_AvgQuatsName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_AvgEulerAnglesArrayName_Key, std::make_any<std::string>(compute_avg_orientation::k_AvgEulersName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(compute_avg_orientation::k_FeatureAttrMatPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/compute_average_orientations.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  DataPath computedAvgEulersPath = compute_avg_orientation::k_FeatureAttrMatPath.createChildPath(compute_avg_orientation::k_AvgEulersName);
  DataPath exemplarAvgEulersPath = compute_avg_orientation::k_FeatureAttrMatPath.createChildPath(compute_avg_orientation::k_Exemplar_AvgEulersName);

  UnitTest::CompareFloatArraysWithNans<float32>(dataStructure, computedAvgEulersPath, exemplarAvgEulersPath, 5.0E-7f, false);

  DataPath computedAvgQuatsPath = compute_avg_orientation::k_FeatureAttrMatPath.createChildPath(compute_avg_orientation::k_AvgQuatsName);
  DataPath exemplarAvgQuatsPath = compute_avg_orientation::k_FeatureAttrMatPath.createChildPath(compute_avg_orientation::k_Exemplar_AvgQuatsName);

  UnitTest::CompareFloatArraysWithNans<float32>(dataStructure, computedAvgQuatsPath, exemplarAvgQuatsPath, 5.0E-7f, false);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
