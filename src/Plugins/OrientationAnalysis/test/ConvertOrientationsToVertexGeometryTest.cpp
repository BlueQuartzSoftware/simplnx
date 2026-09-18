
#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/Algorithms/ConvertOrientations.hpp"
#include "OrientationAnalysis/Filters/ConvertOrientationsToVertexGeometryFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

using namespace nx::core::UnitTest;

namespace convert_orientation_to_vertex_geom
{
const DataPath k_InputOrientationsPath({"DataContainer", "CellData", "EulerAngles"});
const DataPath k_InputCrystalStructuresPath({"DataContainer", "All_Laue_Classes", "CrystalStructures"});

} // namespace convert_orientation_to_vertex_geom

TEST_CASE("OrientationAnalysis::ConvertOrientationsToVertexGeometry", "[OrientationAnalysis][ConvertOrientationsToVertexGeometry]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "convert_orientations_to_vertex_geometry.tar.gz", "convert_orientations_to_vertex_geometry");
  auto baseDataFilePath = fs::path(fmt::format("{}/convert_orientations_to_vertex_geometry/convert_orientations_to_vertex_geometry.dream3d", unit_test::k_TestFilesDir));

  const std::vector<std::string> k_PhaseNames = {"Laue_1", "Laue_2", "Laue_222", "Laue_23", "Laue_3", "Laue_32", "Laue_4", "Laue_422", "Laue_432", "Laue_6", "Laue_622"};

  for(const auto& phaseName : k_PhaseNames)
  {
    SECTION(phaseName)
    {
      // Read the modified Small IN100 Data set
      DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

      // Instantiate the filter, a DataStructure object and an Arguments Object
      ConvertOrientationsToVertexGeometryFilter filter;
      Arguments args;

      const DataPath k_InputPhasesPath({"DataContainer", "CellData", phaseName});

      const DataPath k_OutputGeometry({phaseName});

      args.insertOrAssign(ConvertOrientationsToVertexGeometryFilter::k_InputType_Key, std::make_any<ChoicesParameter::ValueType>(0));
      args.insertOrAssign(ConvertOrientationsToVertexGeometryFilter::k_InputOrientationArrayPath_Key, std::make_any<DataPath>(convert_orientation_to_vertex_geom::k_InputOrientationsPath));
      args.insertOrAssign(ConvertOrientationsToVertexGeometryFilter::k_ConvertToFundamentalZone_Key, std::make_any<bool>(true));

      args.insertOrAssign(ConvertOrientationsToVertexGeometryFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_InputPhasesPath));
      args.insertOrAssign(ConvertOrientationsToVertexGeometryFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(convert_orientation_to_vertex_geom::k_InputCrystalStructuresPath));
      args.insertOrAssign(ConvertOrientationsToVertexGeometryFilter::k_CopyVertexPaths_Key,
                          std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{k_InputPhasesPath}));

      args.insertOrAssign(ConvertOrientationsToVertexGeometryFilter::k_VertexGeometryPath_Key, std::make_any<DataPath>(k_OutputGeometry));
      // args.insertOrAssign(ConvertOrientationsToVertexGeometryFilter::k_VertexAttrMatrixName_Key, std::make_any<std::string>(""));
      // args.insertOrAssign(ConvertOrientationsToVertexGeometryFilter::k_SharedVertexListName_Key, std::make_any<std::string>(""));

      // Preflight the filter and check result
      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
      UnitTest::RequireAutomaticCreateArrayActions(preflightResult.outputActions, 1);
      UnitTest::RequireAutomaticCreateArrayActions(preflightResult.outputActions,
                                                   std::vector<DataPath>{k_OutputGeometry.createChildPath(VertexGeom::k_VertexAttributeMatrixName).createChildPath(k_InputPhasesPath.getTargetName())});

      // Execute the filter and check the result
      auto executeResult = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

      // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
      WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/convert_orientations_to_vertex_geometry_{}.dream3d", unit_test::k_BinaryTestOutputDir, phaseName)));
#endif

      const DataPath k_ExemplarGeometry({fmt::format("{} Data", phaseName)});
      UnitTest::CompareFloatArraysWithNans<float32>(dataStructure, k_OutputGeometry.createChildPath("Shared Vertex List"), k_ExemplarGeometry.createChildPath("Shared Vertex List"), 5.0E-7f, false);
    }
  }
}

TEST_CASE("OrientationAnalysis::ConvertOrientationsToVertexGeometry: Phase Index Bounds", "[OrientationAnalysis][ConvertOrientationsToVertexGeometry]")
{
  UnitTest::LoadPlugins();

  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceOutOfCore, 1);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "convert_orientations_to_vertex_geometry.tar.gz", "convert_orientations_to_vertex_geometry");
  const fs::path inputFile = fs::path(unit_test::k_TestFilesDir.view()) / "convert_orientations_to_vertex_geometry" / "convert_orientations_to_vertex_geometry.dream3d";
  DataStructure dataStructure = UnitTest::LoadDataStructure(inputFile);
  const DataPath cellPhasesPath({"DataContainer", "CellData", "Laue_1"});

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(cellPhasesPath));
  auto& cellPhasesArrayRef = dataStructure.getDataRefAs<Int32Array>(cellPhasesPath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32Array>(convert_orientation_to_vertex_geom::k_InputCrystalStructuresPath));
  const auto& crystalStructuresArrayRef = dataStructure.getDataRefAs<UInt32Array>(convert_orientation_to_vertex_geom::k_InputCrystalStructuresPath);
  if(Application::Instance()->getIOManager("HDF5-OOC") != nullptr)
  {
    REQUIRE(cellPhasesArrayRef.getDataStoreRef().getDataFormat() == "HDF5-OOC");
  }

  ConvertOrientationsToVertexGeometryFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ConvertOrientationsToVertexGeometryFilter::k_InputType_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(ConvertOrientationsToVertexGeometryFilter::k_InputOrientationArrayPath_Key, std::make_any<DataPath>(convert_orientation_to_vertex_geom::k_InputOrientationsPath));
  args.insertOrAssign(ConvertOrientationsToVertexGeometryFilter::k_ConvertToFundamentalZone_Key, std::make_any<bool>(true));
  args.insertOrAssign(ConvertOrientationsToVertexGeometryFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(cellPhasesPath));
  args.insertOrAssign(ConvertOrientationsToVertexGeometryFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(convert_orientation_to_vertex_geom::k_InputCrystalStructuresPath));
  args.insertOrAssign(ConvertOrientationsToVertexGeometryFilter::k_VertexGeometryPath_Key, std::make_any<DataPath>(DataPath({"Bounds Vertices"})));

  SECTION("Negative Phase returns an error")
  {
    cellPhasesArrayRef.getDataStoreRef()[0] = -1;
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == -1005);
  }

  SECTION("Phase missing from Crystal Structures returns an error")
  {
    cellPhasesArrayRef.getDataStoreRef()[0] = static_cast<int32>(crystalStructuresArrayRef.getNumberOfTuples());
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == -1005);
  }

  SECTION("Phase is ignored without fundamental-zone conversion")
  {
    cellPhasesArrayRef.getDataStoreRef()[0] = -1;
    args.insertOrAssign(ConvertOrientationsToVertexGeometryFilter::k_ConvertToFundamentalZone_Key, std::make_any<bool>(false));
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }
}
