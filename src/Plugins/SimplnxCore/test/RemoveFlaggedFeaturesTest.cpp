#include "SimplnxCore/Filters/RemoveFlaggedFeaturesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <array>
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;
namespace fs = std::filesystem;

namespace
{
const std::string k_NewImgGeomPrefix = "NewImgGeom";
const std::string k_NewImgGeom = k_NewImgGeomPrefix + "-3";
const DataPath k_ImageGeomPath({k_DataContainer});
const DataPath k_FeatureIdsPath({k_DataContainer, k_CellData, k_FeatureIds});
const DataPath k_FlaggedFeaturesPath({k_DataContainer, k_CellFeatureData, k_ActiveName});
const DataPath k_NewFeatureIdsPath({k_NewImgGeom, k_CellData, k_FeatureIds});
const DataPath k_CellVectorPath({k_DataContainer, k_CellData, "CellVector"});
const DataPath k_CellBoolPath({k_DataContainer, k_CellData, "CellBool"});

void FillDataStructure(DataStructure& dataStructure)
{
  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_DataContainer);
  std::vector<size_t> dims = {4, 4, 1};
  imageGeom->setDimensions(dims);
  imageGeom->setOrigin(std::vector<float>{0, 0, 0});
  imageGeom->setSpacing(std::vector<float>{1, 1, 1});

  std::vector<size_t> tupleDims(dims.rbegin(), dims.rend());
  auto* attributeMatrix = AttributeMatrix::Create(dataStructure, k_CellData, tupleDims, imageGeom->getId());

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

  auto* cellVector = UnitTest::CreateTestDataArray<int32>(dataStructure, k_CellVectorPath.getTargetName(), tupleDims, {2}, attributeMatrix->getId());
  auto* cellBool = UnitTest::CreateTestDataArray<bool>(dataStructure, k_CellBoolPath.getTargetName(), tupleDims, {1}, attributeMatrix->getId());
  for(usize i = 0; i < testFeatIdsDataStore.getNumberOfTuples(); i++)
  {
    cellVector->getDataStoreRef()[i * 2] = static_cast<int32>(i);
    cellVector->getDataStoreRef()[i * 2 + 1] = -static_cast<int32>(i);
    cellBool->getDataStoreRef()[i] = (i % 2) == 0;
  }

  auto* featureAttributeMatrix = AttributeMatrix::Create(dataStructure, k_CellFeatureData, {4ULL}, imageGeom->getId());
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

void ReplaceBackgroundForFillTest(DataStructure& dataStructure)
{
  auto& featureIds = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath).getDataStoreRef();
  for(usize index = 0; index < featureIds.getNumberOfTuples(); index++)
  {
    if(featureIds[index] == 0)
    {
      featureIds[index] = 1;
    }
  }
}

template <bool RemoveV = true>
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
  if constexpr(RemoveV)
  {
    REQUIRE(featureIdsResult[13] == 0);
    REQUIRE(featureIdsResult[14] == 0);
  }
  if constexpr(!RemoveV)
  {
    REQUIRE(featureIdsResult[13] == 3);
    REQUIRE(featureIdsResult[14] == 3);
  }
  REQUIRE(featureIdsResult[15] == 0);

  if constexpr(RemoveV)
  {
    REQUIRE(cellFeatureAMResult.getNumberOfTuples() == 3);
  }
  if constexpr(!RemoveV)
  {
    REQUIRE(cellFeatureAMResult.getNumberOfTuples() == 4);
  }

  REQUIRE(testArrayResult[0] == 0);
  REQUIRE(testArrayResult[1] == 4041);
  REQUIRE(testArrayResult[2] == 10128);

  if constexpr(!RemoveV)
  {
    REQUIRE(testArrayResult[3] == 2185);
  }
}

void ValidateNewGeom(const Int32Array& featureIdsResult, const AttributeMatrix& cellFeatureAMResult, const Int32Array& testArrayResult)
{
  REQUIRE(featureIdsResult[0] == 3);
  REQUIRE(featureIdsResult[1] == 3);
}
} // namespace

