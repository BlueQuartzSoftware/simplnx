#include <catch2/catch.hpp>

#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include "UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/StlFileReaderFilter.hpp"

#include <filesystem>
#include <string>
namespace fs = std::filesystem;

using namespace complex;
using namespace complex::Constants;

TEST_CASE("ComplexCore::StlFileReaderFilter", "[ComplexCore][StlFileReaderFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  StlFileReaderFilter filter;
  DataStructure dataGraph;
  Arguments args;

  DataGroup::Create(dataGraph, k_LevelZero);

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

  Result<H5::FileWriter> result = H5::FileWriter::CreateFile(fmt::format("{}/StlFileReaderTest.dream3d", unit_test::k_BinaryDir));
  H5::FileWriter fileWriter = std::move(result.value());

  herr_t err = dataGraph.writeHdf5(fileWriter);
  REQUIRE(err >= 0);
}
