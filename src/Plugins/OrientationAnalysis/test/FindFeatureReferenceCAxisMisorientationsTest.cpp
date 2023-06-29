#include <catch2/catch.hpp>

#include "complex/UnitTest/UnitTestCommon.hpp"

#include "OrientationAnalysis/Filters/FindFeatureReferenceCAxisMisorientationsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace complex;
using namespace complex::Constants;

namespace
{
const std::string k_FeatRefCAxisMisExemplar = "FeatureReferenceCAxisMisorientations";
const std::string k_FeatRefCAxisMisComputed = "NX_FeatureReferenceCAxisMisorientations";
const std::string k_FeatAvgCAxisMisExemplar = "FeatureAvgCAxisMisorientations";
const std::string k_FeatAvgCAxisMisComputed = "NX_FeatureAvgCAxisMisorientations";
const std::string k_FeatStDevCAxisMisExemplar = "FeatureStdevCAxisMisorientations";
const std::string k_FeatStDevCAxisMisComputed = "NX_FeatureStdevCAxisMisorientations";
const DataPath k_AvgCAxesPath = k_CellFeatureDataPath.createChildPath("AvgCAxes");
} // namespace

TEST_CASE("OrientationAnalysis::FindFeatureReferenceCAxisMisorientationsFilter: Valid Filter Execution", "[OrientationAnalysis][FindFeatureReferenceCAxisMisorientationsFilter]")
{
  const std::string kDataInputArchive = "6_6_caxis_data.tar.gz";
  const std::string kExpectedOutputTopLevel = "6_6_caxis_data";
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, kDataInputArchive, kExpectedOutputTopLevel,
                                                             complex::unit_test::k_BinaryTestOutputDir);

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_caxis_data/6_6_find_caxis_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindFeatureReferenceCAxisMisorientationsFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(FindFeatureReferenceCAxisMisorientationsFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
  args.insertOrAssign(FindFeatureReferenceCAxisMisorientationsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsArrayPath));
  args.insertOrAssign(FindFeatureReferenceCAxisMisorientationsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesArrayPath));
  args.insertOrAssign(FindFeatureReferenceCAxisMisorientationsFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsArrayPath));
  args.insertOrAssign(FindFeatureReferenceCAxisMisorientationsFilter::k_AvgCAxesArrayPath_Key, std::make_any<DataPath>(k_AvgCAxesPath));
  args.insertOrAssign(FindFeatureReferenceCAxisMisorientationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));
  args.insertOrAssign(FindFeatureReferenceCAxisMisorientationsFilter::k_FeatureAvgCAxisMisorientationsArrayName_Key, std::make_any<std::string>(k_FeatAvgCAxisMisComputed));
  args.insertOrAssign(FindFeatureReferenceCAxisMisorientationsFilter::k_FeatureStdevCAxisMisorientationsArrayName_Key, std::make_any<std::string>(k_FeatStDevCAxisMisComputed));
  args.insertOrAssign(FindFeatureReferenceCAxisMisorientationsFilter::k_FeatureReferenceCAxisMisorientationsArrayName_Key, std::make_any<std::string>(k_FeatRefCAxisMisComputed));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  UnitTest::CompareFloatArraysWithNans<float32>(dataStructure, k_CellAttributeMatrix.createChildPath(k_FeatRefCAxisMisExemplar), k_CellAttributeMatrix.createChildPath(k_FeatRefCAxisMisComputed),
                                                UnitTest::EPSILON, false);
  UnitTest::CompareFloatArraysWithNans<float32>(dataStructure, k_CellFeatureDataPath.createChildPath(k_FeatAvgCAxisMisExemplar), k_CellFeatureDataPath.createChildPath(k_FeatAvgCAxisMisComputed),
                                                UnitTest::EPSILON, false);
  UnitTest::CompareFloatArraysWithNans<float32>(dataStructure, k_CellFeatureDataPath.createChildPath(k_FeatStDevCAxisMisExemplar), k_CellFeatureDataPath.createChildPath(k_FeatStDevCAxisMisComputed),
                                                UnitTest::EPSILON, false);
}

TEST_CASE("OrientationAnalysis::FindFeatureReferenceCAxisMisorientationsFilter: InValid Filter Execution", "[OrientationAnalysis][FindFeatureReferenceCAxisMisorientationsFilter]")
{
  const std::string kDataInputArchive = "6_6_caxis_data.tar.gz";
  const std::string kExpectedOutputTopLevel = "6_6_caxis_data";
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, kDataInputArchive, kExpectedOutputTopLevel,
                                                             complex::unit_test::k_BinaryTestOutputDir);

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_caxis_data/6_6_find_caxis_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  auto& crystalStructs = dataStructure.getDataRefAs<UInt32Array>(k_CrystalStructuresArrayPath);
  crystalStructs[1] = 1;

  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindFeatureReferenceCAxisMisorientationsFilter filter;
  Arguments args;

  // Invalid crystal structure type : should fail in execute
  args.insertOrAssign(FindFeatureReferenceCAxisMisorientationsFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
  args.insertOrAssign(FindFeatureReferenceCAxisMisorientationsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsArrayPath));
  args.insertOrAssign(FindFeatureReferenceCAxisMisorientationsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesArrayPath));
  args.insertOrAssign(FindFeatureReferenceCAxisMisorientationsFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsArrayPath));
  args.insertOrAssign(FindFeatureReferenceCAxisMisorientationsFilter::k_AvgCAxesArrayPath_Key, std::make_any<DataPath>(k_AvgCAxesPath));
  args.insertOrAssign(FindFeatureReferenceCAxisMisorientationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));
  args.insertOrAssign(FindFeatureReferenceCAxisMisorientationsFilter::k_FeatureAvgCAxisMisorientationsArrayName_Key, std::make_any<std::string>(k_FeatAvgCAxisMisComputed));
  args.insertOrAssign(FindFeatureReferenceCAxisMisorientationsFilter::k_FeatureStdevCAxisMisorientationsArrayName_Key, std::make_any<std::string>(k_FeatStDevCAxisMisComputed));
  args.insertOrAssign(FindFeatureReferenceCAxisMisorientationsFilter::k_FeatureReferenceCAxisMisorientationsArrayName_Key, std::make_any<std::string>(k_FeatRefCAxisMisComputed));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(executeResult.result)
}
