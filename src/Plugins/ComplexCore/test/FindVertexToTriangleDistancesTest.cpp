#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/FindVertexToTriangleDistancesFilter.hpp"

#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace complex;
using namespace complex::UnitTest;

namespace
{
inline constexpr StringLiteral k_DistancesName("Distances");
inline constexpr StringLiteral k_ClosestTriangleIdsName("ClosestTriangleId");
inline constexpr StringLiteral k_DistancesNameNX("DistancesNX");
inline constexpr StringLiteral k_ClosestTriangleIdsNameNX("Closest Triangle Ids");
} // namespace

TEST_CASE("ComplexCore::FindVertexToTriangleDistancesFilter", "[ComplexCore][FindVertexToTriangleDistancesFilter]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_6_vertex_to_triangle_distances.tar.gz",
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
    FindVertexToTriangleDistancesFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(FindVertexToTriangleDistancesFilter::k_VertexDataContainer_Key, std::make_any<DataPath>(vertexData));
    args.insertOrAssign(FindVertexToTriangleDistancesFilter::k_TriangleDataContainer_Key, std::make_any<DataPath>(triangleData));
    args.insertOrAssign(FindVertexToTriangleDistancesFilter::k_TriangleNormalsArrayPath_Key, std::make_any<DataPath>(normalsPath));
    args.insertOrAssign(FindVertexToTriangleDistancesFilter::k_DistancesArrayPath_Key, std::make_any<std::string>(k_DistancesNameNX));
    args.insertOrAssign(FindVertexToTriangleDistancesFilter::k_ClosestTriangleIdArrayPath_Key, std::make_any<std::string>(k_ClosestTriangleIdsNameNX));

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
#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/vertex_to_triangle_distances.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}
