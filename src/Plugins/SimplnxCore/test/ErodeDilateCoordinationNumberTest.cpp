#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/ErodeDilateCoordinationNumberFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
const std::string k_EbsdScanDataName("EBSD Scan Data");

const DataPath k_InputData({"Input Data"});
const DataPath k_EbsdScanDataDataPath = k_InputData.createChildPath(k_EbsdScanDataName);
const DataPath k_FeatureIdsDataPath = k_EbsdScanDataDataPath.createChildPath("FeatureIds");

const std::string k_ExemplarDataContainerName("Exemplar Coordination Number");
const DataPath k_ErodeCellAttributeMatrixDataPath = DataPath({k_ExemplarDataContainerName, k_EbsdScanDataName});
} // namespace

TEST_CASE("SimplnxCore::ErodeDilateCoordinationNumberFilter", "[SimplnxCore][ErodeDilateCoordinationNumberFilter]")
{
  UnitTest::LoadPlugins();
  // Erode/Dilate test data: 20x201x189, EulerAngles (float32, 3-comp) => 201*189*3*4 = 455,868 bytes/slice
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 455868, true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_erode_dilate_test.tar.gz", "6_6_erode_dilate_test");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_erode_dilate_test/6_6_erode_dilate_coordination_number.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    const ErodeDilateCoordinationNumberFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_CoordinationNumber_Key, std::make_any<int32>(6));
    args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_Loop_Key, std::make_any<bool>(false));
    args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsDataPath));
    args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
    args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_InputData));

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

TEST_CASE("SimplnxCore::ErodeDilateCoordinationNumberFilter: Benchmark 200x200x200", "[SimplnxCore][ErodeDilateCoordinationNumberFilter][Benchmark]")
{
  UnitTest::LoadPlugins();
  // 200x200x200, EulerAngles (float32, 3-comp) => 200*200*3*4 = 480,000 bytes/slice
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 480000, true);

  constexpr usize k_DimX = 200;
  constexpr usize k_DimY = 200;
  constexpr usize k_DimZ = 200;
  const ShapeType cellTupleShape = {k_DimZ, k_DimY, k_DimX};
  const auto benchmarkFile = fs::path(fmt::format("{}/erode_dilate_coordination_number_benchmark.dream3d", unit_test::k_BinaryTestOutputDir));

  // Stage 1: Build data programmatically and write to .dream3d
  {
    DataStructure buildDS;
    auto* imageGeom = ImageGeom::Create(buildDS, "Input Data");
    imageGeom->setDimensions({k_DimX, k_DimY, k_DimZ});
    imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
    imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

    auto* cellAM = AttributeMatrix::Create(buildDS, "EBSD Scan Data", cellTupleShape, imageGeom->getId());
    imageGeom->setCellData(*cellAM);

    auto* featureIdsArray = UnitTest::CreateTestDataArray<int32>(buildDS, "FeatureIds", cellTupleShape, {1}, cellAM->getId());
    auto& featureIdsStore = featureIdsArray->getDataStoreRef();

    auto* eulerArray = UnitTest::CreateTestDataArray<float32>(buildDS, "EulerAngles", cellTupleShape, {3}, cellAM->getId());
    auto& eulerStore = eulerArray->getDataStoreRef();

    constexpr usize k_BlockSize = 25;
    constexpr usize k_BlocksPerDim = k_DimX / k_BlockSize;
    for(usize z = 0; z < k_DimZ; z++)
    {
      for(usize y = 0; y < k_DimY; y++)
      {
        for(usize x = 0; x < k_DimX; x++)
        {
          const usize idx = z * k_DimX * k_DimY + y * k_DimX + x;

          usize bx = x / k_BlockSize;
          usize by = y / k_BlockSize;
          usize bz = z / k_BlockSize;
          int32 blockFeatureId = static_cast<int32>(bz * k_BlocksPerDim * k_BlocksPerDim + by * k_BlocksPerDim + bx + 1);

          bool isBad = ((x * 7 + y * 13 + z * 29) % 7 == 0);
          featureIdsStore[idx] = isBad ? 0 : blockFeatureId;

          const usize eIdx = idx * 3;
          eulerStore[eIdx] = static_cast<float32>(x) / static_cast<float32>(k_DimX);
          eulerStore[eIdx + 1] = static_cast<float32>(y) / static_cast<float32>(k_DimY);
          eulerStore[eIdx + 2] = static_cast<float32>(z) / static_cast<float32>(k_DimZ);
        }
      }
    }

    UnitTest::WriteTestDataStructure(buildDS, benchmarkFile);
  }

  DataStructure dataStructure = UnitTest::LoadDataStructure(benchmarkFile);

  {
    const ErodeDilateCoordinationNumberFilter filter;
    Arguments args;

    args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_CoordinationNumber_Key, std::make_any<int32>(4));
    args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_Loop_Key, std::make_any<bool>(false));
    args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Input Data", "EBSD Scan Data", "FeatureIds"})));
    args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
    args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Input Data"})));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args, nullptr, IFilter::MessageHandler{[](const IFilter::Message& message) { fmt::print("{}\n", message.message); }});
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  fs::remove(benchmarkFile);
}
