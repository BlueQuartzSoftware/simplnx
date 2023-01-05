#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/FindVertexToTriangleDistancesFilter.hpp"

using namespace complex;

namespace
{
inline constexpr StringLiteral k_DistancesName("Distances");
inline constexpr StringLiteral k_ClosestTriangleIdsName("Closest Triangle Ids");
inline constexpr StringLiteral k_DistancesNameNX("Distances_NX");
inline constexpr StringLiteral k_ClosestTriangleIdsNameNX("Closest_Triangle_Ids_NX");
} // namespace

TEST_CASE("ComplexCore::FindVertexToTriangleDistancesFilter: Valid filter execution", "[ComplexCore][FindVertexToTriangleDistancesFilter]")
{
  // Read the Small IN100 Data set
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/6_6_some_test.dream3d", unit_test::k_TestFilesDir)));
  DataPath smallIn100Group({complex::Constants::k_DataContainer});
  DataPath cellDataPath = smallIn100Group.createChildPath(complex::Constants::k_CellData);
  DataPath cellPhasesPath = cellDataPath.createChildPath(complex::Constants::k_Phases);
  DataPath featureIdsPath = cellDataPath.createChildPath(complex::Constants::k_FeatureIds);
  DataPath featureGroup = smallIn100Group.createChildPath(complex::Constants::k_CellFeatureData);
 
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    FindVertexToTriangleDistancesFilter filter;

    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(FindVertexToTriangleDistancesFilter::k_VertexDataContainer_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(FindVertexToTriangleDistancesFilter::k_TriangleDataContainer_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(FindVertexToTriangleDistancesFilter::k_TriangleNormalsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
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
    DataPath exemplaryDataPath = featureGroup.createChildPath(k_DistancesName);
    UnitTest::CompareArrays<float32>(dataStructure, exemplaryDataPath, featureGroup.createChildPath(k_DistancesNameNX));
  }

  {
    DataPath exemplaryDataPath = featureGroup.createChildPath(k_ClosestTriangleIdsName);
    UnitTest::CompareArrays<float32>(dataStructure, exemplaryDataPath, featureGroup.createChildPath(k_ClosestTriangleIdsNameNX));
  }

  // Write the DataStructure out to the file system
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/calculate_feature_sizes.dream3d", unit_test::k_BinaryTestOutputDir)));
}
