#include "SimplnxCore/Filters/ApproximatePointCloudHull.hpp"
#include "SimplnxCore/Filters/ReadStlFileFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Filter/Actions/CreateVertexGeometryAction.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Utilities/Parsing/HDF5/Writers/FileWriter.hpp"

#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <string>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
namespace
{
static const std::vector<float> s_Vertices = {
    0.0F,      0.0F,       -8.305164E-7F,  40.35763F, 0.0F,       -8.305164E-7F,  139.6424F, 0.0F,       -8.305164E-7F,  180.0F,    0.0F,       -8.305164E-7F,
    45.55068F, 1.3076F,    -7.733594E-7F,  134.4493F, 1.3076F,    -7.733594E-7F,  50.82293F, 2.2462597F, -7.323292E-7F,  56.14818F, 2.8113308F, -7.076292E-7F,
    123.8518F, 2.8113308F, -7.076292E-7F,  129.1771F, 2.2462597F, -7.323292E-7F,  61.5F,     3.0F,       -6.993822E-7F,  118.5F,    3.0F,       -6.993822E-7F,
    50.82293F, 16.753738F, -9.8187236E-8F, 56.14818F, 16.18867F,  -1.2288719E-7F, 61.5F,     16.0F,      -1.3113416E-7F, 118.5F,    16.0F,      -1.3113416E-7F,
    123.8518F, 16.18867F,  -1.2288719E-7F, 129.1771F, 16.753738F, -9.8187236E-8F, 45.55068F, 17.692402F, -5.7156925E-8F, 134.4493F, 17.692402F, -5.7156925E-8F,
    0.0F,      19.0F,      0.0F,           40.35763F, 19.0F,      0.0F,           139.6424F, 19.0F,      0.0F,           180.0F,    19.0F,      0.0F,
    0.0F,      0.0F,       3.1999993F,     40.35763F, 0.0F,       3.1999993F,     139.6424F, 0.0F,       3.1999993F,     180.0F,    0.0F,       3.1999993F,
    45.55068F, 1.3076F,    3.1999993F,     134.4493F, 1.3076F,    3.1999993F,     50.82293F, 2.2462597F, 3.1999993F,     56.14818F, 2.8113308F, 3.1999993F,
    123.8518F, 2.8113308F, 3.1999993F,     129.1771F, 2.2462597F, 3.1999993F,     61.5F,     3.0F,       3.1999993F,     118.5F,    3.0F,       3.1999993F,
    50.82293F, 16.753738F, 3.2F,           56.14818F, 16.18867F,  3.1999998F,     61.5F,     16.0F,      3.1999998F,     118.5F,    16.0F,      3.1999998F,
    123.8518F, 16.18867F,  3.1999998F,     129.1771F, 16.753738F, 3.2F,           45.55068F, 17.692402F, 3.2F,           134.4493F, 17.692402F, 3.2F,
    0.0F,      19.0F,      3.2F,           40.35763F, 19.0F,      3.2F,           139.6424F, 19.0F,      3.2F,           180.0F,    19.0F,      3.2};
}

TEST_CASE("SimplnxCore::ApproximatePointCloudHull: Instantiate Filter", "[ApproximatePointCloudHull]")
{
  std::string triangleGeometryName = "[Triangle Geometry]";
  std::string triangleFaceDataGroupName = "FaceData";
  std::string normalsDataArrayName = "FaceNormals";
  DataPath hullVertexGeomPath({"[Point Cloud Hull]"});

  DataStructure dataStructure;
  DataPath triangleGeomDataPath({triangleGeometryName});
  DataPath triangleFaceDataGroupDataPath({triangleGeometryName, triangleFaceDataGroupName});
  DataPath normalsDataPath({triangleGeometryName, triangleFaceDataGroupName, normalsDataArrayName});

  {
    Arguments args;
    ReadStlFileFilter filter;
    std::string inputFile = fmt::format("{}/ASTMD638_specimen.stl", unit_test::k_ComplexTestDataSourceDir.view());

    // Create default Parameters for the filter.
    args.insertOrAssign(ReadStlFileFilter::k_StlFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputFile)));
    args.insertOrAssign(ReadStlFileFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(triangleGeomDataPath));

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
    // Create a Vertex Geometry from the vertices from the STL file
    TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(triangleGeomDataPath);
    DataArray<float>* triangleVertices = triangleGeom.getVertices();

    DataPath vertexGeomPath({"[Vertex Geometry]"});
    CreateVertexGeometryAction createVertexGeometryAction(vertexGeomPath, triangleVertices->getNumberOfTuples(), INodeGeometry0D::k_VertexDataName, CreateVertexGeometryAction::k_SharedVertexListName);
    Result<> createVertexResult = createVertexGeometryAction.apply(dataStructure, IDataAction::Mode::Execute);
    REQUIRE(createVertexResult.valid());

    // Copy the vertices into the new Vertex Geometry
    VertexGeom& vertexGeom = dataStructure.getDataRefAs<VertexGeom>(vertexGeomPath);
    Float32Array* vertices = vertexGeom.getVertices();
    for(size_t eIdx = 0; eIdx < triangleVertices->getSize(); eIdx++)
    {
      (*vertices)[eIdx] = (*triangleVertices)[eIdx];
    }

    ApproximatePointCloudHull filter;
    Arguments args;

    std::vector<float32> gridResolution = {1.0F, 1.0F, 1.0F};
    uint64 minEmptyNeighbors = 1;

    args.insertOrAssign(ApproximatePointCloudHull::k_GridResolution_Key, std::make_any<std::vector<float32>>(gridResolution));
    args.insertOrAssign(ApproximatePointCloudHull::k_MinEmptyNeighbors_Key, std::make_any<uint64>(minEmptyNeighbors));
    args.insertOrAssign(ApproximatePointCloudHull::k_VertexGeomPath_Key, std::make_any<DataPath>(vertexGeomPath));
    args.insertOrAssign(ApproximatePointCloudHull::k_HullVertexGeomPath_Key, std::make_any<DataPath>(hullVertexGeomPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }
  // Validate the output data
  {
    VertexGeom& vertexGeom1 = dataStructure.getDataRefAs<VertexGeom>(hullVertexGeomPath);
    REQUIRE(vertexGeom1.getNumberOfVertices() == 48);
    Float32Array* vertices = vertexGeom1.getVertices();
    bool verticesEqual = true;
    for(size_t eIdx = 0; eIdx < vertices->getSize(); eIdx++)
    {
      if((*vertices)[eIdx] != s_Vertices[eIdx])
      {
        verticesEqual = false;
        break;
      }
    }
    REQUIRE(verticesEqual);
  }

  // Write out the DataStructure
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/ApproximatePointCloudHull.dream3d", unit_test::k_BinaryTestOutputDir));
#endif
}
