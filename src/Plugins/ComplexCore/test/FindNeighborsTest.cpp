#include <catch2/catch.hpp>

#include "ComplexCore/Filters/FindNeighbors.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"

namespace fs = std::filesystem;

using namespace complex;
using namespace complex::Constants;

TEST_CASE("ComplexCore::FindNeighbors", "[ComplexCore][FindNeighbors]")
{

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/TestFiles/6_6_stats_test.dream3d", unit_test::k_DREAM3DDataDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  DataPath smallIn100Group({complex::Constants::k_DataContainer});
  DataPath cellDataAttributeMatrix = smallIn100Group.createChildPath(k_CellData);
  DataPath featureIdsDataPath({k_DataContainer, k_CellData, k_FeatureIds});
  DataPath cellFeatureAttributeMatrixPath({k_DataContainer, k_CellFeatureData});
  DataPath numNeighborPath = cellFeatureAttributeMatrixPath.createChildPath("NumNeighbors_computed");
  DataPath neighborListPath = cellFeatureAttributeMatrixPath.createChildPath("NeighborList_computed");
  DataPath sharedSurfaceAreaListPath = cellFeatureAttributeMatrixPath.createChildPath("SharedSurfaceAreaList_computed");
  DataPath boundaryCellsPath = cellFeatureAttributeMatrixPath.createChildPath("BoundaryCells_computed");
  DataPath surfaceFeaturesPath = cellFeatureAttributeMatrixPath.createChildPath("SurfaceFeatures_computed");

  {
    FindNeighbors filter;
    Arguments args;

    args.insertOrAssign(FindNeighbors::k_ImageGeom_Key, std::make_any<DataPath>(smallIn100Group));
    args.insertOrAssign(FindNeighbors::k_FeatureIds_Key, std::make_any<DataPath>(featureIdsDataPath));
    args.insertOrAssign(FindNeighbors::k_CellFeatures_Key, std::make_any<DataPath>(cellFeatureAttributeMatrixPath));

    args.insertOrAssign(FindNeighbors::k_StoreBoundary_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindNeighbors::k_BoundaryCells_Key, std::make_any<DataPath>(boundaryCellsPath));

    args.insertOrAssign(FindNeighbors::k_StoreSurface_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindNeighbors::k_SurfaceFeatures_Key, std::make_any<DataPath>(surfaceFeaturesPath));

    args.insertOrAssign(FindNeighbors::k_NumNeighbors_Key, std::make_any<DataPath>(numNeighborPath));
    args.insertOrAssign(FindNeighbors::k_NeighborList_Key, std::make_any<DataPath>(neighborListPath));
    args.insertOrAssign(FindNeighbors::k_SharedSurfaceArea_Key, std::make_any<DataPath>(sharedSurfaceAreaListPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Output
  {
    DataPath featureGroup = smallIn100Group.createChildPath(complex::Constants::k_CellFeatureData);
    DataPath exemplaryDataPath = featureGroup.createChildPath("SurfaceFeatures");
    UnitTest::CompareArrays<bool>(dataStructure, exemplaryDataPath, surfaceFeaturesPath);

    exemplaryDataPath = featureGroup.createChildPath("NumNeighbors");
    UnitTest::CompareArrays<int32>(dataStructure, exemplaryDataPath, numNeighborPath);

    exemplaryDataPath = featureGroup.createChildPath("NeighborList");
    UnitTest::CompareNeighborLists<int32>(dataStructure, exemplaryDataPath, neighborListPath);

    exemplaryDataPath = featureGroup.createChildPath("SharedSurfaceAreaList");
    UnitTest::CompareNeighborLists<float32>(dataStructure, exemplaryDataPath, sharedSurfaceAreaListPath);
  }

  // Write the DataStructure out to the file system
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_neighbors_test.dream3d", unit_test::k_BinaryTestOutputDir)));
}
