#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/RemoveFlaggedFeaturesFilter.hpp"

#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace complex;
using namespace complex::Constants;

namespace
{
const std::string k_NewImgGeomPrefix = "NewImgGeom";
const std::string k_NewImgGeom = k_NewImgGeomPrefix + "-3";
const DataPath k_ImageGeomPath({k_DataContainer});
const DataPath k_FeatureIdsPath({k_DataContainer, k_CellData, k_FeatureIds});
const DataPath k_FlaggedFeaturesPath({k_DataContainer, k_CellFeatureData, k_ActiveName});
const DataPath k_NewFeatureIdsPath({k_NewImgGeom, k_CellData, k_FeatureIds});

void FillDataStructure(DataStructure& dataStructure)
{
  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_DataContainer);
  std::vector<size_t> dims = {4, 4, 1};
  imageGeom->setDimensions(dims);
  imageGeom->setOrigin(std::vector<float>{0, 0, 0});
  imageGeom->setSpacing(std::vector<float>{1, 1, 1});

  auto* attributeMatrix = AttributeMatrix::Create(dataStructure, k_CellData, imageGeom->getId());
  std::vector<size_t> tupleDims(dims.rbegin(), dims.rend());
  attributeMatrix->setShape(tupleDims);
  imageGeom->setCellData(*attributeMatrix);

  Int32Array* featureIds = UnitTest::CreateTestDataArray<int32>(dataStructure, k_FeatureIds, tupleDims, {1}, attributeMatrix->getId());
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

  auto* featureAttributeMatrix = AttributeMatrix::Create(dataStructure, k_CellFeatureData, imageGeom->getId());
  featureAttributeMatrix->setShape({4});
  BoolArray* maskArray = BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, k_ActiveName, {4}, {1}, featureAttributeMatrix->getId());
  auto& maskDataStore = maskArray->getDataStoreRef();
  maskDataStore[0] = false;
  maskDataStore[1] = false;
  maskDataStore[2] = false;
  maskDataStore[3] = true;
  Int32Array* testArray = UnitTest::CreateTestDataArray<int32>(dataStructure, k_Int32DataSet, {4}, {1}, featureAttributeMatrix->getId());
  auto& testStore = testArray->getDataStoreRef();
  testStore[0] = 0;
  testStore[1] = 4041;
  testStore[2] = 10128;
  testStore[3] = 2185;
}

template <bool UseRemove, bool UseExtract>
struct TestTypeOptions
{
  static inline constexpr bool UsingRemove = UseRemove;
  static inline constexpr bool UsingExtract = UseExtract;
};

using Remove = TestTypeOptions<true, false>;
using Extract = TestTypeOptions<false, true>;

template <class TestTypeOptions = Remove>
void ValidateResults(const Int32Array& featureIdsResult, const AttributeMatrix& cellFeatureAMResult, const Int32Array& testArrayResult)
{
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
  if constexpr(TestTypeOptions::UsingRemove)
  {
    REQUIRE(featureIdsResult[13] == 0);
    REQUIRE(featureIdsResult[14] == 0);
    REQUIRE(featureIdsResult[15] == 0);
  }
  if constexpr(TestTypeOptions::UsingExtract)
  {
    REQUIRE(featureIdsResult[13] == 3);
    REQUIRE(featureIdsResult[14] == 3);
    REQUIRE(featureIdsResult[15] == 0);
  }

  if constexpr(TestTypeOptions::UsingRemove)
  {
    REQUIRE(cellFeatureAMResult.getNumTuples() == 3);
  }
  if constexpr(TestTypeOptions::UsingExtract)
  {
    REQUIRE(cellFeatureAMResult.getNumTuples() == 4);
  }

  REQUIRE(testArrayResult[0] == 0);
  REQUIRE(testArrayResult[1] == 4041);
  REQUIRE(testArrayResult[2] == 10128);

  if constexpr(TestTypeOptions::UsingExtract)
  {
    REQUIRE(testArrayResult[3] == 2185);
  }
}

void ValidateNewGeom(const Int32Array& featureIdsResult, const AttributeMatrix& cellFeatureAMResult, const Int32Array& testArrayResult)
{
  REQUIRE(featureIdsResult[0] == 3);
  REQUIRE(featureIdsResult[1] == 3);
  REQUIRE(featureIdsResult[2] == 0);
}
} // namespace

