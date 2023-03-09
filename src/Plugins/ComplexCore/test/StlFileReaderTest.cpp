#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/StlFileReaderFilter.hpp"

#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/Parsing/HDF5/Writers/FileWriter.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <string>

namespace fs = std::filesystem;
using namespace complex;
using namespace complex::Constants;

TEST_CASE("ComplexCore::StlFileReaderFilter", "[ComplexCore][StlFileReaderFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  DataStructure dataStructure;
  Arguments args;
  StlFileReaderFilter filter;

  DataPath triangleGeomDataPath({"[Triangle Geometry]"});

  std::string inputFile = fmt::format("{}/ASTMD638_specimen.stl", unit_test::k_ComplexTestDataSourceDir.view());

  // Create default Parameters for the filter.
  args.insertOrAssign(StlFileReaderFilter::k_StlFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputFile)));
  args.insertOrAssign(StlFileReaderFilter::k_GeometryDataPath_Key, std::make_any<DataPath>(triangleGeomDataPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(triangleGeomDataPath);
  REQUIRE(triangleGeom.getNumberOfFaces() == 92);
  REQUIRE(triangleGeom.getNumberOfVertices() == 48);

  Result<complex::HDF5::FileWriter> result = complex::HDF5::FileWriter::CreateFile(fmt::format("{}/StlFileReaderTest.dream3d", unit_test::k_BinaryTestOutputDir));
  complex::HDF5::FileWriter fileWriter = std::move(result.value());

  auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
  COMPLEX_RESULT_REQUIRE_VALID(resultH5);
}
