#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/ReplaceElementAttributesWithNeighborValuesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
const std::string k_GeomName("DataContainer");
const std::string k_CellDataName("CellData");

const DataPath k_GeomPath({k_GeomName});
const DataPath k_CellDataPath = k_GeomPath.createChildPath(k_CellDataName);
const DataPath k_ConfidencePath = k_CellDataPath.createChildPath("Confidence Index");

/**
 * @brief Builds deterministic confidence, Euler-angle, and phase arrays for OOC tests.
 * @param dataStructure Receives the image geometry and cell arrays.
 * @param dimX Number of cells on the X axis.
 * @param dimY Number of cells on the Y axis.
 * @param dimZ Number of cells on the Z axis.
 */
void BuildTestData(DataStructure& dataStructure, usize dimX, usize dimY, usize dimZ)
{
  const ShapeType cellTupleShape = {dimZ, dimY, dimX};
  const usize sliceSize = dimX * dimY;

  auto* imageGeom = ImageGeom::Create(dataStructure, k_GeomName);
  imageGeom->setDimensions({dimX, dimY, dimZ});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

  auto* cellAM = AttributeMatrix::Create(dataStructure, k_CellDataName, cellTupleShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  auto confDataStore = DataStoreUtilities::CreateDataStore<float32>(dataStructure, k_ConfidencePath, cellTupleShape, {1}, IDataAction::Mode::Execute);
  auto* confArray = DataArray<float32>::Create(dataStructure, "Confidence Index", confDataStore, cellAM->getId());
  auto& confStore = confArray->getDataStoreRef();

  auto eulerDataStore = DataStoreUtilities::CreateDataStore<float32>(dataStructure, k_CellDataPath.createChildPath("EulerAngles"), cellTupleShape, {3}, IDataAction::Mode::Execute);
  auto* eulerArray = DataArray<float32>::Create(dataStructure, "EulerAngles", eulerDataStore, cellAM->getId());
  auto& eulerStore = eulerArray->getDataStoreRef();

  auto phasesDataStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_CellDataPath.createChildPath("Phases"), cellTupleShape, {1}, IDataAction::Mode::Execute);
  auto* phasesArray = DataArray<int32>::Create(dataStructure, "Phases", phasesDataStore, cellAM->getId());
  auto& phasesStore = phasesArray->getDataStoreRef();

  std::vector<float32> confBuf(sliceSize);
  std::vector<float32> eulerBuf(sliceSize * 3);
  std::vector<int32> phasesBuf(sliceSize);

  for(usize z = 0; z < dimZ; z++)
  {
    for(usize y = 0; y < dimY; y++)
    {
      for(usize x = 0; x < dimX; x++)
      {
        const usize inSlice = y * dimX + x;
        phasesBuf[inSlice] = 1;

        confBuf[inSlice] = static_cast<float32>((x * 3 + y * 7 + z * 11) % 100) / 100.0f;

        const usize eIdx = inSlice * 3;
        eulerBuf[eIdx] = static_cast<float32>(x) / static_cast<float32>(dimX);
        eulerBuf[eIdx + 1] = static_cast<float32>(y) / static_cast<float32>(dimY);
        eulerBuf[eIdx + 2] = static_cast<float32>(z) / static_cast<float32>(dimZ);
      }
    }
    const usize zOffset = z * sliceSize;
    confStore.copyFromBuffer(zOffset, nonstd::span<const float32>(confBuf.data(), sliceSize));
    eulerStore.copyFromBuffer(zOffset * 3, nonstd::span<const float32>(eulerBuf.data(), sliceSize * 3));
    phasesStore.copyFromBuffer(zOffset, nonstd::span<const int32>(phasesBuf.data(), sliceSize));
  }
}

/**
 * @brief Counts confidence values that are less than a selected threshold.
 * @param dataStructure Contains the confidence array.
 * @param threshold Exclusive upper limit.
 * @param dimX Number of cells on the X axis.
 * @param dimY Number of cells on the Y axis.
 * @param dimZ Number of cells on the Z axis.
 * @return Number of values below the threshold.
 */
usize CountVoxelsBelowThreshold(const DataStructure& dataStructure, float32 threshold, usize dimX, usize dimY, usize dimZ)
{
  const auto& conf = dataStructure.getDataRefAs<Float32Array>(k_ConfidencePath).getDataStoreRef();
  const usize sliceSize = dimX * dimY;
  std::vector<float32> buf(sliceSize);
  usize count = 0;
  for(usize z = 0; z < dimZ; z++)
  {
    conf.copyIntoBuffer(z * sliceSize, nonstd::span<float32>(buf.data(), sliceSize));
    for(usize i = 0; i < sliceSize; i++)
    {
      if(buf[i] < threshold)
      {
        count++;
      }
    }
  }
  return count;
}

