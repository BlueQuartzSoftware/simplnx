#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/ErodeDilateMaskFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
constexpr ChoicesParameter::ValueType k_Dilate = 0ULL;
constexpr ChoicesParameter::ValueType k_Erode = 1ULL;

const std::string k_EbsdScanDataName("EBSD Scan Data");

const DataPath k_InputData({"Input Data"});
const DataPath k_EbsdScanDataDataPath = k_InputData.createChildPath(k_EbsdScanDataName);
const DataPath k_MaskArrayDataPath = k_EbsdScanDataDataPath.createChildPath("Mask");

} // namespace

TEST_CASE("SimplnxCore::ErodeDilateMaskFilter(Dilate)", "[SimplnxCore][ErodeDilateMaskFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  // Erode/Dilate test data: 20x201x189, EulerAngles (float32, 3-comp) => 201*189*3*4 = 455,868 bytes/slice
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 455868, true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_erode_dilate_test.tar.gz", "6_6_erode_dilate_test");

  const std::string k_ExemplarDataContainerName("Exemplar Mask Dilate");
  const DataPath k_DilateCellAttributeMatrixDataPath = DataPath({k_ExemplarDataContainerName, "EBSD Scan Data"});

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_erode_dilate_test/6_6_erode_dilate_mask.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  {
    const ErodeDilateMaskFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ErodeDilateMaskFilter::k_Operation_Key, std::make_any<ChoicesParameter::ValueType>(k_Dilate));
    args.insertOrAssign(ErodeDilateMaskFilter::k_NumIterations_Key, std::make_any<int32>(2));
    args.insertOrAssign(ErodeDilateMaskFilter::k_XDirOn_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErodeDilateMaskFilter::k_YDirOn_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErodeDilateMaskFilter::k_ZDirOn_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErodeDilateMaskFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(k_MaskArrayDataPath));
    args.insertOrAssign(ErodeDilateMaskFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_InputData));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  UnitTest::CompareExemplarToGeneratedData(dataStructure, dataStructure, k_EbsdScanDataDataPath, k_ExemplarDataContainerName);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ErodeDilateMaskFilter(Erode)", "[SimplnxCore][ErodeDilateMaskFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  // Erode/Dilate test data: 20x201x189, EulerAngles (float32, 3-comp) => 201*189*3*4 = 455,868 bytes/slice
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 455868, true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_erode_dilate_test.tar.gz", "6_6_erode_dilate_test");

  const std::string k_ExemplarDataContainerName("Exemplar Mask Erode");
  const DataPath k_ErodeCellAttributeMatrixDataPath = DataPath({k_ExemplarDataContainerName, "EBSD Scan Data"});

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_erode_dilate_test/6_6_erode_dilate_mask.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  {
    const ErodeDilateMaskFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ErodeDilateMaskFilter::k_Operation_Key, std::make_any<ChoicesParameter::ValueType>(k_Erode));
    args.insertOrAssign(ErodeDilateMaskFilter::k_NumIterations_Key, std::make_any<int32>(2));
    args.insertOrAssign(ErodeDilateMaskFilter::k_XDirOn_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErodeDilateMaskFilter::k_YDirOn_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErodeDilateMaskFilter::k_ZDirOn_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErodeDilateMaskFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(k_MaskArrayDataPath));
    args.insertOrAssign(ErodeDilateMaskFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_InputData));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  UnitTest::CompareExemplarToGeneratedData(dataStructure, dataStructure, k_EbsdScanDataDataPath, k_ExemplarDataContainerName);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ErodeDilateMaskFilter: Benchmark 200x200x200", "[SimplnxCore][ErodeDilateMaskFilter][Benchmark]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  // 200x200x200, Mask (bool, 1-comp) => 200*200*1 = 40,000 bytes/slice
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 40000, true);

  constexpr usize kDimX = 200;
  constexpr usize kDimY = 200;
  constexpr usize kDimZ = 200;
  const ShapeType cellTupleShape = {kDimZ, kDimY, kDimX};
  const auto benchmarkFile = fs::path(fmt::format("{}/erode_dilate_mask_benchmark.dream3d", unit_test::k_BinaryTestOutputDir));

  // Stage 1: Build data programmatically and write to .dream3d
  {
    DataStructure buildDS;
    auto* imageGeom = ImageGeom::Create(buildDS, "Input Data");
    imageGeom->setDimensions({kDimX, kDimY, kDimZ});
    imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
    imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

    auto* cellAM = AttributeMatrix::Create(buildDS, "EBSD Scan Data", cellTupleShape, imageGeom->getId());
    imageGeom->setCellData(*cellAM);

    auto* maskArray = UnitTest::CreateTestDataArray<bool>(buildDS, "Mask", cellTupleShape, {1}, cellAM->getId());
    auto& maskStore = maskArray->getDataStoreRef();

    for(usize z = 0; z < kDimZ; z++)
    {
      for(usize y = 0; y < kDimY; y++)
      {
        for(usize x = 0; x < kDimX; x++)
        {
          const usize idx = z * kDimX * kDimY + y * kDimX + x;
          maskStore[idx] = ((x * 7 + y * 13 + z * 29) % 3 != 0);
        }
      }
    }

    UnitTest::WriteTestDataStructure(buildDS, benchmarkFile);
  }

  DataStructure dataStructure = UnitTest::LoadDataStructure(benchmarkFile);

  {
    const ErodeDilateMaskFilter filter;
    Arguments args;

    args.insertOrAssign(ErodeDilateMaskFilter::k_Operation_Key, std::make_any<ChoicesParameter::ValueType>(k_Dilate));
    args.insertOrAssign(ErodeDilateMaskFilter::k_NumIterations_Key, std::make_any<int32>(2));
    args.insertOrAssign(ErodeDilateMaskFilter::k_XDirOn_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErodeDilateMaskFilter::k_YDirOn_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErodeDilateMaskFilter::k_ZDirOn_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErodeDilateMaskFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Input Data", "EBSD Scan Data", "Mask"})));
    args.insertOrAssign(ErodeDilateMaskFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Input Data"})));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args, nullptr, IFilter::MessageHandler{[](const IFilter::Message& message) { fmt::print("{}\n", message.message); }});
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  fs::remove(benchmarkFile);
}
