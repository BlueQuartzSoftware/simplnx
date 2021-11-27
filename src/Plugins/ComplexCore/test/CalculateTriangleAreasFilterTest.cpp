#include <catch2/catch.hpp>

#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include "UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/CalculateTriangleAreasFilter.hpp"
#include "ComplexCore/Filters/StlFileReaderFilter.hpp"

#include <filesystem>
#include <string>
namespace fs = std::filesystem;

using namespace complex;
using namespace complex::UnitTest::Constants;

TEST_CASE("ComplexCore::CalculateTriangleAreasFilter: Instantiation and Parameter Check", "[ComplexCore][CalculateTriangleAreasFilter]")
{
  std::string triangleGeometryName = "[Triangle Geometry]";
  std::string triangleFaceDataGroupName = "Face Data";
  std::string normalsDataArrayName = "Normals";

  DataStructure dataGraph;

  {
    StlFileReaderFilter filter;
    Arguments args;

    DataGroup::Create(dataGraph, k_LevelZero);

    DataPath parentPath = DataPath({k_LevelZero});

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
  }

  {
    CalculateTriangleAreasFilter filter;
    Arguments args;

    DataPath geometryPath = DataPath({k_LevelZero, triangleGeometryName});

    // Create default Parameters for the filter.
    DataPath triangleAreasDataPath = geometryPath.createChildPath(triangleFaceDataGroupName).createChildPath("Triangle Areas");
    args.insertOrAssign(CalculateTriangleAreasFilter::k_TriangleGeometryDataPath_Key, std::make_any<DataPath>(geometryPath));
    args.insertOrAssign(CalculateTriangleAreasFilter::k_CalculatedAreasDataPath_Key, std::make_any<DataPath>(triangleAreasDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataGraph, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataGraph, args);
    REQUIRE(executeResult.result.valid());

    // Let's sum up all the areas.
    Float64Array& faceAreas = dataGraph.getDataRefAs<Float64Array>(triangleAreasDataPath);
    double sumOfAreas = 0.0;
    for(const auto& area : faceAreas)
    {
      sumOfAreas += area;
    }
    REQUIRE(sumOfAreas > 7098.90);
    REQUIRE(sumOfAreas < 7098.94);

    std::cout << "Sum Of Areas: " << sumOfAreas << std::endl;
  }

  Result<H5::FileWriter> result = H5::FileWriter::CreateFile(fmt::format("{}/TriangleAreas.dream3d", unit_test::k_BinaryDir));
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
