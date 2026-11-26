#include "CellDataAliasTestUtils.hpp"

#include "SimplnxCore/Filters/FillBadDataFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include <catch2/catch.hpp>
#include <nonstd/span.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Pipeline/AbstractPipelineNode.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <algorithm>
#include <filesystem>
#include <string_view>

namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
/**
 * @brief Builds a FillBadData test dataset with block-patterned FeatureIds
 * and ~10% scattered bad voxels (FeatureId=0) using a deterministic pattern.
 */
void BuildFillBadDataTestData(DataStructure& ds, usize dimX, usize dimY, usize dimZ, usize blockSize, bool addLargeDefect = false)
{
  const ShapeType cellShape = {dimZ, dimY, dimX};
  auto* imageGeom = ImageGeom::Create(ds, "DataContainer");
  imageGeom->setDimensions({dimX, dimY, dimZ});
  REQUIRE(imageGeom->setSpacing({1.0f, 1.0f, 1.0f}).valid());
  REQUIRE(imageGeom->setOrigin({0.0f, 0.0f, 0.0f}).valid());

  auto* cellAM = AttributeMatrix::Create(ds, "CellData", cellShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  const DataPath featureIdsPath = DataPath({"DataContainer", "CellData", "FeatureIds"});
  auto featureIdsDataStore = DataStoreUtilities::CreateDataStore<int32>(ds, featureIdsPath, cellShape, {1});
  auto* featureIdsArray = DataArray<int32>::Create(ds, "FeatureIds", featureIdsDataStore, cellAM->getId());
  auto& featureIdsStore = featureIdsArray->getDataStoreRef();

  const DataPath phasesPath = DataPath({"DataContainer", "CellData", "Phases"});
  auto phasesDataStore = DataStoreUtilities::CreateDataStore<int32>(ds, phasesPath, cellShape, {1});
  auto* phasesArray = DataArray<int32>::Create(ds, "Phases", phasesDataStore, cellAM->getId());
  auto& phasesStore = phasesArray->getDataStoreRef();

  const usize blocksPerDim = dimX / blockSize;
  const usize sliceSize = dimY * dimX;
  std::vector<int32> featureIdsSliceBuffer(sliceSize);
  std::vector<int32> phasesSliceBuffer(sliceSize, 1);

  for(usize z = 0; z < dimZ; z++)
  {
    for(usize y = 0; y < dimY; y++)
    {
      for(usize x = 0; x < dimX; x++)
      {
        usize bx = x / blockSize;
        usize by = y / blockSize;
        usize bz = z / blockSize;
        int32 blockFeatureId = static_cast<int32>(bz * blocksPerDim * blocksPerDim + by * blocksPerDim + bx + 1);

        // Scatter bad voxels: ~10% of voxels become bad (FeatureId=0)
        bool isBad = ((x * 7 + y * 13 + z * 29) % 10 == 0);
        featureIdsSliceBuffer[y * dimX + x] = isBad ? 0 : blockFeatureId;
      }
    }

    // Add a contiguous large defect: entire z=dimZ/2 plane set to FeatureId=0
    if(addLargeDefect && z == dimZ / 2)
    {
      std::fill(featureIdsSliceBuffer.begin(), featureIdsSliceBuffer.end(), 0);
    }

    auto featureIdsStoreWriteResult = featureIdsStore.copyFromBuffer(z * sliceSize, nonstd::span<const int32>(featureIdsSliceBuffer.data(), sliceSize));
    SIMPLNX_RESULT_REQUIRE_VALID(featureIdsStoreWriteResult);
    auto phasesStoreWriteResult = phasesStore.copyFromBuffer(z * sliceSize, nonstd::span<const int32>(phasesSliceBuffer.data(), sliceSize));
    SIMPLNX_RESULT_REQUIRE_VALID(phasesStoreWriteResult);
  }
}
void CheckFillExemplar(std::string_view fixture, int32 minDefectSize, bool storeAsNewPhase)
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario, fixture);
  UnitTest::AlgorithmTestScope scope(scenario);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "6_5_fill_bad_data.tar.gz", "6_5_fill_bad_data");
  const fs::path directory = fs::path(unit_test::k_TestFilesDir.view()) / "6_5_fill_bad_data";
  auto dataStructure = UnitTest::LoadDataStructure(directory / fmt::format("{}_input.dream3d", fixture));
  const auto expected = UnitTest::LoadDataStructure(directory / fmt::format("{}_expected.dream3d", fixture));
  const DataPath geometryPath({"DataContainer"});
  const DataPath cellPath = geometryPath.createChildPath("CellData");
  const auto idsPath = cellPath.createChildPath("FeatureIds");
  const auto phasesPath = cellPath.createChildPath("Phases");
  const auto* inputIds = dataStructure.getDataAs<Int32Array>(idsPath);
  REQUIRE(inputIds != nullptr);
  scope.requireExpectedStore(*inputIds);

  FillBadDataFilter filter;
  Arguments args;
  args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(minDefectSize));
  args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(storeAsNewPhase));
  args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(idsPath));
  args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(phasesPath));
  args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(geometryPath));

  const auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto execute = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(execute.result);
  for(const auto& path : {idsPath, phasesPath})
  {
    CAPTURE(path.toString());
    const auto* reference = expected.getDataAs<Int32Array>(path);
    const auto* actual = dataStructure.getDataAs<Int32Array>(path);
    REQUIRE(reference != nullptr);
    REQUIRE(actual != nullptr);
    REQUIRE(actual->getTupleShape() == reference->getTupleShape());
    REQUIRE(actual->getComponentShape() == reference->getComponentShape());
    UnitTest::CompareDataArrays<int32>(*reference, *actual);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// These paths select the FillBadData exemplar archive.
const std::string k_ArchiveName = "fill_bad_data_exemplars.tar.gz";
const std::string k_DataDirName = "fill_bad_data_exemplars";
const fs::path k_DataDir = fs::path(unit_test::k_TestFilesDir.view()) / k_DataDirName;
const fs::path k_ExemplarFile = k_DataDir / "fill_bad_data.dream3d";

// These dimensions define the large generated fixtures.
constexpr usize k_Dim = 200;
constexpr usize k_BlockSize = 25;
constexpr int32 k_MinDefectSize = 50;
} // namespace

