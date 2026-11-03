#include "SimplnxCore/Filters/ApproximatePointCloudHullFilter.hpp"
#include "SimplnxCore/Filters/ReadStlFileFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Filter/Actions/CreateVertexGeometryAction.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

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

TEST_CASE("SimplnxCore::ApproximatePointCloudHullFilter: Instantiate Filter", "[ApproximatePointCloudHullFilter]")
{
  UnitTest::LoadPlugins();

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
    std::string inputFile = fmt::format("{}/ASTMD638_specimen.stl", unit_test::k_SimplnxTestDataSourceDir.view());

    // Create default Parameters for the filter.
    args.insertOrAssign(ReadStlFileFilter::k_StlFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputFile)));
    args.insertOrAssign(ReadStlFileFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(triangleGeomDataPath));

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
    // Create a Vertex Geometry from the vertices from the STL file
    TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(triangleGeomDataPath);
    DataArray<float>* triangleVertices = triangleGeom.getVertices();

    DataPath vertexGeomPath({"[Vertex Geometry]"});
    CreateVertexGeometryAction createVertexGeometryAction(vertexGeomPath, triangleVertices->getNumberOfTuples(), INodeGeometry0D::k_VertexAttributeMatrixName, VertexGeom::k_SharedVertexListName);
    Result<> createVertexResult = createVertexGeometryAction.apply(dataStructure, IDataAction::Mode::Execute);
    REQUIRE(createVertexResult.valid());

    // Copy the vertices into the new Vertex Geometry
    VertexGeom& vertexGeom = dataStructure.getDataRefAs<VertexGeom>(vertexGeomPath);
    Float32Array* vertices = vertexGeom.getVertices();
    for(size_t eIdx = 0; eIdx < triangleVertices->getSize(); eIdx++)
    {
      (*vertices)[eIdx] = (*triangleVertices)[eIdx];
    }

    ApproximatePointCloudHullFilter filter;
    Arguments args;

    std::vector<float32> gridResolution = {1.0F, 1.0F, 1.0F};
    uint64 minEmptyNeighbors = 1;

    args.insertOrAssign(ApproximatePointCloudHullFilter::k_GridResolution_Key, std::make_any<std::vector<float32>>(gridResolution));
    args.insertOrAssign(ApproximatePointCloudHullFilter::k_MinEmptyNeighbors_Key, std::make_any<uint64>(minEmptyNeighbors));
    args.insertOrAssign(ApproximatePointCloudHullFilter::k_VertexGeomPath_Key, std::make_any<DataPath>(vertexGeomPath));
    args.insertOrAssign(ApproximatePointCloudHullFilter::k_HullVertexGeomPath_Key, std::make_any<DataPath>(hullVertexGeomPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
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
  WriteTestDataStructure(dataStructure, fmt::format("{}/ApproximatePointCloudHullFilter.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ApproximatePointCloudHullFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ApproximatePointCloudHullFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ApproximatePointCloudHullFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<ApproximatePointCloudHullFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<uint64>(ApproximatePointCloudHullFilter::k_MinEmptyNeighbors_Key) == 5);
      CHECK(args.value<DataPath>(ApproximatePointCloudHullFilter::k_VertexGeomPath_Key) == DataPath({"VertexDataContainer"}));
      CHECK(args.value<DataPath>(ApproximatePointCloudHullFilter::k_HullVertexGeomPath_Key) == DataPath({"HullDataContainer"}));
    }
  }
}

namespace
{
// Runs the filter on a solid 3x3x3 block of voxels with one point per voxel and checks the hull.
// The points are written in z, y, x order, which is also the order of the voxel loop in the filter.
// Oracle: 27 - 1 interior = 26 surface voxels. With an empty border on every side, each surface voxel has at least
// 9 empty neighbors (more than the threshold of 1); the center voxel (point 13) has none. One point per voxel, so each
// hull point equals its input point.
void RunSolidBlockHullTest(float32 resolution, const std::array<float32, 3>& xs, const std::array<float32, 3>& ys, const std::array<float32, 3>& zs)
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  const DataPath inputPath({"Points"});
  const DataPath outputPath({"Hull"});
  CreateVertexGeometryAction action(inputPath, 27, INodeGeometry0D::k_VertexAttributeMatrixName, VertexGeom::k_SharedVertexListName);
  auto createResult = action.apply(dataStructure, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(createResult);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<VertexGeom>(inputPath));
  auto& points = *dataStructure.getDataRefAs<VertexGeom>(inputPath).getVertices();
  usize index = 0;
  for(float32 z : zs)
  {
    for(float32 y : ys)
    {
      for(float32 x : xs)
      {
        points[index++] = x;
        points[index++] = y;
        points[index++] = z;
      }
    }
  }

  ApproximatePointCloudHullFilter filter;
  Arguments args;
  args.insertOrAssign(ApproximatePointCloudHullFilter::k_GridResolution_Key, std::make_any<std::vector<float32>>(std::vector<float32>{resolution, resolution, resolution}));
  args.insertOrAssign(ApproximatePointCloudHullFilter::k_MinEmptyNeighbors_Key, std::make_any<uint64>(1));
  args.insertOrAssign(ApproximatePointCloudHullFilter::k_VertexGeomPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(ApproximatePointCloudHullFilter::k_HullVertexGeomPath_Key, std::make_any<DataPath>(outputPath));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<VertexGeom>(outputPath));
  const auto& hull = dataStructure.getDataRefAs<VertexGeom>(outputPath);
  REQUIRE(hull.getNumberOfVertices() == 26);
  const auto& hullPoints = *hull.getVertices();
  usize outputIndex = 0;
  for(usize point = 0; point < 27; point++)
  {
    if(point == 13) // The center voxel is the only interior voxel.
    {
      continue;
    }
    for(usize component = 0; component < 3; component++)
    {
      REQUIRE(hullPoints[outputIndex++] == points[3 * point + component]);
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
} // namespace

TEST_CASE("SimplnxCore::ApproximatePointCloudHullFilter: One voxel border", "[SimplnxCore][ApproximatePointCloudHullFilter]")
{
  // Voxel size 2. The points 1, 3 and 5 on each axis are in voxels 0, 1 and 2, so they fill the whole block.
  RunSolidBlockHullTest(2.0F, {1.0F, 3.0F, 5.0F}, {1.0F, 3.0F, 5.0F}, {1.0F, 3.0F, 5.0F});
}

TEST_CASE("SimplnxCore::ApproximatePointCloudHullFilter: One voxel border on voxel boundary", "[SimplnxCore][ApproximatePointCloudHullFilter]")
{
  // Voxel size 0.01. The smallest X (-0.6) is on a voxel boundary. As a float it is slightly below -0.6, so it is in
  // voxel -61. X -0.595 and -0.585 are in voxels -60 and -59. Y and Z are the centers of voxels 0, 1 and 2.
  // Without the border on the X-min side, the face-center voxel of that face has no empty neighbor and drops out.
  RunSolidBlockHullTest(0.01F, {-0.6F, -0.595F, -0.585F}, {0.005F, 0.015F, 0.025F}, {0.005F, 0.015F, 0.025F});
}

TEST_CASE("SimplnxCore::ApproximatePointCloudHullFilter: Border at large global indices", "[SimplnxCore][ApproximatePointCloudHullFilter]")
{
  UnitTest::LoadPlugins();
  for(float32 coordinate : {16777218.0F, 33554432.0F, -16777218.0F, -33554432.0F})
  {
    CAPTURE(coordinate);
    DataStructure dataStructure;
    const DataPath inputPath({"Points"});
    const DataPath outputPath({"Hull"});
    CreateVertexGeometryAction action(inputPath, 1, INodeGeometry0D::k_VertexAttributeMatrixName, VertexGeom::k_SharedVertexListName);
    auto createResult = action.apply(dataStructure, IDataAction::Mode::Execute);
    SIMPLNX_RESULT_REQUIRE_VALID(createResult);
    auto& points = *dataStructure.getDataRefAs<VertexGeom>(inputPath).getVertices();
    // Each axis has a large global index, but the padded grid is only 3x3x3.
    for(usize component = 0; component < 3; component++)
    {
      points[component] = coordinate;
    }
    ApproximatePointCloudHullFilter filter;
    Arguments args;
    args.insertOrAssign(ApproximatePointCloudHullFilter::k_GridResolution_Key, std::make_any<std::vector<float32>>(std::vector<float32>{1.0F, 1.0F, 1.0F}));
    args.insertOrAssign(ApproximatePointCloudHullFilter::k_MinEmptyNeighbors_Key, std::make_any<uint64>(25));
    args.insertOrAssign(ApproximatePointCloudHullFilter::k_VertexGeomPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(ApproximatePointCloudHullFilter::k_HullVertexGeomPath_Key, std::make_any<DataPath>(outputPath));
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    const auto& hull = dataStructure.getDataRefAs<VertexGeom>(outputPath);
    // A singleton has 26 empty neighbors; the strict >25 threshold must retain it.
    REQUIRE(hull.getNumberOfVertices() == 1);
    for(usize component = 0; component < 3; component++)
    {
      REQUIRE((*hull.getVertices())[component] == coordinate);
    }
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}
