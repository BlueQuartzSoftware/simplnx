
/**
 * This file is auto generated from the original ImportExport/ReadStlFile
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
 * When you start working on this unit test remove "[ReadStlFile][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Utilities/Parsing/HDF5/H5.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"

#include "UnitTestCommon.hpp"

#include "ComplexCore/Filters/StlFileReaderFilter.hpp"
#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include <filesystem>
#include <string>
namespace fs = std::filesystem;

using namespace complex;
using namespace complex::UnitTest::Constants;

TEST_CASE("ComplexCore::StlFileReaderFilter: Instantiation and Parameter Check", "[ComplexCore][StlFileReaderFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  StlFileReaderFilter filter;
  DataStructure dataGraph;
  Arguments args;

  DataGroup* topLevelGroup = DataGroup::Create(dataGraph, k_LevelZero);

  DataPath parentPath = DataPath({k_LevelZero});
  std::string triangleGeometryName = "[Triangle Geometry]";
  std::string triangleFaceDataGroupName = "Face Data";
  std::string normalsDataArrayName = "Normals";

  DataPath normalsDataPath = parentPath.createChildPath(triangleGeometryName).createChildPath(triangleFaceDataGroupName).createChildPath(normalsDataArrayName);

  std::string inputFile = fmt::format("{}/ASTMD638_specimen.stl", unit_test::k_ComplexTestDataSourceDir.view());


  // Create default Parameters for the filter.
  args.insertOrAssign(StlFileReaderFilter::k_StlFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputFile)));
  args.insertOrAssign(StlFileReaderFilter::k_ParentDataGroupPath_Key, std::make_any<DataPath>(parentPath));
  args.insertOrAssign(StlFileReaderFilter::k_GeometryName_Key, std::make_any<std::string>(triangleGeometryName));
  args.insertOrAssign(StlFileReaderFilter::k_FaceDataGroupName_Key, std::make_any<std::string>(triangleFaceDataGroupName));
  args.insertOrAssign(StlFileReaderFilter::k_FaceNormalsArrayName_Key, std::make_any<DataPath>(normalsDataPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(executeResult.result.valid());

  TriangleGeom& triangleGeom = dataGraph.getDataRefAs<TriangleGeom>(parentPath.createChildPath(triangleGeometryName));
  REQUIRE(triangleGeom.getNumberOfFaces() == 92);
  REQUIRE(triangleGeom.getNumberOfVertices() == 48);

  Result<H5::FileWriter> result = H5::FileWriter::CreateFile("/tmp/out.dream3d");
  H5::FileWriter fileWriter = std::move(result.value());

  herr_t err = dataGraph.writeHdf5(fileWriter);
  REQUIRE(err >= 0);
}

// TEST_CASE("ImportExport::ReadStlFile: Valid filter execution")
//{
//
//}

// TEST_CASE("ImportExport::ReadStlFile: InValid filter execution")
//{
//
//}
