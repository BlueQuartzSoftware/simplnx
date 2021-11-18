/**
 * This file is auto generated from the original SimulationIO/Export3dSolidMesh
 * runtime information. These are the steps that need to be taken to utilize this
 * unit test in the proper way.
 *
 * 1: Validate each of the default parameters that gets created.
 * 2: Inspect the actual filter to determine if the filter in its default state
 * would pass or fail BOTH the preflight() and execute() methods
 * 3: UPDATE the ```REQUIRE(result.result.valid());``` code to have the proper
 *
 * 4: Add additional unit tests to actually test each code path within the filter
 *
 * There are some example Catch2 ```TEST_CASE``` sections for your inspiration.
 *
 * NOTE the format of the ```TEST_CASE``` macro. Please stick to this format to
 * allow easier parsing of the unit tests.
 *
 * When you start working on this unit test remove "[Export3dSolidMesh][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

#include "SimulationIO/Filters/Export3dSolidMesh.hpp"
#include "SimulationIO/SimulationIO_test_dirs.hpp"

using namespace complex;

TEST_CASE("SimulationIO::Export3dSolidMesh: Instantiation and Parameter Check", "[SimulationIO][Export3dSolidMesh][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  Export3dSolidMesh filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(Export3dSolidMesh::k_outputPath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path("/Path/To/Output/Directory/To/Read")));
  args.insertOrAssign(Export3dSolidMesh::k_PackageLocation_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path("/Path/To/Output/Directory/To/Read")));
  args.insertOrAssign(Export3dSolidMesh::k_NetgenSTLFileName_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insertOrAssign(Export3dSolidMesh::k_GmshSTLFileName_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insertOrAssign(Export3dSolidMesh::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(Export3dSolidMesh::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(Export3dSolidMesh::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(Export3dSolidMesh::k_FeatureCentroidArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(Export3dSolidMesh::k_MeshFileFormat_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(Export3dSolidMesh::k_RefineMesh_Key, std::make_any<bool>(false));
  args.insertOrAssign(Export3dSolidMesh::k_MaxRadiusEdgeRatio_Key, std::make_any<float32>(1.23345f));
  args.insertOrAssign(Export3dSolidMesh::k_MinDihedralAngle_Key, std::make_any<float32>(1.23345f));
  args.insertOrAssign(Export3dSolidMesh::k_OptimizationLevel_Key, std::make_any<int32>(1234356));
  args.insertOrAssign(Export3dSolidMesh::k_MeshSize_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(Export3dSolidMesh::k_LimitTetrahedraVolume_Key, std::make_any<bool>(false));
  args.insertOrAssign(Export3dSolidMesh::k_MaxTetrahedraVolume_Key, std::make_any<float32>(1.23345f));
  args.insertOrAssign(Export3dSolidMesh::k_IncludeHolesUsingPhaseID_Key, std::make_any<bool>(false));
  args.insertOrAssign(Export3dSolidMesh::k_PhaseID_Key, std::make_any<int32>(1234356));
  args.insertOrAssign(Export3dSolidMesh::k_TetDataContainerName_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insertOrAssign(Export3dSolidMesh::k_VertexAttributeMatrixName_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insertOrAssign(Export3dSolidMesh::k_CellAttributeMatrixName_Key, std::make_any<StringParameter::ValueType>("SomeString"));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("SimulationIO::Export3dSolidMesh: Valid filter execution")
//{
//
//}

// TEST_CASE("SimulationIO::Export3dSolidMesh: InValid filter execution")
//{
//
//}