/**
 * @brief Counts confidence values that are greater than a selected threshold.
 * @param dataStructure Contains the confidence array.
 * @param threshold Exclusive lower limit.
 * @param dimX Number of cells on the X axis.
 * @param dimY Number of cells on the Y axis.
 * @param dimZ Number of cells on the Z axis.
 * @return Number of values above the threshold.
 */
usize CountVoxelsAboveThreshold(const DataStructure& dataStructure, float32 threshold, usize dimX, usize dimY, usize dimZ)
{
  const auto& conf = dataStructure.getDataRefAs<Float32Array>(k_ConfidencePath).getDataStoreRef();
  const usize sliceSize = dimX * dimY;
  std::vector<float32> buf(sliceSize);
  usize count = 0;
  for(usize z = 0; z < dimZ; z++)
  {
    conf.copyIntoBuffer(z * sliceSize, nonstd::span<float32>(buf.data(), sliceSize));
    for(usize i = 0; i < sliceSize; i++)
    {
      if(buf[i] > threshold)
      {
        count++;
      }
    }
  }
  return count;
}

const DataPath k_ConfidenceIndexPath = k_CellAttributeMatrix.createChildPath(Constants::k_Confidence_Index);
const std::string k_ExemplarDataContainer2("DataContainer");

const std::string k_SyntheticImageGeomName("Image3D");
const std::string k_SyntheticCellAMName("CellData");
const std::string k_SyntheticConfName("Confidence Index");
const std::string k_SyntheticMarkerName("Marker");
const DataPath k_SyntheticConfPath({k_SyntheticImageGeomName, k_SyntheticCellAMName, k_SyntheticConfName});
const DataPath k_SyntheticMarkerPath({k_SyntheticImageGeomName, k_SyntheticCellAMName, k_SyntheticMarkerName});

/**
 * @brief Builds a 3-cubed fixture that exposes the selected neighbor for each replacement.
 * @param goodValue Confidence value for retained cells.
 * @param badValue Confidence value for cells that require replacement.
 * @param badIndices Linear indices that receive badValue.
 * @return A DataStructure with confidence and marker arrays.
 *
 * Each marker starts with its linear index. A replaced marker therefore
 * identifies the exact neighbor that supplied the copied tuple.
 */
DataStructure BuildSyntheticDataStructure(float32 goodValue, float32 badValue, const std::vector<usize>& badIndices)
{
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, k_SyntheticImageGeomName);
  imageGeom->setDimensions({3, 3, 3});

  auto* cellAM = AttributeMatrix::Create(dataStructure, k_SyntheticCellAMName, {3, 3, 3}, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  auto* confArray = UnitTest::CreateTestDataArray<float32>(dataStructure, k_SyntheticConfName, {3, 3, 3}, {1}, cellAM->getId());
  auto* markerArray = UnitTest::CreateTestDataArray<int32>(dataStructure, k_SyntheticMarkerName, {3, 3, 3}, {1}, cellAM->getId());

  auto& confStore = confArray->getDataStoreRef();
  auto& markerStore = markerArray->getDataStoreRef();
  for(usize i = 0; i < confStore.getNumberOfTuples(); i++)
  {
    confStore[i] = goodValue;
    markerStore[i] = static_cast<int32>(i);
  }
  for(usize idx : badIndices)
  {
    confStore[idx] = badValue;
  }
  return dataStructure;
}
} // namespace

// These hidden cases generate small and large OOC fixtures.

TEST_CASE("SimplnxCore::ReplaceElementAttributesWithNeighborValuesFilter: Generate Test Data", "[SimplnxCore][ReplaceElementAttributesWithNeighborValuesFilter][.GenerateTestData]")
{
  const auto outputDir = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "generated_test_data" / "replace_element_attributes";
  fs::create_directories(outputDir);

  // The small fixture supports focused correctness tests.
  {
    DataStructure buildDS;
    BuildTestData(buildDS, 20, 20, 20);
    UnitTest::WriteTestDataStructure(buildDS, outputDir / "small_input.dream3d");
  }

  // The large fixture exercises bounded OOC processing.
  {
    DataStructure buildDS;
    BuildTestData(buildDS, 200, 200, 200);
    UnitTest::WriteTestDataStructure(buildDS, outputDir / "large_input.dream3d");
  }
}

