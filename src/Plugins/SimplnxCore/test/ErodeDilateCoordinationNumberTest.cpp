#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/ErodeDilateCoordinationNumberFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
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
const std::string k_GeomName("ImageGeom");
const std::string k_CellDataName("CellData");

const DataPath k_GeomPath({k_GeomName});
const DataPath k_CellDataPath = k_GeomPath.createChildPath(k_CellDataName);
const DataPath k_FeatureIdsPath = k_CellDataPath.createChildPath("FeatureIds");

void BuildTestData(DataStructure& dataStructure, usize dimX, usize dimY, usize dimZ, usize blockSize)
{
  const ShapeType cellTupleShape = {dimZ, dimY, dimX};
  const usize sliceSize = dimX * dimY;

  auto* imageGeom = ImageGeom::Create(dataStructure, k_GeomName);
  imageGeom->setDimensions({dimX, dimY, dimZ});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

  auto* cellAM = AttributeMatrix::Create(dataStructure, k_CellDataName, cellTupleShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  auto featureIdsDataStore = DataStoreUtilities::CreateDataStore<int32>(cellTupleShape, {1}, IDataAction::Mode::Execute);
  auto* featureIdsArray = DataArray<int32>::Create(dataStructure, "FeatureIds", featureIdsDataStore, cellAM->getId());
  auto& featureIdsStore = featureIdsArray->getDataStoreRef();

  auto eulerDataStore = DataStoreUtilities::CreateDataStore<float32>(cellTupleShape, {3}, IDataAction::Mode::Execute);
  auto* eulerArray = DataArray<float32>::Create(dataStructure, "EulerAngles", eulerDataStore, cellAM->getId());
  auto& eulerStore = eulerArray->getDataStoreRef();

  const usize blocksPerDimX = dimX / blockSize;
  const usize blocksPerDimY = dimY / blockSize;

  std::vector<int32> featureIdsBuf(sliceSize);
  std::vector<float32> eulerBuf(sliceSize * 3);

  for(usize z = 0; z < dimZ; z++)
  {
    for(usize y = 0; y < dimY; y++)
    {
      for(usize x = 0; x < dimX; x++)
      {
        const usize inSlice = y * dimX + x;

        usize bx = x / blockSize;
        usize by = y / blockSize;
        usize bz = z / blockSize;
        int32 blockFeatureId = static_cast<int32>(bz * blocksPerDimY * blocksPerDimX + by * blocksPerDimX + bx + 1);

        bool isBad = ((x * 7 + y * 13 + z * 29) % 7 == 0);
        featureIdsBuf[inSlice] = isBad ? 0 : blockFeatureId;

        const usize eIdx = inSlice * 3;
        eulerBuf[eIdx] = static_cast<float32>(x) / static_cast<float32>(dimX);
        eulerBuf[eIdx + 1] = static_cast<float32>(y) / static_cast<float32>(dimY);
        eulerBuf[eIdx + 2] = static_cast<float32>(z) / static_cast<float32>(dimZ);
      }
    }
    const usize zOffset = z * sliceSize;
    featureIdsStore.copyFromBuffer(zOffset, nonstd::span<const int32>(featureIdsBuf.data(), sliceSize));
    eulerStore.copyFromBuffer(zOffset * 3, nonstd::span<const float32>(eulerBuf.data(), sliceSize * 3));
  }
}

usize CountBadVoxels(const DataStructure& dataStructure, usize dimX, usize dimY, usize dimZ)
{
  const auto& featureIds = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath).getDataStoreRef();
  const usize sliceSize = dimX * dimY;
  std::vector<int32> buf(sliceSize);
  usize count = 0;
  for(usize z = 0; z < dimZ; z++)
  {
    featureIds.copyIntoBuffer(z * sliceSize, nonstd::span<int32>(buf.data(), sliceSize));
    for(usize i = 0; i < sliceSize; i++)
    {
      if(buf[i] == 0)
      {
        count++;
      }
    }
  }
  return count;
}
} // namespace

TEST_CASE("SimplnxCore::ErodeDilateCoordinationNumberFilter: Small Correctness", "[SimplnxCore][ErodeDilateCoordinationNumberFilter]")
{
  UnitTest::LoadPlugins();
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  // 20x20x20, EulerAngles (float32, 3-comp) => 20*20*3*4 = 4,800 bytes/slice
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 4800, true);

  DYNAMIC_SECTION("forceOoc: " << forceOocAlgo)
  {
    DataStructure dataStructure;
    BuildTestData(dataStructure, 20, 20, 20, 5);

    const usize badCountBefore = CountBadVoxels(dataStructure, 20, 20, 20);
    REQUIRE(badCountBefore > 0);

    {
      const ErodeDilateCoordinationNumberFilter filter;
      Arguments args;
      args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_CoordinationNumber_Key, std::make_any<int32>(4));
      args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_Loop_Key, std::make_any<bool>(false));
      args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
      args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
      args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));

      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
      auto executeResult = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    }

    const usize badCountAfter = CountBadVoxels(dataStructure, 20, 20, 20);
    REQUIRE(badCountAfter < badCountBefore);

    // TODO: Add exemplar comparison after exemplar archive is published

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ErodeDilateCoordinationNumberFilter: Generate Test Data", "[SimplnxCore][ErodeDilateCoordinationNumberFilter][.GenerateTestData]")
{
  const auto outputDir = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "generated_test_data" / "erode_dilate_coordination_number";
  fs::create_directories(outputDir);

  // Small input data (20x20x20, blockSize=5)
  {
    DataStructure buildDS;
    BuildTestData(buildDS, 20, 20, 20, 5);
    UnitTest::WriteTestDataStructure(buildDS, outputDir / "small_input.dream3d");
    fmt::print("Generated small input: {}\n", (outputDir / "small_input.dream3d").string());
  }

  // Large input data (200x200x200, blockSize=25)
  {
    DataStructure buildDS;
    BuildTestData(buildDS, 200, 200, 200, 25);
    UnitTest::WriteTestDataStructure(buildDS, outputDir / "large_input.dream3d");
    fmt::print("Generated large input: {}\n", (outputDir / "large_input.dream3d").string());
  }
}

TEST_CASE("SimplnxCore::ErodeDilateCoordinationNumberFilter: 200x200x200 Large OOC", "[SimplnxCore][ErodeDilateCoordinationNumberFilter]")
{
  UnitTest::LoadPlugins();
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  // 200x200x200, EulerAngles (float32, 3-comp) => 200*200*3*4 = 480,000 bytes/slice
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 480000, true);

  DYNAMIC_SECTION("forceOoc: " << forceOocAlgo)
  {
    DataStructure dataStructure;
    BuildTestData(dataStructure, 200, 200, 200, 25);

    const usize badCountBefore = CountBadVoxels(dataStructure, 200, 200, 200);
    REQUIRE(badCountBefore > 0);

    const ErodeDilateCoordinationNumberFilter filter;
    Arguments args;
    args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_CoordinationNumber_Key, std::make_any<int32>(4));
    args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_Loop_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
    args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
    args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const usize badCountAfter = CountBadVoxels(dataStructure, 200, 200, 200);
    REQUIRE(badCountAfter < badCountBefore);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}
