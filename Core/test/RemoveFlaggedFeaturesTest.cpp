#include "Core/Core_test_dirs.hpp"
#include "Core/Filters/RemoveFlaggedFeaturesFilter.hpp"

#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace complex;

TEST_CASE("Core::RemoveFlaggedFeatures: Test Algorithm", "[Core][RemoveFlaggedFeatures]")
{ // Instantiate the filter, a DataStructure object and an Arguments Object
  RemoveFlaggedFeaturesFilter filter;
  DataStructure dataStructure;
  Arguments args;

  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, Constants::k_DataContainer);
  std::vector<size_t> dims = {4, 4, 1};
  imageGeom->setDimensions(dims);
  imageGeom->setOrigin(std::vector<float>{0, 0, 0});
  imageGeom->setSpacing(std::vector<float>{1, 1, 1});

  auto* attributeMatrix = AttributeMatrix::Create(dataStructure, Constants::k_CellData, imageGeom->getId());
  std::vector<size_t> tupleDims(dims.rbegin(), dims.rend());
  attributeMatrix->setShape(tupleDims);
  imageGeom->setCellData(*attributeMatrix);

  Int32Array* featureIds = UnitTest::CreateTestDataArray<int32>(dataStructure, Constants::k_FeatureIds, tupleDims, {1}, attributeMatrix->getId());
  auto& testFeatIdsDataStore = featureIds->getDataStoreRef();
  testFeatIdsDataStore[0] = 0;
  testFeatIdsDataStore[1] = 1;
  testFeatIdsDataStore[2] = 1;
  testFeatIdsDataStore[3] = 1;
  testFeatIdsDataStore[4] = 1;
  testFeatIdsDataStore[5] = 0;
  testFeatIdsDataStore[6] = 2;
  testFeatIdsDataStore[7] = 2;
  testFeatIdsDataStore[8] = 2;
  testFeatIdsDataStore[9] = 2;
  testFeatIdsDataStore[10] = 0;
  testFeatIdsDataStore[11] = 1;
  testFeatIdsDataStore[12] = 2;
  testFeatIdsDataStore[13] = 3;
  testFeatIdsDataStore[14] = 3;
  testFeatIdsDataStore[15] = 0;

  auto* featureAttributeMatrix = AttributeMatrix::Create(dataStructure, Constants::k_CellFeatureData, imageGeom->getId());
  featureAttributeMatrix->setShape({4});
  BoolArray* maskArray = BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, Constants::k_ActiveName, {4}, {1}, featureAttributeMatrix->getId());
  auto& maskDataStore = maskArray->getDataStoreRef();
  maskDataStore[0] = false;
  maskDataStore[1] = false;
  maskDataStore[2] = false;
  maskDataStore[3] = true;
  Int32Array* testArray = UnitTest::CreateTestDataArray<int32>(dataStructure, Constants::k_Int32DataSet, {4}, {1}, featureAttributeMatrix->getId());
  auto& testStore = testArray->getDataStoreRef();
  testStore[0] = 0;
  testStore[1] = 4041;
  testStore[2] = 10128;
  testStore[3] = 2185;

  DataPath imageGeomPath({Constants::k_DataContainer});
  DataPath featureIdsPath({Constants::k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  DataPath flaggedFeaturesPath({Constants::k_DataContainer, Constants::k_CellFeatureData, Constants::k_ActiveName});

  // Create default Parameters for the filter.
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_FillRemovedFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_ImageGeometry_Key, std::make_any<DataPath>(imageGeomPath));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsPath));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_FlaggedFeaturesArrayPath_Key, std::make_any<DataPath>(flaggedFeaturesPath));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  Int32Array& featureIdsResult = dataStructure.getDataRefAs<Int32Array>(featureIdsPath);
  REQUIRE(featureIdsResult[0] == 0);
  REQUIRE(featureIdsResult[1] == 1);
  REQUIRE(featureIdsResult[2] == 1);
  REQUIRE(featureIdsResult[3] == 1);
  REQUIRE(featureIdsResult[4] == 1);
  REQUIRE(featureIdsResult[5] == 0);
  REQUIRE(featureIdsResult[6] == 2);
  REQUIRE(featureIdsResult[7] == 2);
  REQUIRE(featureIdsResult[8] == 2);
  REQUIRE(featureIdsResult[9] == 2);
  REQUIRE(featureIdsResult[10] == 0);
  REQUIRE(featureIdsResult[11] == 1);
  REQUIRE(featureIdsResult[12] == 2);
  REQUIRE(featureIdsResult[13] == 0);
  REQUIRE(featureIdsResult[14] == 0);
  REQUIRE(featureIdsResult[15] == 0);

  AttributeMatrix& cellFeatureAMResult = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({Constants::k_DataContainer, Constants::k_CellFeatureData}));
  REQUIRE(cellFeatureAMResult.getNumTuples() == 3);

  Int32Array& testArrayResult = dataStructure.getDataRefAs<Int32Array>(DataPath({Constants::k_DataContainer, Constants::k_CellFeatureData, Constants::k_Int32DataSet}));
  REQUIRE(testArrayResult[0] == 0);
  REQUIRE(testArrayResult[1] == 4041);
  REQUIRE(testArrayResult[2] == 10128);
}
