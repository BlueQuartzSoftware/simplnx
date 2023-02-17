#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/ComputeFeatureRectFilter.hpp"

#include <catch2/catch.hpp>

using namespace complex;

namespace
{
const std::string k_ImageGeometryName = "ImageGeometry";
const std::string k_CellAttrMatrixName = "CellData";
const std::string k_FeatureAttrMatrixName = "FeatureData";
const std::string k_FeatureIdsArrayName = "FeatureIds";
const std::string k_RectCoordsArrayName = "RectCoords";
const std::string k_RectCoordsExemplaryArrayName = "RectCoordsExemplary";

// -----------------------------------------------------------------------------
DataStructure CreateTestData()
{
  DataStructure dataStructure;

  ImageGeom* iGeom = ImageGeom::Create(dataStructure, k_ImageGeometryName);
  std::vector<usize> dims = {5, 5, 1};
  iGeom->setDimensions(dims);
  AttributeMatrix* cellAM = AttributeMatrix::Create(dataStructure, k_CellAttrMatrixName, iGeom->getId());
  cellAM->setShape({dims[2], dims[1], dims[0]});

  Int32Array* featureIds = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, k_FeatureIdsArrayName, {dims[2], dims[1], dims[0]}, {1}, cellAM->getId());
  featureIds->fill(0);

  auto& featureIdsStore = featureIds->getIDataStoreRefAs<Int32DataStore>();
  featureIdsStore[6] = 1;
  featureIdsStore[7] = 1;
  featureIdsStore[8] = 1;

  featureIdsStore[11] = 1;
  featureIdsStore[12] = 1;
  featureIdsStore[13] = 1;

  featureIdsStore[16] = 1;
  featureIdsStore[17] = 1;
  featureIdsStore[18] = 1;

  AttributeMatrix* featureAM = AttributeMatrix::Create(dataStructure, k_FeatureAttrMatrixName, iGeom->getId());
  featureAM->setShape({2});

  std::vector<usize> compDims = {6};
  UInt32Array* rect = UInt32Array::CreateWithStore<UInt32DataStore>(dataStructure, k_RectCoordsExemplaryArrayName, {2}, {6}, featureAM->getId());
  rect->fill(0);

  auto& rectStore = rect->getIDataStoreRefAs<UInt32DataStore>();
  rectStore.setComponent(1, 0, 1);
  rectStore.setComponent(1, 1, 1);
  rectStore.setComponent(1, 2, 0);
  rectStore.setComponent(1, 3, 3);
  rectStore.setComponent(1, 4, 3);
  rectStore.setComponent(1, 5, 0);

  return dataStructure;
}
} // namespace

TEST_CASE("ComplexCore::ComputeFeatureRectFilter: Valid filter execution")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeFeatureRectFilter filter;
  DataStructure dataStructure = CreateTestData();
  Arguments args;

  args.insertOrAssign(ComputeFeatureRectFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{{k_ImageGeometryName, k_CellAttrMatrixName, k_FeatureIdsArrayName}}));
  args.insertOrAssign(ComputeFeatureRectFilter::k_FeatureRectArrayPath_Key, std::make_any<DataPath>(DataPath{{k_ImageGeometryName, k_FeatureAttrMatrixName, k_RectCoordsArrayName}}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  
  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32Array>({{k_ImageGeometryName, k_FeatureAttrMatrixName, k_RectCoordsArrayName}}));
  const UInt32Array& coords = dataStructure.getDataRefAs<UInt32Array>({{k_ImageGeometryName, k_FeatureAttrMatrixName, k_RectCoordsArrayName}});
  REQUIRE_NOTHROW(coords.getIDataStoreRefAs<UInt32DataStore>());
  const auto& coordsDataStore = coords.getIDataStoreRefAs<UInt32DataStore>();

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32Array>({{k_ImageGeometryName, k_FeatureAttrMatrixName, k_RectCoordsExemplaryArrayName}}));
  const UInt32Array& exemplaryCoords = dataStructure.getDataRefAs<UInt32Array>({{k_ImageGeometryName, k_FeatureAttrMatrixName, k_RectCoordsExemplaryArrayName}});
  REQUIRE_NOTHROW(exemplaryCoords.getIDataStoreRefAs<UInt32DataStore>());
  const auto& exemplaryCoordsDataStore = exemplaryCoords.getIDataStoreRefAs<UInt32DataStore>();

  REQUIRE(coordsDataStore.getComponentValue(1, 0) == exemplaryCoordsDataStore.getComponentValue(1, 0));
  REQUIRE(coordsDataStore.getComponentValue(1, 1) == exemplaryCoordsDataStore.getComponentValue(1, 1));
  REQUIRE(coordsDataStore.getComponentValue(1, 2) == exemplaryCoordsDataStore.getComponentValue(1, 2));
  REQUIRE(coordsDataStore.getComponentValue(1, 3) == exemplaryCoordsDataStore.getComponentValue(1, 3));
  REQUIRE(coordsDataStore.getComponentValue(1, 4) == exemplaryCoordsDataStore.getComponentValue(1, 4));
  REQUIRE(coordsDataStore.getComponentValue(1, 5) == exemplaryCoordsDataStore.getComponentValue(1, 5));
}

// TEST_CASE("ComplexCore::ComputeFeatureRectFilter: InValid filter execution")
//{
//
// }
