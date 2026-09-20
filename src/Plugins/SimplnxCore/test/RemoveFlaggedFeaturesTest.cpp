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

#include <algorithm>
#include <array>
#include <atomic>
#include <catch2/catch.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

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

/**
 * @brief Creates the shared data fixture for RemoveFlaggedFeatures tests.
 *
 * The shared fixture keeps Direct and Scanline comparisons on identical input data.
 * @return A populated in-memory DataStructure.
 */
DataStructure CreateFlaggedFeaturesFixture()
{
  DataStructure dataStructure;
  FillDataStructure(dataStructure);
  return dataStructure;
}

/**
 * @brief Creates the shared arguments for RemoveFlaggedFeatures fill-path tests.
 *
 * The shared arguments keep both algorithm paths on one filter contract.
 * @return Arguments that select feature removal with filling enabled.
 */
Arguments MakeRemoveArgs()
{
  Arguments args;
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_Functionality_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_FillRemovedFeatures_Key, std::make_any<bool>(true));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_FlaggedFeaturesArrayPath_Key, std::make_any<DataPath>(k_FlaggedFeaturesPath));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  return args;
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
  Arguments args = MakeRemoveArgs();

  DataStructure expectedData;
  bool hasExpectedData = false;
  const auto scenarios = SelectAlgorithmTestScenariosForInMemoryStores();
  for(const auto scenario : scenarios)
  {
    CAPTURE(scenario);
    DataStructure dataStructure = CreateFlaggedFeaturesFixture();
    AlgorithmTestScope scope(scenario);
    auto result = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_CellVectorPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<BoolArray>(k_CellBoolPath));
    const auto& ids = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath);
    const auto& vectors = dataStructure.getDataRefAs<Int32Array>(k_CellVectorPath);
    const auto& bools = dataStructure.getDataRefAs<BoolArray>(k_CellBoolPath);
    // The bottom-row removed cells are 13 and 14. Cell 13 has two
    // feature-2 neighbors; cell 14 has two background neighbors. The
    // winning second neighbors in face order are cells 12 and 15.
    const std::array<int32, 16> expectedIds = {0, 1, 1, 1, 1, 0, 2, 2, 2, 2, 0, 1, 2, 2, 0, 0};
    REQUIRE(ids.getSize() == expectedIds.size());
    REQUIRE(vectors.getSize() == 2 * expectedIds.size());
    REQUIRE(bools.getSize() == expectedIds.size());
    for(usize tupleIdx = 0; tupleIdx < expectedIds.size(); ++tupleIdx)
    {
      CAPTURE(tupleIdx);
      const usize sourceIdx = tupleIdx == 13 ? 12 : (tupleIdx == 14 ? 15 : tupleIdx);
      CHECK(ids[tupleIdx] == expectedIds[tupleIdx]);
      CHECK(vectors[2 * tupleIdx] == static_cast<int32>(sourceIdx));
      CHECK(vectors[2 * tupleIdx + 1] == -static_cast<int32>(sourceIdx));
      CHECK(bools[tupleIdx] == (sourceIdx % 2 == 0));
    }
    const DataPath featureDataPath({k_DataContainer, k_CellFeatureData});
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(featureDataPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(featureDataPath.createChildPath(k_Int32DataSet)));
    CHECK(dataStructure.getDataRefAs<AttributeMatrix>(featureDataPath).getNumberOfTuples() == 3);
    const auto& featureValues = dataStructure.getDataRefAs<Int32Array>(featureDataPath.createChildPath(k_Int32DataSet));
    REQUIRE(featureValues.getSize() == 3);
    CHECK(featureValues[0] == 0);
    CHECK(featureValues[1] == 4041);
    CHECK(featureValues[2] == 10128);

    if(hasExpectedData)
    {
      REQUIRE_NOTHROW(expectedData.getDataRefAs<IDataArray>(k_FeatureIdsPath));
      REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(k_FeatureIdsPath));
      CompareDataArrays<int32>(expectedData.getDataRefAs<IDataArray>(k_FeatureIdsPath), dataStructure.getDataRefAs<IDataArray>(k_FeatureIdsPath));
      REQUIRE_NOTHROW(expectedData.getDataRefAs<IDataArray>(k_CellVectorPath));
      REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(k_CellVectorPath));
      CompareDataArrays<int32>(expectedData.getDataRefAs<IDataArray>(k_CellVectorPath), dataStructure.getDataRefAs<IDataArray>(k_CellVectorPath));
      REQUIRE_NOTHROW(expectedData.getDataRefAs<IDataArray>(k_CellBoolPath));
      REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(k_CellBoolPath));
      CompareDataArrays<bool>(expectedData.getDataRefAs<IDataArray>(k_CellBoolPath), dataStructure.getDataRefAs<IDataArray>(k_CellBoolPath));
    }
    else
    {
      expectedData = std::move(dataStructure);
      hasExpectedData = true;
    }
  }
}