// The remaining cases verify exemplars, synthetic edge cases, and conversion.
TEST_CASE("SimplnxCore::ReplaceElementAttributesWithNeighborValuesFilter", "[SimplnxCore][ReplaceElementAttributesWithNeighborValuesFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_replace_element_attributes_with_neighbor.tar.gz",
                                                              "6_6_replace_element_attributes_with_neighbor");

  // Load the exemplar and its related input fixture.
  auto exemplarFilePath = fs::path(fmt::format("{}/TestFiles/6_6_replace_element_attributes_with_neighbor/6_6_replace_element_attributes_with_neighbor.dream3d", unit_test::k_DREAM3DDataDir));
  DataStructure exemplarDataStructure = nx::core::UnitTest::LoadDataStructure(exemplarFilePath);

  // Load the input fixture before executing the filter.
  auto baseDataFilePath = fs::path(fmt::format("{}/TestFiles/6_6_replace_element_attributes_with_neighbor/6_6_replace_element_attributes_with_neighbor.dream3d", unit_test::k_DREAM3DDataDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    // Configure the filter for the exemplar comparison.
    ReplaceElementAttributesWithNeighborValuesFilter filter;
    Arguments args;

    // Confidence selects replacements, and all cell arrays copy with each tuple.
    args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_MinConfidence_Key, std::make_any<float32>(0.1F));
    args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_SelectedComparison_Key, std::make_any<ChoicesParameter::ValueType>(0));
    args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_Loop_Key, std::make_any<bool>(true));
    args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_ComparisonDataPath, std::make_any<DataPath>(k_ConfidenceIndexPath));
    args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));

    // Preflight must accept the complete cell-array selection.
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execution must reproduce the exemplar arrays.
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  UnitTest::CompareExemplarToGeneratedData(dataStructure, exemplarDataStructure, k_CellAttributeMatrix, k_ExemplarDataContainer2);

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/7_0_replace_element_attributes_with_neighbor.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ReplaceElementAttributesWithNeighborValuesFilter: Synthetic neighbor replacement", "[SimplnxCore][ReplaceElementAttributesWithNeighborValuesFilter]")
{
  // The center has six in-bounds neighbors. Each opposite corner has three missing neighbors.
  // Together, these cells exercise both sides of each neighbor-boundary condition.
  const std::vector<usize> badIndices = {0, 13, 26};
  constexpr float32 k_Threshold = 0.5F;

  SECTION("LessThan replaces low voxels and loops until none remain")
  {
    constexpr float32 k_Good = 0.9F;
    constexpr float32 k_Bad = 0.1F;
    DataStructure dataStructure = BuildSyntheticDataStructure(k_Good, k_Bad, badIndices);

    ReplaceElementAttributesWithNeighborValuesFilter filter;
    Arguments args;
    args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_MinConfidence_Key, std::make_any<float32>(k_Threshold));
    args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_SelectedComparison_Key, std::make_any<ChoicesParameter::ValueType>(0));
    args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_Loop_Key, std::make_any<bool>(true));
    args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_ComparisonDataPath, std::make_any<DataPath>(k_SyntheticConfPath));
    args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({k_SyntheticImageGeomName})));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(k_SyntheticConfPath));
    const auto& confStore = dataStructure.getDataRefAs<Float32Array>(k_SyntheticConfPath).getDataStoreRef();
    // Every low-confidence cell must receive a value that meets the threshold.
    for(usize i = 0; i < confStore.getNumberOfTuples(); i++)
    {
      REQUIRE(confStore[i] >= k_Threshold);
    }
    for(usize idx : badIndices)
    {
      REQUIRE(confStore[idx] == Approx(k_Good));
    }
    // Tuple replacement also copies the marker array from the selected neighbor.
    // Thus, the center marker must differ from its original linear index.
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_SyntheticMarkerPath));
    const auto& markerStore = dataStructure.getDataRefAs<Int32Array>(k_SyntheticMarkerPath).getDataStoreRef();
    REQUIRE(markerStore[13] != 13);
  }

  SECTION("GreaterThan replaces high voxels and loops until none remain")
  {
    constexpr float32 k_Good = 0.1F;
    constexpr float32 k_Bad = 0.9F;
    DataStructure dataStructure = BuildSyntheticDataStructure(k_Good, k_Bad, badIndices);

    ReplaceElementAttributesWithNeighborValuesFilter filter;
    Arguments args;
    args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_MinConfidence_Key, std::make_any<float32>(k_Threshold));
    args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_SelectedComparison_Key, std::make_any<ChoicesParameter::ValueType>(1));
    args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_Loop_Key, std::make_any<bool>(true));
    args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_ComparisonDataPath, std::make_any<DataPath>(k_SyntheticConfPath));
    args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({k_SyntheticImageGeomName})));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(k_SyntheticConfPath));
    const auto& confStore = dataStructure.getDataRefAs<Float32Array>(k_SyntheticConfPath).getDataStoreRef();
    for(usize i = 0; i < confStore.getNumberOfTuples(); i++)
    {
      REQUIRE(confStore[i] <= k_Threshold);
    }
    for(usize idx : badIndices)
    {
      REQUIRE(confStore[idx] == Approx(k_Good));
    }
  }

  SECTION("loop disabled performs a single pass")
  {
    constexpr float32 k_Good = 0.9F;
    constexpr float32 k_Bad = 0.1F;
    DataStructure dataStructure = BuildSyntheticDataStructure(k_Good, k_Bad, {13});

    ReplaceElementAttributesWithNeighborValuesFilter filter;
    Arguments args;
    args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_MinConfidence_Key, std::make_any<float32>(k_Threshold));
    args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_SelectedComparison_Key, std::make_any<ChoicesParameter::ValueType>(0));
    args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_Loop_Key, std::make_any<bool>(false));
    args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_ComparisonDataPath, std::make_any<DataPath>(k_SyntheticConfPath));
    args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({k_SyntheticImageGeomName})));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(k_SyntheticConfPath));
    const auto& confStore = dataStructure.getDataRefAs<Float32Array>(k_SyntheticConfPath).getDataStoreRef();
    // One pass fills the isolated interior cell because all face neighbors are good.
    REQUIRE(confStore[13] == Approx(k_Good));
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("bad voxel surrounded only by bad neighbors is not replaced (compare1=false skip)")
  {
    constexpr float32 k_Good = 0.9F;
    constexpr float32 k_Bad = 0.1F;
    // Center cell 13 and its six face neighbors all start below the threshold.
    // The scan finds no source for the center, so its best-neighbor index stays -1.
    // Each surrounding cell has an exterior good neighbor and is replaced in the same pass.
    const std::vector<usize> clusterIndices = {4, 10, 12, 13, 14, 16, 22};
    DataStructure dataStructure = BuildSyntheticDataStructure(k_Good, k_Bad, clusterIndices);

    ReplaceElementAttributesWithNeighborValuesFilter filter;
    Arguments args;
    args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_MinConfidence_Key, std::make_any<float32>(k_Threshold));
    args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_SelectedComparison_Key, std::make_any<ChoicesParameter::ValueType>(0));
    args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_Loop_Key, std::make_any<bool>(false));
    args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_ComparisonDataPath, std::make_any<DataPath>(k_SyntheticConfPath));
    args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({k_SyntheticImageGeomName})));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(k_SyntheticConfPath));
    const auto& confStore = dataStructure.getDataRefAs<Float32Array>(k_SyntheticConfPath).getDataStoreRef();

    // The center has no selected source, so its value must remain unchanged.
    REQUIRE(confStore[13] == Approx(k_Bad));

    // Each surrounding cell has an exterior source, so every surrounding value changes.
    for(usize idx : {4UL, 10UL, 12UL, 14UL, 16UL, 22UL})
    {
      REQUIRE(confStore[idx] == Approx(k_Good));
    }

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ReplaceElementAttributesWithNeighborValuesFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ReplaceElementAttributesWithNeighborValuesFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ReplaceElementAttributesWithNeighborValuesFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ReplaceElementAttributesWithNeighborValuesFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ReplaceElementAttributesWithNeighborValuesFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<float32>(ReplaceElementAttributesWithNeighborValuesFilter::k_MinConfidence_Key) == 2.5f);
      CHECK(args.value<ChoicesParameter::ValueType>(ReplaceElementAttributesWithNeighborValuesFilter::k_SelectedComparison_Key) == 0);
      CHECK(args.value<bool>(ReplaceElementAttributesWithNeighborValuesFilter::k_Loop_Key) == true);
      CHECK(args.value<DataPath>(ReplaceElementAttributesWithNeighborValuesFilter::k_ComparisonDataPath) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ReplaceElementAttributesWithNeighborValuesFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
    }
  }
}