TEST_CASE("SimplnxCore::FillBadData_SmallIN100", "[Core][FillBadDataFilter]")
{
  // Load the application and plugins.
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "6_5_fill_bad_data.tar.gz", "6_5_fill_bad_data");
  // Load the exemplar output.
  auto exemplarFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/6_5_exemplar.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Load the Small IN100 input.
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_fill_bad_data/6_5_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    // Configure the filter arguments.
    FillBadDataFilter filter;
    Arguments args;

    args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(1000));
    args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));
    args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(Constants::k_FeatureIdsArrayPath));
    args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(Constants::k_PhasesArrayPath));

    args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
    args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));

    // Preflight the filter and check the result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  UnitTest::CompareExemplarToGeneratedData(dataStructure, exemplarDataStructure, Constants::k_CellAttributeMatrix, Constants::k_DataContainer);

// The optional output supports manual inspection.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_fill_bad_data.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData::Test01_SingleSmallDefect", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  CheckFillExemplar("test_01", 20, false);
}

TEST_CASE("SimplnxCore::FillBadData::Test02_SingleLargeDefect", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  CheckFillExemplar("test_02", 20, false);
}

TEST_CASE("SimplnxCore::FillBadData::Test03_ThresholdBoundary", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  CheckFillExemplar("test_03", 25, false);
}

TEST_CASE("SimplnxCore::FillBadData::Test04_MultipleSmallDefects", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  CheckFillExemplar("test_04", 50, false);
}

TEST_CASE("SimplnxCore::FillBadData::Test05_MixedSmallAndLarge", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  CheckFillExemplar("test_05", 50, false);
}

TEST_CASE("SimplnxCore::FillBadData::Test06_SingleVoxelDefects", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  CheckFillExemplar("test_06", 10, false);
}

TEST_CASE("SimplnxCore::FillBadData::Test07_DefectsAtBoundaries", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  CheckFillExemplar("test_07", 20, false);
}

