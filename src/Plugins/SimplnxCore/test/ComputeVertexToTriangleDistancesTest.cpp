#include "SimplnxCore/Filters/ComputeVertexToTriangleDistancesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <algorithm>
#include <catch2/catch.hpp>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::UnitTest;

namespace
{
constexpr StringLiteral k_DistancesName("Distances");
constexpr StringLiteral k_ClosestTriangleIdsName("ClosestTriangleId");
constexpr StringLiteral k_DistancesNameNX("DistancesNX");
constexpr StringLiteral k_ClosestTriangleIdsNameNX("Closest Triangle Ids");
} // namespace

TEST_CASE("SimplnxCore::ComputeVertexToTriangleDistancesFilter", "[SimplnxCore][ComputeVertexToTriangleDistancesFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_vertex_to_triangle_distances.tar.gz", "6_6_vertex_to_triangle_distances.dream3d");

  // Read the Small IN100 Data set
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/6_6_vertex_to_triangle_distances.dream3d", unit_test::k_TestFilesDir)));
  DataPath triangleData({Constants::k_TriangleDataContainerName});
  DataPath vertexData({Constants::k_VertexDataContainerName});
  DataPath normalsPath({Constants::k_TriangleDataContainerName, Constants::k_FaceData, Constants::k_FaceNormals});
  DataPath distancesPath({Constants::k_VertexDataContainerName, Constants::k_VertexData, k_DistancesName});
  DataPath closestTrianglePath({Constants::k_VertexDataContainerName, Constants::k_VertexData, k_ClosestTriangleIdsName});
  DataPath distancesNXPath({Constants::k_VertexDataContainerName, Constants::k_VertexData, k_DistancesNameNX});
  DataPath closestTriangleNXPath({Constants::k_VertexDataContainerName, Constants::k_VertexData, k_ClosestTriangleIdsNameNX});

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ComputeVertexToTriangleDistancesFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeVertexToTriangleDistancesFilter::k_SelectedVertexGeometryPath_Key, std::make_any<DataPath>(vertexData));
    args.insertOrAssign(ComputeVertexToTriangleDistancesFilter::k_SelectedTriangleGeometryPath_Key, std::make_any<DataPath>(triangleData));
    args.insertOrAssign(ComputeVertexToTriangleDistancesFilter::k_TriangleNormalsArrayPath_Key, std::make_any<DataPath>(normalsPath));
    args.insertOrAssign(ComputeVertexToTriangleDistancesFilter::k_DistancesArrayName_Key, std::make_any<std::string>(k_DistancesNameNX));
    args.insertOrAssign(ComputeVertexToTriangleDistancesFilter::k_ClosestTriangleIdArrayName_Key, std::make_any<std::string>(k_ClosestTriangleIdsNameNX));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Compare Outputs
  {
    UnitTest::CompareArrays<float32>(dataStructure, distancesPath, distancesNXPath);
  }

  {
    UnitTest::CompareArrays<int64>(dataStructure, closestTrianglePath, closestTriangleNXPath);
  }

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/vertex_to_triangle_distances.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeVertexToTriangleDistancesFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeVertexToTriangleDistancesFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeVertexToTriangleDistancesFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeVertexToTriangleDistancesFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ComputeVertexToTriangleDistancesFilter::k_SelectedVertexGeometryPath_Key) == DataPath({"VertexDataContainer"}));
      CHECK(args.value<DataPath>(ComputeVertexToTriangleDistancesFilter::k_SelectedTriangleGeometryPath_Key) == DataPath({"TriangleDataContainer"}));
      CHECK(args.value<DataPath>(ComputeVertexToTriangleDistancesFilter::k_TriangleNormalsArrayPath_Key) == DataPath({"TriangleDataContainer", "FaceData", "Normals"}));
      CHECK(args.value<std::string>(ComputeVertexToTriangleDistancesFilter::k_DistancesArrayName_Key) == "Distances");
      CHECK(args.value<std::string>(ComputeVertexToTriangleDistancesFilter::k_ClosestTriangleIdArrayName_Key) == "ClosestTriangleId");
    }
  }
}

TEST_CASE("SimplnxCore::ComputeVertexToTriangleDistancesFilter: Triangle Normals requires 3 components", "[SimplnxCore][ComputeVertexToTriangleDistancesFilter][ComponentShape]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  auto* triangleGeom = TriangleGeom::Create(dataStructure, "Triangles");
  REQUIRE(triangleGeom != nullptr);
  auto* vertices = UnitTest::CreateTestDataArray<float32>(dataStructure, "Vertices", {3}, {3}, triangleGeom->getId());
  triangleGeom->setVertices(*vertices);
  triangleGeom->setVertexCoordinate(0, {0.0F, 0.0F, 0.0F});
  triangleGeom->setVertexCoordinate(1, {1.0F, 0.0F, 0.0F});
  triangleGeom->setVertexCoordinate(2, {0.0F, 1.0F, 0.0F});
  auto* faces = UnitTest::CreateTestDataArray<IGeometry::MeshIndexType>(dataStructure, "Faces", {1}, {3}, triangleGeom->getId());
  (*faces)[0] = 0;
  (*faces)[1] = 1;
  (*faces)[2] = 2;
  triangleGeom->setFaceList(*faces);
  auto* faceData = AttributeMatrix::Create(dataStructure, "Face Data", {1}, triangleGeom->getId());
  REQUIRE(faceData != nullptr);
  triangleGeom->setFaceAttributeMatrix(*faceData);
  UnitTest::CreateTestDataArray<float64>(dataStructure, "Normals", {1}, {1}, faceData->getId());
  auto* vertexGeom = VertexGeom::Create(dataStructure, "Query Points");
  REQUIRE(vertexGeom != nullptr);
  auto* queryVertices = UnitTest::CreateTestDataArray<float32>(dataStructure, "Vertices", {1}, {3}, vertexGeom->getId());
  vertexGeom->setVertices(*queryVertices);
  vertexGeom->setVertexCoordinate(0, {0.25F, 0.25F, 1.0F});
  auto* vertexData = AttributeMatrix::Create(dataStructure, "Vertex Data", {1}, vertexGeom->getId());
  REQUIRE(vertexData != nullptr);
  vertexGeom->setVertexAttributeMatrix(*vertexData);

  ComputeVertexToTriangleDistancesFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeVertexToTriangleDistancesFilter::k_SelectedVertexGeometryPath_Key, std::make_any<DataPath>(DataPath({"Query Points"})));
  args.insertOrAssign(ComputeVertexToTriangleDistancesFilter::k_SelectedTriangleGeometryPath_Key, std::make_any<DataPath>(DataPath({"Triangles"})));
  args.insertOrAssign(ComputeVertexToTriangleDistancesFilter::k_TriangleNormalsArrayPath_Key, std::make_any<DataPath>(DataPath({"Triangles", "Face Data", "Normals"})));
  args.insertOrAssign(ComputeVertexToTriangleDistancesFilter::k_DistancesArrayName_Key, std::make_any<std::string>("Distances"));
  args.insertOrAssign(ComputeVertexToTriangleDistancesFilter::k_ClosestTriangleIdArrayName_Key, std::make_any<std::string>("Closest Triangle Ids"));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(std::any_of(preflightResult.outputActions.errors().begin(), preflightResult.outputActions.errors().end(), [](const Error& error) { return error.code == -208; }));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
