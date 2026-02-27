#include "SimplnxCore/Filters/ComputeFeatureNeighborsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
// Geometry Level
const std::string k_ImageGeomName = "Image";
const DataPath k_ImageGeomPath = DataPath({k_ImageGeomName});

// Cell Level
const std::string k_CellAMName = "CellData";
const DataPath k_CellAMPath = k_ImageGeomPath.createChildPath(k_CellAMName);
const std::string k_FeatureIdsName = "FeatureIds";
const DataPath k_FeatureIdsPath = k_CellAMPath.createChildPath(k_FeatureIdsName);

// Feature Level
const std::string k_FeatureAMName = "FeatureData";
const DataPath k_FeatureAMPath = k_ImageGeomPath.createChildPath(k_FeatureAMName);

// Created Array Names and Paths
const std::string k_NumElementsName = "NumElements";
const DataPath k_NumElementsPath = k_FeatureAMPath.createChildPath(k_NumElementsName);
const std::string k_VolumesName = "Volumes";
const DataPath k_VolumesPath = k_FeatureAMPath.createChildPath(k_VolumesName);
const std::string k_EquivalentDiametersName = "EquivalentDiameters";
const DataPath k_EquivalentDiametersPath = k_FeatureAMPath.createChildPath(k_EquivalentDiametersName);

DataStructure Create2DImageDataStructure()
{
  // Create an ImageGeom
  DataStructure dataStructure = {};
  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeomName);
  imageGeom->setSpacing(FloatVec3{std::array<float32, 3>{2.2f, 1.2f, 67777.1f}});
  imageGeom->setOrigin(FloatVec3{std::array<float32, 3>{0.0f, 0.0f, 0.0f}});
  imageGeom->setDimensions(SizeVec3{std::array<usize, 3>{3, 3, 1}});

  AttributeMatrix* cellData = AttributeMatrix::Create(dataStructure, k_CellAMName, ShapeType{3, 3, 1}, imageGeom->getId());
  imageGeom->setCellData(*cellData);

  Int32Array* featureIds = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, k_FeatureIdsName, cellData->getShape(), ShapeType{1}, cellData->getId());

  AttributeMatrix* featureData = AttributeMatrix::Create(dataStructure, k_FeatureAMName, ShapeType{4}, imageGeom->getId());

  // clang-format off
  const std::array<uint8, 25> featureIdsArray = {
    1, 2, 3,
    3, 3, 1,
    3, 3, 3
  };
  // clang-format on

  for(usize i = 0; i < featureIds->getNumberOfTuples(); i++)
  {
    featureIds->setValue(i, featureIdsArray[i]);
  }

  return dataStructure;
}
}

// TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: Base Case", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
// {
//
// }

TEST_CASE("SimplnxCore::ComputeFeatureNeighborsFilter: SmallIn100 ", "[SimplnxCore][ComputeFeatureNeighborsFilter]")
{
  UnitTest::LoadPlugins();

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "6_6_stats_test_v2.tar.gz", "6_6_stats_test_v2.dream3d");
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_stats_test_v2.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  DataPath smallIn100Group({Constants::k_DataContainer});
  DataPath cellDataAttributeMatrix = smallIn100Group.createChildPath(Constants::k_CellData);
  DataPath featureIdsDataPath({Constants::k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  DataPath cellFeatureAttributeMatrixPath({Constants::k_DataContainer, Constants::k_CellFeatureData});
  std::string numNeighborName = "NumNeighbors_computed";
  std::string neighborListName = "NeighborList_computed";
  std::string sharedSurfaceAreaListName = "SharedSurfaceAreaList_computed";
  std::string boundaryCellsName = "BoundaryCells_computed";
  std::string surfaceFeaturesName = "SurfaceFeatures_computed";

  {
    ComputeFeatureNeighborsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(smallIn100Group));
    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(featureIdsDataPath));
    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_CellFeaturesPath_Key, std::make_any<DataPath>(cellFeatureAttributeMatrixPath));

    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_StoreBoundary_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_BoundaryCellsName_Key, std::make_any<std::string>(boundaryCellsName));

    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_StoreSurface_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_SurfaceFeaturesName_Key, std::make_any<std::string>(surfaceFeaturesName));

    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_NumNeighborsName_Key, std::make_any<std::string>(numNeighborName));
    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_NeighborListName_Key, std::make_any<std::string>(neighborListName));
    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_SharedSurfaceAreaName_Key, std::make_any<std::string>(sharedSurfaceAreaListName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Output
  {
    DataPath featureGroup = smallIn100Group.createChildPath(Constants::k_CellFeatureData);
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
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_neighbors_test.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
