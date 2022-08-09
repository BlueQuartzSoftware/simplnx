#include <catch2/catch.hpp>

#include "ComplexCore/Filters/FindNeighbors.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"

namespace fs = std::filesystem;

using namespace complex;
using namespace complex::Constants;

namespace
{

template <typename T>
void CompareArrays(const DataStructure& dataStructure, const DataPath& exemplaryDataPath, const DataPath& computedPath)
{
  // DataPath exemplaryDataPath = featureGroup.createChildPath("SurfaceFeatures");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(exemplaryDataPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(computedPath));

  const auto& featureArrayExemplary = dataStructure.getDataRefAs<DataArray<T>>(exemplaryDataPath);
  const auto& createdFeatureArray = dataStructure.getDataRefAs<DataArray<T>>(computedPath);
  REQUIRE(createdFeatureArray.getNumberOfTuples() == featureArrayExemplary.getNumberOfTuples());

  for(usize i = 0; i < featureArrayExemplary.getSize(); i++)
  {
    REQUIRE(featureArrayExemplary[i] == createdFeatureArray[i]);
  }
}

template <typename T>
void CompareNeighborLists(const DataStructure& dataStructure, const DataPath& exemplaryDataPath, const DataPath& computedPath)
{
  // DataPath exemplaryDataPath = featureGroup.createChildPath("SurfaceFeatures");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<NeighborList<T>>(exemplaryDataPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<NeighborList<T>>(computedPath));

  const auto& exemplaryList = dataStructure.getDataRefAs<NeighborList<T>>(exemplaryDataPath);
  const auto& computedNeighborList = dataStructure.getDataRefAs<NeighborList<T>>(computedPath);
  REQUIRE(computedNeighborList.getNumberOfTuples() == exemplaryList.getNumberOfTuples());

  for(usize i = 0; i < exemplaryList.getSize(); i++)
  {
    const auto exemplary = exemplaryList.getList(i);
    const auto computed = computedNeighborList.getList(i);
    if(exemplary.get() != nullptr && computed.get() != nullptr)
    {
      REQUIRE(exemplary->size() == computed->size());
    }
  }
}

} // namespace

TEST_CASE("ComplexCore::FindNeighbors", "[ComplexCore][FindNeighbors]")
{

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1.dream3d", unit_test::k_TestDataSourceDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  DataPath smallIn100Group({complex::Constants::k_DataContainer});
  DataPath cellDataAttributeMatrix = smallIn100Group.createChildPath(k_CellData);
  DataPath featureIdsDataPath({k_DataContainer, k_CellData, k_FeatureIds});
  DataPath cellFeatureAttributeMatrixPath({k_DataContainer, k_CellFeatureData});
  DataPath numNeighborPath = cellFeatureAttributeMatrixPath.createChildPath("NumNeighbors2");
  DataPath neighborListPath = cellFeatureAttributeMatrixPath.createChildPath("NeighborList2");
  DataPath sharedSurfaceAreaListPath = cellFeatureAttributeMatrixPath.createChildPath("SharedSurfaceAreaList2");
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
    CompareArrays<bool>(dataStructure, exemplaryDataPath, surfaceFeaturesPath);

    exemplaryDataPath = featureGroup.createChildPath("NumNeighbors2");
    CompareArrays<int32>(dataStructure, exemplaryDataPath, numNeighborPath);

    exemplaryDataPath = featureGroup.createChildPath("NeighborList2");
    CompareNeighborLists<int32>(dataStructure, exemplaryDataPath, neighborListPath);

    exemplaryDataPath = featureGroup.createChildPath("SharedSurfaceAreaList2");
    CompareNeighborLists<float32>(dataStructure, exemplaryDataPath, sharedSurfaceAreaListPath);
  }

  {
    // Write out the DataStructure for later viewing/debugging
    Result<H5::FileWriter> result = H5::FileWriter::CreateFile(fmt::format("{}/find_neighbors_test.dream3d", unit_test::k_BinaryDir));
    H5::FileWriter fileWriter = std::move(result.value());
    herr_t err = dataStructure.writeHdf5(fileWriter);
    REQUIRE(err >= 0);
  }
}
