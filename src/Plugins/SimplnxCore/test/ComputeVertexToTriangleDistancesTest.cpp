#include "SimplnxCore/Filters/ComputeVertexToTriangleDistancesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

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
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_vertex_to_triangle_distances.tar.gz",
                                                              "6_6_vertex_to_triangle_distances.dream3d");

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
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
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
}
