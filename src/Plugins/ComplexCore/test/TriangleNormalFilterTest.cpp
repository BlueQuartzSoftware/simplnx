#include <catch2/catch.hpp>

#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/StlFileReaderFilter.hpp"
#include "ComplexCore/Filters/TriangleNormalFilter.hpp"

#include <filesystem>
#include <iostream>
#include <string>
namespace fs = std::filesystem;

using namespace complex;
using namespace complex::Constants;

namespace
{
constexpr float64 k_max_difference = 0.0001;
}

TEST_CASE("ComplexCore::TriangleNormalFilter", "[ComplexCore][TriangleNormalFilter]")
{
  std::string triangleGeometryName = "[Triangle Geometry]";
  std::string triangleFaceDataGroupName = "Face Data";
  std::string normalsDataArrayName = "Normals";

  DataStructure dataGraph;

  {
    Arguments args;
    StlFileReaderFilter filter;

    DataPath triangleGeomDataPath({triangleGeometryName});
    DataPath triangleFaceDataGroupDataPath({triangleGeometryName, triangleFaceDataGroupName});
    DataPath normalsDataPath({triangleGeometryName, triangleFaceDataGroupName, normalsDataArrayName});

    std::string inputFile = fmt::format("{}/ASTMD638_specimen.stl", unit_test::k_ComplexTestDataSourceDir.view());

    // Create default Parameters for the filter.
    args.insertOrAssign(StlFileReaderFilter::k_StlFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputFile)));
    args.insertOrAssign(StlFileReaderFilter::k_GeometryDataPath_Key, std::make_any<DataPath>(triangleGeomDataPath));
    args.insertOrAssign(StlFileReaderFilter::k_FaceGroupDataPath_Key, std::make_any<DataPath>(triangleFaceDataGroupDataPath));
    args.insertOrAssign(StlFileReaderFilter::k_FaceNormalsDataPath_Key, std::make_any<DataPath>(normalsDataPath));

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
    TriangleNormalFilter filter;
    Arguments args;
    std::string triangleNormalsName = "Triangle Normals";

    DataPath geometryPath = DataPath({triangleGeometryName});

    // Create default Parameters for the filter.
    DataPath triangleNormalsDataPath = geometryPath.createChildPath(triangleFaceDataGroupName).createChildPath(triangleNormalsName);
    args.insertOrAssign(TriangleNormalFilter::k_TriGeometryDataPath_Key, std::make_any<DataPath>(geometryPath));
    args.insertOrAssign(TriangleNormalFilter::k_SurfaceMeshTriangleNormalsArrayPath_Key, std::make_any<DataPath>(triangleNormalsDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataGraph, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataGraph, args);
    REQUIRE(executeResult.result.valid());

    // Let's compare the normals.
    DataPath normalsDataPath({triangleGeometryName, triangleFaceDataGroupName, normalsDataArrayName});
    auto& officialNormals = dataGraph.getDataRefAs<Float64Array>(normalsDataPath);
    Float64Array& calculatedNormals = dataGraph.getDataRefAs<Float64Array>(triangleNormalsDataPath);
    std::vector<double> offNorms, calcNorms;
    for(int64 i = 0; i < officialNormals.getSize(); i++)
    {
      auto result = fabs(officialNormals[i] - calculatedNormals[i]);
      REQUIRE(result < ::k_max_difference);
    }
  }

  Result<H5::FileWriter> result = H5::FileWriter::CreateFile(fmt::format("{}/TriangleNormals.dream3d", unit_test::k_BinaryDir));
  H5::FileWriter fileWriter = std::move(result.value());

  herr_t err = dataGraph.writeHdf5(fileWriter);
  REQUIRE(err >= 0);
}