TEST_CASE("SimplnxCore::FillBadData::Test11_NeighborTieBreaking", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  CheckFillExemplar("test_11", 10, false);
}

TEST_CASE("SimplnxCore::FillBadData::Test13_StoreAsNewPhase", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  CheckFillExemplar("test_13", 20, true);
}

TEST_CASE("SimplnxCore::FillBadDataFilter: Preflight Feature Data Modification Warning", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  BuildFillBadDataTestData(dataStructure, 2, 2, 2, 1);

  FillBadDataFilter filter;
  Arguments args;
  args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(1));
  args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"DataContainer"})));
  args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "FeatureIds"})));
  args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "Phases"})));
  args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));

  const auto preflightResult = filter.preflight(dataStructure, args);
  CellDataAliasTest::CheckFeatureDataWarning(preflightResult);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadDataFilter:: Invalid Preflight Min Defect Size", "[Core][FillBadDataFilter]")
{
  DataStructure dataStructure;
  const DataPath k_GeomPath({"DataContainer"});
  const DataPath k_FeatureIdsPath({"DataContainer", "CellData", "FeatureIds"});

  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "DataContainer");
  imageGeomPtr->setDimensions({1, 1, 1});
  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, 1, 1}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellDataPtr);
  UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", {1, 1, 1}, {1}, cellDataPtr->getId());

  FillBadDataFilter filter;
  Arguments args;
  args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(0));
  args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
  args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -16500);
}

