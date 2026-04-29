#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/ErodeDilateMaskFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <filesystem>
#include <memory>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
constexpr ChoicesParameter::ValueType k_Dilate = 0ULL;
constexpr ChoicesParameter::ValueType k_Erode = 1ULL;

const std::string k_GeomName("ImageGeom");
const std::string k_CellDataName("CellData");

const DataPath k_GeomPath({k_GeomName});
const DataPath k_CellDataPath = k_GeomPath.createChildPath(k_CellDataName);
const DataPath k_MaskPath = k_CellDataPath.createChildPath("Mask");

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

  auto maskDataStore = DataStoreUtilities::CreateDataStore<bool>(dataStructure, k_MaskPath, cellTupleShape, {1}, IDataAction::Mode::Execute);
  auto* maskArray = DataArray<bool>::Create(dataStructure, "Mask", maskDataStore, cellAM->getId());
  auto& maskStore = maskArray->getDataStoreRef();

  // Use Z-slice buffered writes for OOC efficiency (bool needs raw array, not vector<bool>)
  auto maskBuf = std::make_unique<bool[]>(sliceSize);

  for(usize z = 0; z < dimZ; z++)
  {
    for(usize y = 0; y < dimY; y++)
    {
      for(usize x = 0; x < dimX; x++)
      {
        const usize inSlice = y * dimX + x;
        maskBuf[inSlice] = ((x * 7 + y * 13 + z * 29) % 3 != 0);
      }
    }
    maskStore.copyFromBuffer(z * sliceSize, nonstd::span<const bool>(maskBuf.get(), sliceSize));
  }
}

usize CountTrueVoxels(const DataStructure& dataStructure, usize dimX, usize dimY, usize dimZ)
{
  const auto& mask = dataStructure.getDataRefAs<BoolArray>(k_MaskPath).getDataStoreRef();
  const usize sliceSize = dimX * dimY;
  auto buf = std::make_unique<bool[]>(sliceSize);
  usize count = 0;
  for(usize z = 0; z < dimZ; z++)
  {
    mask.copyIntoBuffer(z * sliceSize, nonstd::span<bool>(buf.get(), sliceSize));
    for(usize i = 0; i < sliceSize; i++)
    {
      if(buf[i])
      {
        count++;
      }
    }
  }
  return count;
}
} // namespace

TEST_CASE("SimplnxCore::ErodeDilateMaskFilter: Small Correctness", "[SimplnxCore][ErodeDilateMaskFilter]")
{
  UnitTest::LoadPlugins();
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  // 20x20x20, Mask (bool, 1-comp) => 20*20*1 = 400 bytes/slice
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 400, true);

  auto operation = GENERATE(k_Erode, k_Dilate);
  std::string operationName = (operation == k_Erode) ? "Erode" : "Dilate";
  DYNAMIC_SECTION("Operation: " << operationName << " forceOoc: " << forceOocAlgo)
  {
    DataStructure dataStructure;
    BuildTestData(dataStructure, 20, 20, 20);

    const usize trueCountBefore = CountTrueVoxels(dataStructure, 20, 20, 20);
    REQUIRE(trueCountBefore > 0);
    REQUIRE(trueCountBefore < 20 * 20 * 20);

    {
      const ErodeDilateMaskFilter filter;
      Arguments args;
      args.insertOrAssign(ErodeDilateMaskFilter::k_Operation_Key, std::make_any<ChoicesParameter::ValueType>(operation));
      args.insertOrAssign(ErodeDilateMaskFilter::k_NumIterations_Key, std::make_any<int32>(2));
      args.insertOrAssign(ErodeDilateMaskFilter::k_XDirOn_Key, std::make_any<bool>(true));
      args.insertOrAssign(ErodeDilateMaskFilter::k_YDirOn_Key, std::make_any<bool>(true));
      args.insertOrAssign(ErodeDilateMaskFilter::k_ZDirOn_Key, std::make_any<bool>(true));
      args.insertOrAssign(ErodeDilateMaskFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(k_MaskPath));
      args.insertOrAssign(ErodeDilateMaskFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));

      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
      auto executeResult = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    }

    const usize trueCountAfter = CountTrueVoxels(dataStructure, 20, 20, 20);
    if(operation == k_Erode)
    {
      REQUIRE(trueCountAfter < trueCountBefore);
    }
    else
    {
      REQUIRE(trueCountAfter > trueCountBefore);
    }

    // TODO: Add exemplar comparison after exemplar archive is published

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ErodeDilateMaskFilter: Generate Test Data", "[SimplnxCore][ErodeDilateMaskFilter][.GenerateTestData]")
{
  const auto outputDir = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "generated_test_data" / "erode_dilate_mask";
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

TEST_CASE("SimplnxCore::ErodeDilateMaskFilter: 200x200x200 Large OOC", "[SimplnxCore][ErodeDilateMaskFilter]")
{
  UnitTest::LoadPlugins();
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  // 200x200x200, Mask (bool, 1-comp) => 200*200*1 = 40,000 bytes/slice
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 40000, true);

  DYNAMIC_SECTION("forceOoc: " << forceOocAlgo)
  {
    DataStructure dataStructure;
    BuildTestData(dataStructure, 200, 200, 200);

    const usize trueCountBefore = CountTrueVoxels(dataStructure, 200, 200, 200);
    REQUIRE(trueCountBefore > 0);
    REQUIRE(trueCountBefore < 200 * 200 * 200);

    const ErodeDilateMaskFilter filter;
    Arguments args;
    args.insertOrAssign(ErodeDilateMaskFilter::k_Operation_Key, std::make_any<ChoicesParameter::ValueType>(k_Erode));
    args.insertOrAssign(ErodeDilateMaskFilter::k_NumIterations_Key, std::make_any<int32>(2));
    args.insertOrAssign(ErodeDilateMaskFilter::k_XDirOn_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErodeDilateMaskFilter::k_YDirOn_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErodeDilateMaskFilter::k_ZDirOn_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErodeDilateMaskFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(k_MaskPath));
    args.insertOrAssign(ErodeDilateMaskFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const usize trueCountAfter = CountTrueVoxels(dataStructure, 200, 200, 200);
    REQUIRE(trueCountAfter < trueCountBefore);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}