TEST_CASE("SimplnxCore::RemoveFlaggedFeatures: fill terminates with background zeros present", "[SimplnxCore][RemoveFlaggedFeatures]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  DYNAMIC_SECTION(scenario)
  {
    UnitTest::AlgorithmTestScope scope(scenario);

    // The fixture contains background Feature IDs. Fill mode never turns a zero into another id.
    DataStructure dataStructure = CreateFlaggedFeaturesFixture();
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath));

    // The filter overwrites the array in place, so the expected values are copied out first.
    std::vector<int32> idsBefore;
    usize zerosBefore = 0;
    {
      const auto& idsBeforeArray = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath);
      idsBefore.resize(idsBeforeArray.getNumberOfTuples());
      for(usize i = 0; i < idsBeforeArray.getNumberOfTuples(); ++i)
      {
        idsBefore[i] = idsBeforeArray[i];
        zerosBefore += idsBefore[i] == 0 ? 1 : 0;
      }
    }
    REQUIRE(zerosBefore > 0);

    RemoveFlaggedFeaturesFilter filter;
    Arguments args = MakeRemoveArgs();
    args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_FillRemovedFeatures_Key, std::make_any<bool>(true));

    std::atomic_bool cancel = false;
    std::atomic_bool finished = false;
    std::atomic_bool timedOut = false;
    // The watchdog cancels after 30 seconds so a nonterminating fill loop cannot block the test
    // process. It watches its own `finished` flag and raises `timedOut` itself, so a slow machine
    // cannot leave `cancel` set after a successful run and turn that into a spurious failure.
    std::thread watchdog([&]() {
      for(int32 i = 0; i < 300; ++i)
      {
        if(finished.load())
        {
          return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
      // The filter can finish during the last sleep, so the flag is checked once more before a
      // completed run is turned into a timeout.
      if(finished.load())
      {
        return;
      }
      timedOut = true;
      cancel = true;
    });
    auto result = scope.executeFilter(filter, dataStructure, args, nullptr, IFilter::MessageHandler{}, cancel);
    finished = true;
    watchdog.join();
    REQUIRE_FALSE(timedOut.load());
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath));
    const auto& idsAfter = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath);
    REQUIRE(idsAfter.getNumberOfTuples() == idsBefore.size());

    // Feature 3 is the only flagged feature in the fixture and it occupies exactly these voxels,
    // so every other voxel must come back untouched.
    const std::array<usize, 2> filledVoxels = {13, 14};

    usize negativesAfter = 0;
    usize zerosAfter = 0;
    for(usize i = 0; i < idsAfter.getNumberOfTuples(); ++i)
    {
      negativesAfter += idsAfter[i] < 0 ? 1 : 0;
      zerosAfter += idsAfter[i] == 0 ? 1 : 0;
      if(std::find(filledVoxels.cbegin(), filledVoxels.cend(), i) == filledVoxels.cend())
      {
        REQUIRE(idsAfter[i] == idsBefore[i]);
      }
    }

    // The fill loop marks each removed voxel with a negative ID and replaces it from a neighbor.
    // A negative ID left behind means the loop stopped before it converged.
    REQUIRE(negativesAfter == 0);
    for(usize filledIndex : filledVoxels)
    {
      // Feature 2 and the background border these voxels, so the fill value is one of the
      // surviving Feature IDs and never the removed feature.
      REQUIRE(idsAfter[filledIndex] >= 0);
      REQUIRE(idsAfter[filledIndex] <= 2);
    }
    // Background is a valid fill source, so removed voxels next to background become zero and the count can grow.
    REQUIRE(zerosAfter >= zerosBefore);
  }
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