// A volume with no good neighbor cannot be filled.
// A retained component stays at zero. A component marked for filling has no donor and stays at -1.
TEST_CASE("SimplnxCore::FillBadData::AllBadData_TerminatesWithoutHang", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure;
  const DataPath k_GeomPath({"DataContainer"});
  const DataPath k_CellDataPath = k_GeomPath.createChildPath("CellData");
  const DataPath k_FeatureIdsPath = k_CellDataPath.createChildPath("FeatureIds");
  const DataPath k_PhasesPath = k_CellDataPath.createChildPath("Phases");

  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "DataContainer");
  imageGeomPtr->setDimensions({3, 3, 1});
  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, "CellData", {1, 3, 3}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellDataPtr);
  auto* featureIds = UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", {1, 3, 3}, {1}, cellDataPtr->getId());
  auto* phases = UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", {1, 3, 3}, {1}, cellDataPtr->getId());
  // Every voxel is bad data (featureId 0 -> marked for fill); there is no good neighbor to fill from.
  featureIds->fill(0);
  phases->fill(1);

  // All nine voxels form one component. Only threshold 10 sends them to the fill loop.
  const auto [threshold, expectedId, expectWarning] = GENERATE(table<int32, int32, bool>({{1, 0, false}, {10, -1, true}}));
  CAPTURE(threshold);
  FillBadDataFilter filter;
  Arguments args;
  args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(threshold));
  args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
  args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesPath));
  args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  bool sawNoDonorWarning = false;
  const IFilter::MessageHandler handler{[&sawNoDonorWarning](const IFilter::Message& message) {
    if(message.type == IFilter::Message::Type::Warning && message.message.find("9 bad-data voxel(s) could not be filled") != std::string::npos)
    {
      sawNoDonorWarning = true;
    }
  }};
  auto executeResult = scope.executeFilter(filter, dataStructure, args, nullptr, handler);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  CHECK(sawNoDonorWarning == expectWarning);
  const auto* finalIds = dataStructure.getDataAs<Int32Array>(k_FeatureIdsPath);
  const auto* finalPhases = dataStructure.getDataAs<Int32Array>(k_PhasesPath);
  REQUIRE(finalIds != nullptr);
  REQUIRE(finalPhases != nullptr);
  REQUIRE(finalIds->getNumberOfTuples() == 9);
  REQUIRE(finalPhases->getNumberOfTuples() == 9);
  REQUIRE(finalIds->getNumberOfComponents() == 1);
  REQUIRE(finalPhases->getNumberOfComponents() == 1);
  for(usize index = 0; index < 9; ++index)
  {
    CAPTURE(index);
    CHECK((*finalIds)[index] == expectedId);
    CHECK((*finalPhases)[index] == 1);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData: real HDF5 cross-slice defect oracle", "[SimplnxCore][FillBadDataFilter][.OocStoreContract]")
{
  UnitTest::LoadPlugins();
  REQUIRE(Application::Instance()->getIOManager("HDF5-OOC") != nullptr);
  const bool newPhase = GENERATE(false, true);
  const PreferencesSentinel preferencesSentinel(DataStorageMode::ForceOutOfCore, 1);
  DataStructure dataStructure;
  BuildFillBadDataTestData(dataStructure, 5, 5, 5, 5);
  const DataPath featureIdsPath({"DataContainer", "CellData", "FeatureIds"});
  const DataPath phasesPath({"DataContainer", "CellData", "Phases"});
  const DataPath payloadPath({"DataContainer", "CellData", "Payload"});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(featureIdsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(phasesPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(payloadPath.getParent()));
  auto& featureIds = dataStructure.getDataRefAs<Int32Array>(featureIdsPath);
  auto& phases = dataStructure.getDataRefAs<Int32Array>(phasesPath);
  auto& cells = dataStructure.getDataRefAs<AttributeMatrix>(payloadPath.getParent());
  auto payloadStore = DataStoreUtilities::CreateDataStore<float32>(dataStructure, payloadPath, cells.getShape(), {2});
  auto* payloadPtr = Float32Array::Create(dataStructure, "Payload", payloadStore, cells.getId());
  REQUIRE(payloadPtr != nullptr);
  featureIds.fill(1);
  phases.fill(1);
  for(usize voxelIdx = 0; voxelIdx < 125; voxelIdx++)
  {
    (*payloadPtr)[voxelIdx * 2] = 7.0F;
    (*payloadPtr)[voxelIdx * 2 + 1] = -2.0F;
  }
  // The three-cell component spans Z slices and meets the retention threshold.
  // The isolated diagonal cell has only one voxel and must copy a good tuple.
  for(usize voxelIdx : {37ULL, 62ULL, 87ULL, 56ULL})
  {
    featureIds[voxelIdx] = 0;
    phases[voxelIdx] = 0;
    (*payloadPtr)[voxelIdx * 2] = -5.0F;
    (*payloadPtr)[voxelIdx * 2 + 1] = -6.0F;
  }
  REQUIRE(featureIds.getDataStoreRef().getDataFormat() == "HDF5-OOC");
  REQUIRE(phases.getDataStoreRef().getDataFormat() == "HDF5-OOC");
  REQUIRE(payloadStore->getDataFormat() == "HDF5-OOC");
  FillBadDataFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(3));
  args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(newPhase));
  args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsPath));
  args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(phasesPath));
  args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"DataContainer"})));
  const auto before = GetAlgorithmPathExecutionCounts();
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const auto after = GetAlgorithmPathExecutionCounts();
  REQUIRE(after.OutOfCoreOnOutOfCoreStore == before.OutOfCoreOnOutOfCoreStore + 1);
  REQUIRE(after.InCore == before.InCore);
  const auto* finalIds = dataStructure.getDataAs<Int32Array>(featureIdsPath);
  const auto* finalPhases = dataStructure.getDataAs<Int32Array>(phasesPath);
  const auto* finalPayload = dataStructure.getDataAs<Float32Array>(payloadPath);
  REQUIRE(finalIds != nullptr);
  REQUIRE(finalPhases != nullptr);
  REQUIRE(finalPayload != nullptr);
  REQUIRE(finalIds->getNumberOfTuples() == 125);
  REQUIRE(finalIds->getNumberOfComponents() == 1);
  REQUIRE(finalPhases->getNumberOfTuples() == 125);
  REQUIRE(finalPhases->getNumberOfComponents() == 1);
  REQUIRE(finalPayload->getNumberOfTuples() == 125);
  REQUIRE(finalPayload->getNumberOfComponents() == 2);
  for(usize voxelIdx = 0; voxelIdx < 125; voxelIdx++)
  {
    const bool retainedDefect = voxelIdx == 37 || voxelIdx == 62 || voxelIdx == 87;
    INFO("voxel " << voxelIdx);
    REQUIRE((*finalIds)[voxelIdx] == (retainedDefect ? 0 : 1));
    REQUIRE((*finalPhases)[voxelIdx] == (retainedDefect ? (newPhase ? 2 : 0) : 1));
    REQUIRE((*finalPayload)[voxelIdx * 2] == (retainedDefect ? -5.0F : 7.0F));
    REQUIRE((*finalPayload)[voxelIdx * 2 + 1] == (retainedDefect ? -6.0F : -2.0F));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadDataFilter: SIMPL Backwards Compatibility", "[SimplnxCore][FillBadDataFilter][BackwardsCompatibility]")
{
  const auto version = GENERATE("6_4", "6_5");
  CAPTURE(version);
  const fs::path fixturePath = fs::path(unit_test::k_SourceDir.view()) / "test" / "simpl_conversion" / version / "FillBadDataFilter.json";
  const auto args = UnitTest::LoadSIMPLConversionArguments(fixturePath, FilterTraits<FillBadDataFilter>::uuid);
  CHECK(args.value<int32>(FillBadDataFilter::k_MinAllowedDefectSize_Key) == 5);
  CHECK(args.value<bool>(FillBadDataFilter::k_StoreAsNewPhase_Key) == true);
  CHECK(args.value<DataPath>(FillBadDataFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
  CHECK(args.value<DataPath>(FillBadDataFilter::k_SelectedCellDataGroup_Key) == DataPath({"DataContainer", "CellData"}));
  CHECK(args.value<DataPath>(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
  CHECK(args.value<DataPath>(FillBadDataFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "Phases"}));
  CHECK(args.value<std::vector<DataPath>>(FillBadDataFilter::k_IgnoredDataArrayPaths_Key) ==
        std::vector<DataPath>{DataPath({"DataContainer", "CellData", "Confidence"}), DataPath({"DataContainer", "CellData", "Intensity"})});
}

TEST_CASE("SimplnxCore::FillBadData: 200x200x200 Correctness", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  // int32 1-comp => 200*200*4 = 160,000 bytes/slice

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, k_ArchiveName, k_DataDirName);
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);

  std::string testName = GENERATE("NoNewPhase", "NewPhase");
  DYNAMIC_SECTION("Variant: " << testName)
  {
    const bool storeAsNewPhase = (testName == "NewPhase");

    DataStructure dataStructure;
    BuildFillBadDataTestData(dataStructure, k_Dim, k_Dim, k_Dim, k_BlockSize, true);

    scope.requireExpectedStore(dataStructure.getDataRefAs<Int32Array>(DataPath({"DataContainer", "CellData", "FeatureIds"})));

    FillBadDataFilter filter;
    Arguments args;
    args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(k_MinDefectSize));
    args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(storeAsNewPhase));
    args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "FeatureIds"})));
    args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "Phases"})));
    args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
    args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"DataContainer"})));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    // Compare against exemplar
    const std::string exemplarGeomName = "DataContainer_" + testName + "_Exemplar";
    const DataPath exemplarFeatureIdsPath({exemplarGeomName, "CellData", "FeatureIds"});
    const DataPath exemplarPhasesPath({exemplarGeomName, "CellData", "Phases"});

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"DataContainer", "CellData", "FeatureIds"})));
    REQUIRE_NOTHROW(exemplarDS.getDataRefAs<Int32Array>(exemplarFeatureIdsPath));
    CompareDataArrays<int32>(exemplarDS.getDataRefAs<Int32Array>(exemplarFeatureIdsPath), dataStructure.getDataRefAs<Int32Array>(DataPath({"DataContainer", "CellData", "FeatureIds"})));

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"DataContainer", "CellData", "Phases"})));
    REQUIRE_NOTHROW(exemplarDS.getDataRefAs<Int32Array>(exemplarPhasesPath));
    CompareDataArrays<int32>(exemplarDS.getDataRefAs<Int32Array>(exemplarPhasesPath), dataStructure.getDataRefAs<Int32Array>(DataPath({"DataContainer", "CellData", "Phases"})));

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::FillBadData: 200x200x200 Ignored Arrays", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();
  // SIMPLNX_TEST_ALGORITHM_PATH selects the algorithm scenarios.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  // int32 1-comp => 200*200*4 = 160,000 bytes/slice

  constexpr int32 k_Sentinel = -999;

  DataStructure dataStructure;
  BuildFillBadDataTestData(dataStructure, k_Dim, k_Dim, k_Dim, k_BlockSize, false);

  scope.requireExpectedStore(dataStructure.getDataRefAs<Int32Array>(DataPath({"DataContainer", "CellData", "FeatureIds"})));

  // Add an extra "IgnoredArray" filled with a sentinel value
  auto& cellAM = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"DataContainer", "CellData"}));
  const DataPath ignoredArrayPath = DataPath({"DataContainer", "CellData", "IgnoredArray"});
  auto ignoredDataStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, ignoredArrayPath, cellAM.getShape(), {1});
  auto* ignoredArray = DataArray<int32>::Create(dataStructure, "IgnoredArray", ignoredDataStore, cellAM.getId());
  auto& ignoredStore = ignoredArray->getDataStoreRef();
  ignoredStore.fill(k_Sentinel);

  // Record bad-voxel count before fill using bulk reads
  const auto& featureIdsBefore = dataStructure.getDataRefAs<Int32Array>(DataPath({"DataContainer", "CellData", "FeatureIds"}));
  const auto& fidsStore = featureIdsBefore.getDataStoreRef();
  const usize totalTuples = fidsStore.getNumberOfTuples();
  usize badCount = 0;
  {
    constexpr usize kChunk = 40000;
    auto buf = std::make_unique<int32[]>(kChunk);
    for(usize off = 0; off < totalTuples; off += kChunk)
    {
      const usize count = std::min(kChunk, totalTuples - off);
      auto fidsStoreReadResult = fidsStore.copyIntoBuffer(off, nonstd::span<int32>(buf.get(), count));
      SIMPLNX_RESULT_REQUIRE_VALID(fidsStoreReadResult);
      for(usize i = 0; i < count; i++)
      {
        if(buf[i] == 0)
        {
          badCount++;
        }
      }
    }
  }
  REQUIRE(badCount > 0);

  {
    FillBadDataFilter filter;
    Arguments args;
    args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(50));
    args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));
    args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "FeatureIds"})));
    args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "Phases"})));
    args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"DataContainer"})));

    // Include the IgnoredArray in the ignored paths
    MultiArraySelectionParameter::ValueType ignoredPaths = {DataPath({"DataContainer", "CellData", "IgnoredArray"})};
    args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(ignoredPaths));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Verify: FeatureIds has no zeros (all scattered bad voxels filled) — bulk read
  const auto& featureIdsAfter = dataStructure.getDataRefAs<Int32Array>(DataPath({"DataContainer", "CellData", "FeatureIds"}));
  {
    const auto& fidsAfterStore = featureIdsAfter.getDataStoreRef();
    constexpr usize kChunk = 40000;
    auto buf = std::make_unique<int32[]>(kChunk);
    for(usize off = 0; off < totalTuples; off += kChunk)
    {
      const usize count = std::min(kChunk, totalTuples - off);
      auto fidsAfterStoreReadResult = fidsAfterStore.copyIntoBuffer(off, nonstd::span<int32>(buf.get(), count));
      SIMPLNX_RESULT_REQUIRE_VALID(fidsAfterStoreReadResult);
      for(usize i = 0; i < count; i++)
      {
        if(buf[i] == 0)
        {
          UNSCOPED_INFO(fmt::format("FeatureIds still zero at index {}", off + i));
          REQUIRE(false);
        }
      }
    }
  }

  // Verify: IgnoredArray is completely unchanged (sentinel at every voxel) — bulk read
  const auto& ignoredAfter = dataStructure.getDataRefAs<Int32Array>(DataPath({"DataContainer", "CellData", "IgnoredArray"}));
  {
    const auto& ignoredAfterStore = ignoredAfter.getDataStoreRef();
    constexpr usize kChunk = 40000;
    auto buf = std::make_unique<int32[]>(kChunk);
    for(usize off = 0; off < totalTuples; off += kChunk)
    {
      const usize count = std::min(kChunk, totalTuples - off);
      auto ignoredAfterStoreReadResult = ignoredAfterStore.copyIntoBuffer(off, nonstd::span<int32>(buf.get(), count));
      SIMPLNX_RESULT_REQUIRE_VALID(ignoredAfterStoreReadResult);
      for(usize i = 0; i < count; i++)
      {
        if(buf[i] != k_Sentinel)
        {
          UNSCOPED_INFO(fmt::format("IgnoredArray changed at index {}: expected {} got {}", off + i, k_Sentinel, buf[i]));
          REQUIRE(false);
        }
      }
    }
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadDataFilter: Strict Feature Ids", "[SimplnxCore][FillBadDataFilter][StrictFeatureIds]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  const auto& cells = CellDataAliasTest::BuildStrictInput(dataStructure);

  FillBadDataFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(CellDataAliasTest::k_StrictGeometry));
  args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(CellDataAliasTest::k_StrictIds));
  args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));

  int32 expectedCode = 0;
  SECTION("Correct Cell Attribute Matrix")
  {
    args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{CellDataAliasTest::k_StrictIds}));
  }
  SECTION("Feature Ids in another Attribute Matrix with the same tuple shape")
  {
    args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(CellDataAliasTest::k_OtherValues));
    expectedCode = -16501;
  }
  SECTION("Ignored array in another Attribute Matrix")
  {
    args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{CellDataAliasTest::k_OtherValues}));
    expectedCode = -16503;
  }
  SECTION("Modified Cell Phases in another Attribute Matrix")
  {
    args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(true));
    args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(CellDataAliasTest::k_OtherValues));
    expectedCode = -16502;
  }
  SECTION("Unused Cell Phases in another Attribute Matrix")
  {
    args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(CellDataAliasTest::k_OtherValues));
  }
  SECTION("Modified Cell Phases in the Cell Attribute Matrix")
  {
    const auto phasesPath = CellDataAliasTest::k_StrictCells.createChildPath("Phases");
    REQUIRE(Int32Array::Create(dataStructure, "Phases", std::make_shared<DataStore<int32>>(cells.getShape(), ShapeType{1}, 1), cells.getId()) != nullptr);
    args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(true));
    args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(phasesPath));
  }
  const auto result = filter.preflight(dataStructure, args);
  CellDataAliasTest::CheckStrictPreflight(result, expectedCode);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadDataFilter: Ignored Array Cell AM Alias", "[SimplnxCore][FillBadDataFilter][StrictFeatureIds]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  CellDataAliasTest::BuildAliasInput(dataStructure);

  // The connected pair of zero IDs stays at the threshold and fills one cell below it.
  const auto [minimumSize, expectedIds] = GENERATE(table<int32, std::vector<int32>>({{2, {1, 0, 0, 1}}, {3, {1, 1, 1, 1}}}));
  CAPTURE(minimumSize);
  FillBadDataFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(CellDataAliasTest::k_AliasGeometry));
  args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(CellDataAliasTest::k_AliasCells.createChildPath("Ids")));
  args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{CellDataAliasTest::k_AliasCells.createChildPath("Ignored")}));
  args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(minimumSize));
  const auto preflight = filter.preflight(dataStructure, args);
  CellDataAliasTest::CheckAliasPreflight(preflight);
  const auto execute = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(execute.result);
  CellDataAliasTest::CheckAliasOutput(dataStructure, expectedIds);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FillBadData: Generate Test Data", "[Core][FillBadDataFilter][.GenerateTestData]")
{
  UnitTest::LoadPlugins();

  const auto outputDir = fs::path(fmt::format("{}/generated_test_data/fill_bad_data", unit_test::k_BinaryTestOutputDir));
  fs::create_directories(outputDir);

  // 200^3 input data with large defect (full z=k_Dim/2 plane)
  {
    DataStructure ds;
    BuildFillBadDataTestData(ds, k_Dim, k_Dim, k_Dim, k_BlockSize, true);
    UnitTest::WriteTestDataStructure(ds, outputDir / "input.dream3d");
  }
}
