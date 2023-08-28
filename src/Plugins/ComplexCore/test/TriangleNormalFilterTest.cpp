#include "ComplexCore/Filters/TriangleNormalFilter.hpp"
#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/StlFileReaderFilter.hpp"

#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/Parsing/HDF5/Readers/FileReader.hpp"
#include "complex/Utilities/Parsing/HDF5/Writers/FileWriter.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
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
  std::string normalsDataArrayName = "FaceNormals";

  DataStructure dataStructure;

  {
    Arguments args;
    StlFileReaderFilter filter;

    DataPath triangleGeomDataPath({triangleGeometryName});
    DataPath triangleFaceDataGroupDataPath({triangleGeometryName, triangleFaceDataGroupName});
    DataPath normalsDataPath({triangleGeometryName, triangleFaceDataGroupName, normalsDataArrayName});

    std::string inputFile = fmt::format("{}/ASTMD638_specimen.stl", unit_test::k_ComplexTestDataSourceDir.view());

    // Create default Parameters for the filter.
    args.insertOrAssign(StlFileReaderFilter::k_StlFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputFile)));
    args.insertOrAssign(StlFileReaderFilter::k_TriangleGeometryName_Key, std::make_any<DataPath>(triangleGeomDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

    TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(triangleGeomDataPath);
    REQUIRE(triangleGeom.getNumberOfFaces() == 92);
    REQUIRE(triangleGeom.getNumberOfVertices() == 48);
  }

  {
    TriangleNormalFilter filter;
    Arguments args;
    std::string triangleNormalsName = "Triangle Normals";

    DataPath geometryPath = DataPath({triangleGeometryName});

    // Create default Parameters for the filter.
    args.insertOrAssign(TriangleNormalFilter::k_TriGeometryDataPath_Key, std::make_any<DataPath>(geometryPath));
    args.insertOrAssign(TriangleNormalFilter::k_SurfaceMeshTriangleNormalsArrayPath_Key, std::make_any<std::string>(triangleNormalsName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());

    DataPath triangleNormalsDataPath = geometryPath.createChildPath(triangleFaceDataGroupName).createChildPath(triangleNormalsName);

    // Let's compare the normals.
    DataPath normalsDataPath({triangleGeometryName, triangleFaceDataGroupName, normalsDataArrayName});
    auto& officialNormals = dataStructure.getDataRefAs<Float64Array>(normalsDataPath);
    Float64Array& calculatedNormals = dataStructure.getDataRefAs<Float64Array>(triangleNormalsDataPath);
    std::vector<double> offNorms, calcNorms;
    for(int64 i = 0; i < officialNormals.getSize(); i++)
    {
      auto result = fabs(officialNormals[i] - calculatedNormals[i]);
      REQUIRE(result < ::k_max_difference);
    }
  }

  // Write the DataStructure out to the file system
#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/TriangleNormals.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}