TEST_CASE("SimplnxCore::RemoveFlaggedFeatures: Test Remove Algorithm", "[SimplnxCore][RemoveFlaggedFeatures]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  // Configure the filter arguments.
  RemoveFlaggedFeaturesFilter filter;
  DataStructure dataStructure;
  FillDataStructure(dataStructure);
  Arguments args;

  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_Functionality_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_FillRemovedFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_FlaggedFeaturesArrayPath_Key, std::make_any<DataPath>(k_FlaggedFeaturesPath));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& featureIdsResult = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath);
  const auto& cellFeatureAMResult = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({k_DataContainer, k_CellFeatureData}));
  const auto& testArrayResult = dataStructure.getDataRefAs<Int32Array>(DataPath({k_DataContainer, k_CellFeatureData, k_Int32DataSet}));
  ValidateResults(featureIdsResult, cellFeatureAMResult, testArrayResult);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RemoveFlaggedFeatures: Test Extract Algorithm", "[SimplnxCore][RemoveFlaggedFeatures]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  // Configure the filter arguments.
  RemoveFlaggedFeaturesFilter filter;
  DataStructure dataStructure;
  FillDataStructure(dataStructure);
  Arguments args;

  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_Functionality_Key, std::make_any<ChoicesParameter::ValueType>(1));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_CreatedImageGeometryPrefix_Key, std::make_any<std::string>(k_NewImgGeomPrefix));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_FlaggedFeaturesArrayPath_Key, std::make_any<DataPath>(k_FlaggedFeaturesPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/extract_flagged_features.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  auto& featureIdsResult = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath);
  auto& cellFeatureAMResult = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({k_DataContainer, k_CellFeatureData}));
  auto& testArrayResult = dataStructure.getDataRefAs<Int32Array>(DataPath({k_DataContainer, k_CellFeatureData, k_Int32DataSet}));
  ValidateResults<false>(featureIdsResult, cellFeatureAMResult, testArrayResult);

  auto& newFeatureIdsResult = dataStructure.getDataRefAs<Int32Array>(k_NewFeatureIdsPath);
  auto& newCellFeatureAMResult = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({k_NewImgGeom, k_CellFeatureData}));
  auto& newTestArrayResult = dataStructure.getDataRefAs<Int32Array>(DataPath({k_NewImgGeom, k_CellFeatureData, k_Int32DataSet}));
  ValidateNewGeom(newFeatureIdsResult, newCellFeatureAMResult, newTestArrayResult);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RemoveFlaggedFeatures: fill direct and scanline parity", "[SimplnxCore][RemoveFlaggedFeatures]")
{
  UnitTest::LoadPlugins();
  RemoveFlaggedFeaturesFilter filter;
  Arguments args;
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_Functionality_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_FillRemovedFeatures_Key, std::make_any<bool>(true));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_FlaggedFeaturesArrayPath_Key, std::make_any<DataPath>(k_FlaggedFeaturesPath));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));

  DataStructure directData;
  FillDataStructure(directData);
  ReplaceBackgroundForFillTest(directData);
  {
    AlgorithmTestScope scope(AlgorithmTestScenario::InCoreAlgorithmOnInMemoryStore);
    auto result = scope.executeFilter(filter, directData, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  }
  DataStructure scanlineData;
  FillDataStructure(scanlineData);
  ReplaceBackgroundForFillTest(scanlineData);
  {
    AlgorithmTestScope scope(AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
    auto result = scope.executeFilter(filter, scanlineData, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  }

  CompareDataArrays<int32>(directData.getDataRefAs<IDataArray>(k_FeatureIdsPath), scanlineData.getDataRefAs<IDataArray>(k_FeatureIdsPath));
  CompareDataArrays<int32>(directData.getDataRefAs<IDataArray>(k_CellVectorPath), scanlineData.getDataRefAs<IDataArray>(k_CellVectorPath));
  CompareDataArrays<bool>(directData.getDataRefAs<IDataArray>(k_CellBoolPath), scanlineData.getDataRefAs<IDataArray>(k_CellBoolPath));
}

TEST_CASE("SimplnxCore::RemoveFlaggedFeatures: Test Extract then Remove Algorithm", "[SimplnxCore][RemoveFlaggedFeatures]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  // Configure the filter arguments.
  RemoveFlaggedFeaturesFilter filter;
  DataStructure dataStructure;
  FillDataStructure(dataStructure);
  Arguments args;

  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_Functionality_Key, std::make_any<ChoicesParameter::ValueType>(2));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_FillRemovedFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_CreatedImageGeometryPrefix_Key, std::make_any<std::string>(k_NewImgGeomPrefix));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_FlaggedFeaturesArrayPath_Key, std::make_any<DataPath>(k_FlaggedFeaturesPath));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& featureIdsResult = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath);
  const auto& cellFeatureAMResult = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({k_DataContainer, k_CellFeatureData}));
  const auto& testArrayResult = dataStructure.getDataRefAs<Int32Array>(DataPath({k_DataContainer, k_CellFeatureData, k_Int32DataSet}));
  ValidateResults<true>(featureIdsResult, cellFeatureAMResult, testArrayResult);

  auto& newFeatureIdsResult = dataStructure.getDataRefAs<Int32Array>(k_NewFeatureIdsPath);
  auto& newCellFeatureAMResult = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({k_NewImgGeom, k_CellFeatureData}));
  auto& newTestArrayResult = dataStructure.getDataRefAs<Int32Array>(DataPath({k_NewImgGeom, k_CellFeatureData, k_Int32DataSet}));
  ValidateNewGeom(newFeatureIdsResult, newCellFeatureAMResult, newTestArrayResult);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RemoveFlaggedFeaturesFilter: SIMPL Backwards Compatibility", "[SimplnxCore][RemoveFlaggedFeaturesFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "RemoveFlaggedFeaturesFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "RemoveFlaggedFeaturesFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<RemoveFlaggedFeaturesFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<bool>(RemoveFlaggedFeaturesFilter::k_FillRemovedFeatures_Key) == true);
      // Successful pipeline loading verifies the MultiDataArraySelectionFilterParameterConverter value.
      CHECK(args.value<DataPath>(RemoveFlaggedFeaturesFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(RemoveFlaggedFeaturesFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(RemoveFlaggedFeaturesFilter::k_FlaggedFeaturesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
    }
  }
}
