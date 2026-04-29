#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/ReplaceElementAttributesWithNeighborValuesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <filesystem>

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
} // namespace

TEST_CASE("SimplnxCore::ReplaceElementAttributesWithNeighborValuesFilter: Small Correctness", "[SimplnxCore][ReplaceElementAttributesWithNeighborValuesFilter]")
{
  UnitTest::LoadPlugins();
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  // 20x20x20, EulerAngles (float32, 3-comp) => 20*20*3*4 = 4,800 bytes/slice
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 4800, true);

  auto comparison = GENERATE(0ULL, 1ULL);
  bool loopUntilDone = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));

  std::string compName = (comparison == 0) ? "LessThan" : "GreaterThan";
  std::string loopName = loopUntilDone ? "Loop" : "NoLoop";
  DYNAMIC_SECTION(compName << " " << loopName << " forceOoc: " << forceOocAlgo)
  {
    constexpr float32 k_Threshold = 0.1F;

    DataStructure dataStructure;
    BuildTestData(dataStructure, 20, 20, 20);

    const usize belowCountBefore = CountVoxelsBelowThreshold(dataStructure, k_Threshold, 20, 20, 20);
    const usize aboveCountBefore = CountVoxelsAboveThreshold(dataStructure, k_Threshold, 20, 20, 20);
    REQUIRE(belowCountBefore > 0);
    REQUIRE(aboveCountBefore > 0);

    {
      ReplaceElementAttributesWithNeighborValuesFilter filter;
      Arguments args;
      args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_MinConfidence_Key, std::make_any<float32>(k_Threshold));
      args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_SelectedComparison_Key, std::make_any<ChoicesParameter::ValueType>(comparison));
      args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_Loop_Key, std::make_any<bool>(loopUntilDone));
      args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_ComparisonDataPath, std::make_any<DataPath>(k_ConfidencePath));
      args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));

      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
      auto executeResult = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    }

    const usize belowCountAfter = CountVoxelsBelowThreshold(dataStructure, k_Threshold, 20, 20, 20);
    const usize aboveCountAfter = CountVoxelsAboveThreshold(dataStructure, k_Threshold, 20, 20, 20);

    if(comparison == 0)
    {
      REQUIRE(belowCountAfter <= belowCountBefore);
    }
    else
    {
      REQUIRE(aboveCountAfter <= aboveCountBefore);
    }

    // TODO: Add exemplar comparison after exemplar archive is published

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ReplaceElementAttributesWithNeighborValuesFilter: Generate Test Data", "[SimplnxCore][ReplaceElementAttributesWithNeighborValuesFilter][.GenerateTestData]")
{
  const auto outputDir = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "generated_test_data" / "replace_element_attributes";
  fs::create_directories(outputDir);

  // Small input data (20x20x20)
  {
    DataStructure buildDS;
    BuildTestData(buildDS, 20, 20, 20);
    UnitTest::WriteTestDataStructure(buildDS, outputDir / "small_input.dream3d");
    fmt::print("Generated small input: {}\n", (outputDir / "small_input.dream3d").string());
  }

  // Large input data (200x200x200)
  {
    DataStructure buildDS;
    BuildTestData(buildDS, 200, 200, 200);
    UnitTest::WriteTestDataStructure(buildDS, outputDir / "large_input.dream3d");
    fmt::print("Generated large input: {}\n", (outputDir / "large_input.dream3d").string());
  }
}

TEST_CASE("SimplnxCore::ReplaceElementAttributesWithNeighborValuesFilter: 200x200x200 Large OOC", "[SimplnxCore][ReplaceElementAttributesWithNeighborValuesFilter]")
{
  UnitTest::LoadPlugins();
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  // 200x200x200, EulerAngles (float32, 3-comp) => 200*200*3*4 = 480,000 bytes/slice
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 480000, true);

  DYNAMIC_SECTION("forceOoc: " << forceOocAlgo)
  {
    constexpr float32 k_Threshold = 0.1F;

    DataStructure dataStructure;
    BuildTestData(dataStructure, 200, 200, 200);

    const usize belowCountBefore = CountVoxelsBelowThreshold(dataStructure, k_Threshold, 200, 200, 200);
    REQUIRE(belowCountBefore > 0);

    ReplaceElementAttributesWithNeighborValuesFilter filter;
    Arguments args;
    args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_MinConfidence_Key, std::make_any<float32>(k_Threshold));
    args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_SelectedComparison_Key, std::make_any<ChoicesParameter::ValueType>(0ULL));
    args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_Loop_Key, std::make_any<bool>(true));
    args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_ComparisonDataPath, std::make_any<DataPath>(k_ConfidencePath));
    args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const usize belowCountAfter = CountVoxelsBelowThreshold(dataStructure, k_Threshold, 200, 200, 200);
    REQUIRE(belowCountAfter <= belowCountBefore);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}
