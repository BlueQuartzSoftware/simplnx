#include <catch2/catch.hpp>

#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/CalculateTriangleAreasFilter.hpp"
#include "ComplexCore/Filters/StlFileReaderFilter.hpp"

#include <filesystem>
#include <string>
namespace fs = std::filesystem;

using namespace complex;
using namespace complex::Constants;

TEST_CASE("ComplexCore::CalculateTriangleAreasFilter", "[ComplexCore][CalculateTriangleAreasFilter]")
{
  std::string triangleGeometryName = "[Triangle Geometry]";
  std::string triangleFaceDataGroupName = INodeGeometry2D::k_FaceDataName;
  std::string normalsDataArrayName = "FaceNormals";

  DataStructure dataGraph;

  {
    Arguments args;
    StlFileReaderFilter filter;

    DataPath triangleGeomDataPath({triangleGeometryName});

    std::string inputFile = fmt::format("{}/ASTMD638_specimen.stl", unit_test::k_ComplexTestDataSourceDir.view());

    // Create default Parameters for the filter.
    args.insertOrAssign(StlFileReaderFilter::k_StlFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputFile)));
    args.insertOrAssign(StlFileReaderFilter::k_GeometryDataPath_Key, std::make_any<DataPath>(triangleGeomDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataGraph, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataGraph, args);
    REQUIRE(executeResult.result.valid());

    TriangleGeom& triangleGeom = dataGraph.getDataRefAs<TriangleGeom>(triangleGeomDataPath);
    REQUIRE(triangleGeom.getNumberOfFaces() == 92);
    REQUIRE(triangleGeom.getNumberOfVertices() == 48);
  }

  {
    CalculateTriangleAreasFilter filter;
    Arguments args;
    std::string triangleAreasName = "Triangle Areas";

    DataPath geometryPath = DataPath({triangleGeometryName});

    // Create default Parameters for the filter.
    args.insertOrAssign(CalculateTriangleAreasFilter::k_TriangleGeometryDataPath_Key, std::make_any<DataPath>(geometryPath));
    args.insertOrAssign(CalculateTriangleAreasFilter::k_CalculatedAreasDataPath_Key, std::make_any<std::string>(triangleAreasName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataGraph, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataGraph, args);
    REQUIRE(executeResult.result.valid());

    auto& triangleGeom = dataGraph.getDataRefAs<TriangleGeom>(geometryPath);
    DataPath triangleAreasDataPath = geometryPath.createChildPath(triangleGeom.getFaceData()->getName()).createChildPath(triangleAreasName);

    // Let's sum up all the areas.
    Float64Array& faceAreas = dataGraph.getDataRefAs<Float64Array>(triangleAreasDataPath);
    double sumOfAreas = 0.0;
    for(const auto& area : faceAreas)
    {
      sumOfAreas += area;
    }
    REQUIRE(sumOfAreas > 7098.90);
    REQUIRE(sumOfAreas < 7098.94);
  }

  Result<H5::FileWriter> result = H5::FileWriter::CreateFile(fmt::format("{}/TriangleAreas.dream3d", unit_test::k_BinaryDir));
  H5::FileWriter fileWriter = std::move(result.value());

  auto resultH5 = dataGraph.writeHdf5(fileWriter);
  REQUIRE(resultH5.valid());
}

// TEST_CASE("ImportExport::ReadStlFile: Valid filter execution")
//{
//
//}

// TEST_CASE("ImportExport::ReadStlFile: InValid filter execution")
//{
//
//}
