#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/ComputeFeatureRectFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

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
  std::vector<usize> dims = {5, 5, 5};
  iGeom->setDimensions(dims);
  AttributeMatrix* cellAM = AttributeMatrix::Create(dataStructure, k_CellAttrMatrixName, {dims[2], dims[1], dims[0]}, iGeom->getId());

  Int32Array* featureIds = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, k_FeatureIdsArrayName, {dims[2], dims[1], dims[0]}, {1}, cellAM->getId());
  featureIds->fill(0);

  auto& featureIdsStore = featureIds->getDataStoreRef();
  featureIdsStore[6] = 1;
  featureIdsStore[7] = 1;
  featureIdsStore[8] = 1;

  featureIdsStore[11] = 1;
  featureIdsStore[12] = 1;
  featureIdsStore[13] = 1;

  featureIdsStore[16] = 1;
  featureIdsStore[17] = 1;
  featureIdsStore[18] = 1;

  featureIdsStore[31] = 2;
  featureIdsStore[32] = 2;
  featureIdsStore[33] = 2;

  featureIdsStore[36] = 2;
  featureIdsStore[37] = 2;
  featureIdsStore[38] = 2;

  featureIdsStore[41] = 2;
  featureIdsStore[42] = 2;
  featureIdsStore[43] = 2;

  featureIdsStore[56] = 3;
  featureIdsStore[57] = 3;
  featureIdsStore[58] = 3;

  featureIdsStore[61] = 3;
  featureIdsStore[62] = 3;
  featureIdsStore[63] = 3;

  featureIdsStore[66] = 3;
  featureIdsStore[67] = 3;
  featureIdsStore[68] = 3;

  AttributeMatrix* featureAM = AttributeMatrix::Create(dataStructure, k_FeatureAttrMatrixName, {4}, iGeom->getId());

  std::vector<usize> compDims = {6};
  UInt32Array* rect = UInt32Array::CreateWithStore<UInt32DataStore>(dataStructure, k_RectCoordsExemplaryArrayName, {4}, {6}, featureAM->getId());
  auto& rectStore = rect->getDataStoreRef();
  rectStore.setComponent(1, 0, 1);
  rectStore.setComponent(1, 1, 1);
  rectStore.setComponent(1, 2, 0);
  rectStore.setComponent(1, 3, 3);
  rectStore.setComponent(1, 4, 3);
  rectStore.setComponent(1, 5, 0);

  rectStore.setComponent(2, 0, 1);
  rectStore.setComponent(2, 1, 1);
  rectStore.setComponent(2, 2, 1);
  rectStore.setComponent(2, 3, 3);
  rectStore.setComponent(2, 4, 3);
  rectStore.setComponent(2, 5, 1);

  rectStore.setComponent(3, 0, 1);
  rectStore.setComponent(3, 1, 1);
  rectStore.setComponent(3, 2, 2);
  rectStore.setComponent(3, 3, 3);
  rectStore.setComponent(3, 4, 3);
  rectStore.setComponent(3, 5, 2);

  return dataStructure;
}
} // namespace

TEST_CASE("SimplnxCore::ComputeFeatureRectFilter: Valid filter execution", "[SimplnxCore][ComputeFeatureRectFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeFeatureRectFilter filter;
  DataStructure dataStructure = CreateTestData();
  Arguments args;

  args.insertOrAssign(ComputeFeatureRectFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{{k_ImageGeometryName, k_CellAttrMatrixName, k_FeatureIdsArrayName}}));
  args.insertOrAssign(ComputeFeatureRectFilter::k_FeatureDataAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath{{k_ImageGeometryName, k_FeatureAttrMatrixName}}));
  args.insertOrAssign(ComputeFeatureRectFilter::k_FeatureRectArrayPath_Key, std::make_any<std::string>(k_RectCoordsArrayName));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32Array>({{k_ImageGeometryName, k_FeatureAttrMatrixName, k_RectCoordsArrayName}}));
  const UInt32Array& coords = dataStructure.getDataRefAs<UInt32Array>({{k_ImageGeometryName, k_FeatureAttrMatrixName, k_RectCoordsArrayName}});
  const auto& coordsDataStore = coords.getDataStoreRef();

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32Array>({{k_ImageGeometryName, k_FeatureAttrMatrixName, k_RectCoordsExemplaryArrayName}}));
  const UInt32Array& exemplaryCoords = dataStructure.getDataRefAs<UInt32Array>({{k_ImageGeometryName, k_FeatureAttrMatrixName, k_RectCoordsExemplaryArrayName}});
  const auto& exemplaryCoordsDataStore = exemplaryCoords.getDataStoreRef();

  REQUIRE(coordsDataStore.getNumberOfTuples() == exemplaryCoordsDataStore.getNumberOfTuples());
  REQUIRE(coordsDataStore.getNumberOfComponents() == exemplaryCoordsDataStore.getNumberOfComponents());
  for(usize tuple = 1; tuple < coordsDataStore.getNumberOfTuples(); tuple++)
  {
    for(usize comp = 0; comp < coordsDataStore.getNumberOfComponents(); comp++)
    {
      REQUIRE(coordsDataStore.getComponentValue(tuple, comp) == exemplaryCoordsDataStore.getComponentValue(tuple, comp));
    }
  }
}
