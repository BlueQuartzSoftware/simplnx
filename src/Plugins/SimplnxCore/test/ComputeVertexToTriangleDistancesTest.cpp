#include "SimplnxCore/Filters/ComputeVertexToTriangleDistancesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

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
