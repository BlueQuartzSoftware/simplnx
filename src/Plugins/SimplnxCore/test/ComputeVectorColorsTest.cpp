#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <array>
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>
#include <memory>

#include <nonstd/span.hpp>

#include "SimplnxCore/Filters/ComputeVectorColorsFilter.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
DataPath ebsdPath = DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData});
DataPath eulerAnglesPath = ebsdPath.createChildPath(Constants::k_EulerAngles);
const std::string k_VecColorsNX = "Vector Colors";
} // namespace

TEST_CASE("SimplnxCore::ComputeVectorColorsFilter: Rounded full-turn trend stays red", "[SimplnxCore][ComputeVectorColorsFilter]")
{
  DataStructure dataStructure;
  const DataPath vectorsPath({"Vectors"});
  auto vectorsStore = DataStoreUtilities::CreateDataStore<float32>(dataStructure, vectorsPath, {1}, {3});
  REQUIRE(Float32Array::Create(dataStructure, vectorsPath.getTargetName(), vectorsStore) != nullptr);
  const std::array<float32, 3> vectors = {1.0f, -1.0e-8f, 0.0f};
  const auto writeResult = vectorsStore->copyFromBuffer(0, nonstd::span<const float32>(vectors.data(), vectors.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);

  ComputeVectorColorsFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeVectorColorsFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeVectorColorsFilter::k_VectorsArrayPath_Key, std::make_any<DataPath>(vectorsPath));
  args.insertOrAssign(ComputeVectorColorsFilter::k_CellVectorColorsArrayName_Key, std::make_any<std::string>(k_VecColorsNX));
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Hand calculation: atan2(-1e-8, 1) is about -5.73e-7 degrees.
  // Adding 360 rounds to exactly 360 in float32, equivalent to trend 0.
  // With Z=0, plunge is 90 degrees: the red endpoint is RGB (255, 0, 0).
  const std::array<uint8, 3> expectedColors = {255, 0, 0};
  const auto* colors = dataStructure.getDataAs<UInt8Array>(DataPath({k_VecColorsNX}));
  REQUIRE(colors != nullptr);
  std::array<uint8, 3> actualColors = {};
  const auto readResult = colors->getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint8>(actualColors.data(), actualColors.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(readResult);
  CHECK(actualColors == expectedColors);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeVectorColorsFilter: Repeated unmasked execution creates only colors", "[SimplnxCore][ComputeVectorColorsFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  const DataPath vectorsPath({"Vectors"});
  auto vectorsStore = DataStoreUtilities::CreateDataStore<float32>(dataStructure, vectorsPath, {2}, {3});
  REQUIRE(Float32Array::Create(dataStructure, vectorsPath.getTargetName(), vectorsStore) != nullptr);
  const std::array<float32, 6> vectors = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
  const auto writeResult = vectorsStore->copyFromBuffer(0, nonstd::span<const float32>(vectors.data(), vectors.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);

  // Hand calculation: +X has trend 0 and plunge 90, giving pure red.
  // +Z has plunge 0, so every channel blends fully to white.
  const std::array<uint8, 6> expectedColors = {255, 0, 0, 255, 255, 255};
  std::vector<DataPath> expectedPaths = {vectorsPath};
  for(const std::string outputName : {"First Colors", "Second Colors"})
  {
    CAPTURE(outputName);
    ComputeVectorColorsFilter filter;
    Arguments args;
    args.insertOrAssign(ComputeVectorColorsFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(ComputeVectorColorsFilter::k_VectorsArrayPath_Key, std::make_any<DataPath>(vectorsPath));
    args.insertOrAssign(ComputeVectorColorsFilter::k_CellVectorColorsArrayName_Key, std::make_any<std::string>(outputName));

    const auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    const auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const DataPath colorsPath({outputName});
    expectedPaths.push_back(colorsPath);
    CHECK(dataStructure.getData(DataPath({"mask array"})) == nullptr);
    CHECK(dataStructure.getAllDataPaths().size() == expectedPaths.size());
    for(const auto& path : expectedPaths)
    {
      CHECK(dataStructure.getData(path) != nullptr);
    }

    const auto* colors = dataStructure.getDataAs<UInt8Array>(colorsPath);
    REQUIRE(colors != nullptr);
    CHECK(colors->getTupleShape() == ShapeType{2});
    CHECK(colors->getComponentShape() == ShapeType{3});
    std::array<uint8, 6> actualColors = {};
    const auto readResult = colors->getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint8>(actualColors.data(), actualColors.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(readResult);
    CHECK(actualColors == expectedColors);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ComputeVectorColorsFilter: Valid Filter Execution", "[SimplnxCore][ComputeVectorColorsFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "generate_vector_colors.tar.gz", "generate_vector_colors");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/generate_vector_colors/6_6_generate_vector_colors.dream3d", unit_test::k_TestFilesDir)));
  {
    // Create the filter arguments for the calculation.
    ComputeVectorColorsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeVectorColorsFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(ComputeVectorColorsFilter::k_VectorsArrayPath_Key, std::make_any<DataPath>(eulerAnglesPath));
    args.insertOrAssign(ComputeVectorColorsFilter::k_CellVectorColorsArrayName_Key, std::make_any<std::string>(k_VecColorsNX));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  UnitTest::CompareArrays<uint8>(dataStructure, ebsdPath.createChildPath("VectorColor"), ebsdPath.createChildPath(k_VecColorsNX));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeVectorColorsFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeVectorColorsFilter][BackwardsCompatibility]")
{
  const auto version = GENERATE("6_4", "6_5");
  CAPTURE(version);
  const fs::path fixturePath = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion" / version / "ComputeVectorColorsFilter.json";
  const auto args = UnitTest::LoadSIMPLConversionArguments(fixturePath, FilterTraits<ComputeVectorColorsFilter>::uuid);
  CHECK(args.value<bool>(ComputeVectorColorsFilter::k_UseMask_Key) == true);
  CHECK(args.value<DataPath>(ComputeVectorColorsFilter::k_VectorsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
  CHECK(args.value<DataPath>(ComputeVectorColorsFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
  CHECK(args.value<std::string>(ComputeVectorColorsFilter::k_CellVectorColorsArrayName_Key) == "TestName");
}
