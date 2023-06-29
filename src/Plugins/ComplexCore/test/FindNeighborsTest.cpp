#include "ComplexCore/Filters/FindNeighbors.hpp"
#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

namespace fs = std::filesystem;
using namespace complex;
using namespace complex::Constants;
using namespace complex::UnitTest;

TEST_CASE("ComplexCore::FindNeighbors", "[ComplexCore][FindNeighbors]")
{
  const std::string kDataInputArchive = "6_6_stats_test.tar.gz";
  const std::string kExpectedOutputTopLevel = "6_6_stats_test.dream3d";
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, kDataInputArchive, kExpectedOutputTopLevel,
                                                             complex::unit_test::k_BinaryTestOutputDir);
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_stats_test.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  DataPath smallIn100Group({complex::Constants::k_DataContainer});
  DataPath cellDataAttributeMatrix = smallIn100Group.createChildPath(k_CellData);
  DataPath featureIdsDataPath({k_DataContainer, k_CellData, k_FeatureIds});
  DataPath cellFeatureAttributeMatrixPath({k_DataContainer, k_CellFeatureData});
  std::string numNeighborName = "NumNeighbors_computed";
  std::string neighborListName = "NeighborList_computed";
  std::string sharedSurfaceAreaListName = "SharedSurfaceAreaList_computed";
  std::string boundaryCellsName = "BoundaryCells_computed";
  std::string surfaceFeaturesName = "SurfaceFeatures_computed";

  {
    FindNeighbors filter;
    Arguments args;

    args.insertOrAssign(FindNeighbors::k_ImageGeom_Key, std::make_any<DataPath>(smallIn100Group));
    args.insertOrAssign(FindNeighbors::k_FeatureIds_Key, std::make_any<DataPath>(featureIdsDataPath));
    args.insertOrAssign(FindNeighbors::k_CellFeatures_Key, std::make_any<DataPath>(cellFeatureAttributeMatrixPath));

    args.insertOrAssign(FindNeighbors::k_StoreBoundary_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindNeighbors::k_BoundaryCells_Key, std::make_any<std::string>(boundaryCellsName));

    args.insertOrAssign(FindNeighbors::k_StoreSurface_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindNeighbors::k_SurfaceFeatures_Key, std::make_any<std::string>(surfaceFeaturesName));

    args.insertOrAssign(FindNeighbors::k_NumNeighbors_Key, std::make_any<std::string>(numNeighborName));
    args.insertOrAssign(FindNeighbors::k_NeighborList_Key, std::make_any<std::string>(neighborListName));
    args.insertOrAssign(FindNeighbors::k_SharedSurfaceArea_Key, std::make_any<std::string>(sharedSurfaceAreaListName));

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
    UnitTest::CompareArrays<bool>(dataStructure, exemplaryDataPath, cellFeatureAttributeMatrixPath.createChildPath(surfaceFeaturesName));

    exemplaryDataPath = featureGroup.createChildPath("NumNeighbors");
    UnitTest::CompareArrays<int32>(dataStructure, exemplaryDataPath, cellFeatureAttributeMatrixPath.createChildPath(numNeighborName));

    exemplaryDataPath = featureGroup.createChildPath("NeighborList");
    UnitTest::CompareNeighborLists<int32>(dataStructure, exemplaryDataPath, cellFeatureAttributeMatrixPath.createChildPath(neighborListName));

    exemplaryDataPath = featureGroup.createChildPath("SharedSurfaceAreaList");
    UnitTest::CompareNeighborLists<float32>(dataStructure, exemplaryDataPath, cellFeatureAttributeMatrixPath.createChildPath(sharedSurfaceAreaListName));
  }

// Write the DataStructure out to the file system
#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_neighbors_test.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}
