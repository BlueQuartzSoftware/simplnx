#include "SimplnxCore/Filters/TriangleNormalFilter.hpp"
#include "SimplnxCore/Filters/ReadStlFileFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Parsing/HDF5/Readers/FileReader.hpp"
#include "simplnx/Utilities/Parsing/HDF5/Writers/FileWriter.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <string>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
constexpr float64 k_max_difference = 0.0001;
}

TEST_CASE("SimplnxCore::TriangleNormalFilter", "[SimplnxCore][TriangleNormalFilter]")
{
  std::string triangleGeometryName = "[Triangle Geometry]";
  std::string triangleFaceDataGroupName = "Face Data";
  std::string normalsDataArrayName = "Face Normals";

  DataStructure dataStructure;

  {
    Arguments args;
    ReadStlFileFilter filter;

    DataPath triangleGeomDataPath({triangleGeometryName});
    DataPath triangleFaceDataGroupDataPath({triangleGeometryName, triangleFaceDataGroupName});
    DataPath normalsDataPath({triangleGeometryName, triangleFaceDataGroupName, normalsDataArrayName});

    std::string inputFile = fmt::format("{}/ASTMD638_specimen.stl", unit_test::k_ComplexTestDataSourceDir.view());

    // Create default Parameters for the filter.
    args.insertOrAssign(ReadStlFileFilter::k_StlFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputFile)));
    args.insertOrAssign(ReadStlFileFilter::k_TriangleGeometryName_Key, std::make_any<DataPath>(triangleGeomDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(triangleGeomDataPath);
    REQUIRE(triangleGeom.getNumberOfFaces() == 92);
    REQUIRE(triangleGeom.getNumberOfVertices() == 48);
  }

  {
    TriangleNormalFilter filter;
    Arguments args;
    std::string triangleNormalsName = "Face Normals (computed)";

    DataPath geometryPath = DataPath({triangleGeometryName});

    // Create default Parameters for the filter.
    args.insertOrAssign(TriangleNormalFilter::k_TriGeometryDataPath_Key, std::make_any<DataPath>(geometryPath));
    args.insertOrAssign(TriangleNormalFilter::k_SurfaceMeshTriangleNormalsArrayPath_Key, std::make_any<std::string>(triangleNormalsName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

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
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/TriangleNormals.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}
