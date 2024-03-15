#include "SimplnxCore/Filters/CalculateTriangleAreasFilter.hpp"
#include "SimplnxCore/Filters/ReadStlFileFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Parsing/HDF5/Writers/FileWriter.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <string>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;

TEST_CASE("SimplnxCore::CalculateTriangleAreasFilter", "[SimplnxCore][CalculateTriangleAreasFilter]")
{
  std::string triangleGeometryName = "[Triangle Geometry]";
  std::string triangleFaceDataGroupName = INodeGeometry2D::k_FaceDataName;
  std::string normalsDataArrayName = "FaceNormals";

  DataStructure dataStructure;

  {
    Arguments args;
    ReadStlFileFilter filter;

    DataPath triangleGeomDataPath({triangleGeometryName});

    std::string inputFile = fmt::format("{}/ASTMD638_specimen.stl", unit_test::k_ComplexTestDataSourceDir.view());

    // Create default Parameters for the filter.
    args.insertOrAssign(ReadStlFileFilter::k_StlFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputFile)));
    args.insertOrAssign(ReadStlFileFilter::k_TriangleGeometryName_Key, std::make_any<DataPath>(triangleGeomDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());

    TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(triangleGeomDataPath);
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
    args.insertOrAssign(CalculateTriangleAreasFilter::k_CalculatedAreasDataName_Key, std::make_any<std::string>(triangleAreasName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());

    auto& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(geometryPath);
    DataPath triangleAreasDataPath = geometryPath.createChildPath(triangleGeom.getFaceAttributeMatrix()->getName()).createChildPath(triangleAreasName);

    // Let's sum up all the areas.
    Float64Array& faceAreas = dataStructure.getDataRefAs<Float64Array>(triangleAreasDataPath);
    double sumOfAreas = 0.0;
    for(const auto& area : faceAreas)
    {
      sumOfAreas += area;
    }
    REQUIRE(sumOfAreas > 7098.90);
    REQUIRE(sumOfAreas < 7098.94);
  }

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/calculate_triangle_areas.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}