TEST_CASE("ComplexCore::RemoveFlaggedFeatures: Test Remove Algorithm", "[ComplexCore][RemoveFlaggedFeatures]")
{ // Instantiate the filter, a DataStructure object and an Arguments Object
  RemoveFlaggedFeaturesFilter filter;
  DataStructure dataStructure;
  FillDataStructure(dataStructure);
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_Functionality_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_FillRemovedFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_ImageGeometry_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_FlaggedFeaturesArrayPath_Key, std::make_any<DataPath>(k_FlaggedFeaturesPath));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& featureIdsResult = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath);
  const auto& cellFeatureAMResult = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({k_DataContainer, k_CellFeatureData}));
  const auto& testArrayResult = dataStructure.getDataRefAs<Int32Array>(DataPath({k_DataContainer, k_CellFeatureData, k_Int32DataSet}));
  ValidateResults(featureIdsResult, cellFeatureAMResult, testArrayResult);
}

TEST_CASE("ComplexCore::RemoveFlaggedFeatures: Test Extract Algorithm", "[ComplexCore][RemoveFlaggedFeatures]")
{ // Instantiate the filter, a DataStructure object and an Arguments Object
  RemoveFlaggedFeaturesFilter filter;
  DataStructure dataStructure;
  FillDataStructure(dataStructure);
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_Functionality_Key, std::make_any<ChoicesParameter::ValueType>(1));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_ImageGeometry_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_CreatedImageGeometryPrefix_Key, std::make_any<std::string>(k_NewImgGeomPrefix));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_FlaggedFeaturesArrayPath_Key, std::make_any<DataPath>(k_FlaggedFeaturesPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  auto& featureIdsResult = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath);
  auto& cellFeatureAMResult = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({k_DataContainer, k_CellFeatureData}));
  auto& testArrayResult = dataStructure.getDataRefAs<Int32Array>(DataPath({k_DataContainer, k_CellFeatureData, k_Int32DataSet}));
  ValidateResults<Extract>(featureIdsResult, cellFeatureAMResult, testArrayResult);

  auto& newFeatureIdsResult = dataStructure.getDataRefAs<Int32Array>(k_NewFeatureIdsPath);
  auto& newCellFeatureAMResult = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({k_NewImgGeom, k_CellFeatureData}));
  auto& newTestArrayResult = dataStructure.getDataRefAs<Int32Array>(DataPath({k_NewImgGeom, k_CellFeatureData, k_Int32DataSet}));
  ValidateNewGeom(newFeatureIdsResult, newCellFeatureAMResult, newTestArrayResult);
}

TEST_CASE("ComplexCore::RemoveFlaggedFeatures: Test Extract then Remove Algorithm", "[ComplexCore][RemoveFlaggedFeatures]")
{ // Instantiate the filter, a DataStructure object and an Arguments Object
  RemoveFlaggedFeaturesFilter filter;
  DataStructure dataStructure;
  FillDataStructure(dataStructure);
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_Functionality_Key, std::make_any<ChoicesParameter::ValueType>(2));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_FillRemovedFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_ImageGeometry_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_CreatedImageGeometryPrefix_Key, std::make_any<std::string>(k_NewImgGeomPrefix));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_FlaggedFeaturesArrayPath_Key, std::make_any<DataPath>(k_FlaggedFeaturesPath));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& featureIdsResult = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath);
  const auto& cellFeatureAMResult = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({k_DataContainer, k_CellFeatureData}));
  const auto& testArrayResult = dataStructure.getDataRefAs<Int32Array>(DataPath({k_DataContainer, k_CellFeatureData, k_Int32DataSet}));
  ValidateResults(featureIdsResult, cellFeatureAMResult, testArrayResult);

  auto& newFeatureIdsResult = dataStructure.getDataRefAs<Int32Array>(k_NewFeatureIdsPath);
  auto& newCellFeatureAMResult = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({k_NewImgGeom, k_CellFeatureData}));
  auto& newTestArrayResult = dataStructure.getDataRefAs<Int32Array>(DataPath({k_NewImgGeom, k_CellFeatureData, k_Int32DataSet}));
  ValidateNewGeom(newFeatureIdsResult, newCellFeatureAMResult, newTestArrayResult);
}
