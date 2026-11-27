#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <array>
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>
#include <memory>

#include <nonstd/span.hpp>

#include "SimplnxCore/Filters/ComputeVectorColorsFilter.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
DataPath EbsdPath = DataPath({constants::k_SmallIN100, constants::k_EbsdScanData});
DataPath EulerAnglesPath = EbsdPath.createChildPath(constants::k_EulerAngles);
const std::string k_VecColorsNX = "Vector Colors";
} // namespace

TEST_CASE("SimplnxCore::ComputeVectorColorsFilter: Rounded full-turn trend stays red", "[SimplnxCore][ComputeVectorColorsFilter]")
{
  DataStructure dataStructure;
  const DataPath vectorsPath({"Vectors"});
  auto vectorsStore = data_store_utilities::CreateDataStore<float32>(dataStructure, vectorsPath, {1}, {3});
  REQUIRE(Float32ArrayType::Create(dataStructure, vectorsPath.getTargetName(), vectorsStore) != nullptr);
  const std::array<float32, 3> vectors = {1.0f, -1.0e-8f, 0.0f};
  const auto writeResult = vectorsStore->copyFromBuffer(0, nonstd::span<const float32>(vectors.data(), vectors.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);

  ComputeVectorColorsFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeVectorColorsFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeVectorColorsFilter::k_VectorsArrayPath_Key, std::make_any<DataPath>(vectorsPath));
  args.insertOrAssign(ComputeVectorColorsFilter::k_CellVectorColorsArrayName_Key, std::make_any<std::string>(k_VecColorsNX));
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Hand calculation: atan2(-1e-8, 1) is about -5.73e-7 degrees.
  // Adding 360 rounds to exactly 360 in float32, equivalent to trend 0.
  // With Z=0, plunge is 90 degrees: the red endpoint is RGB (255, 0, 0).
  const std::array<uint8, 3> expectedColors = {255, 0, 0};
  const auto* colorsPtr = dataStructure.getDataAs<UInt8ArrayType>(DataPath({k_VecColorsNX}));
  REQUIRE(colorsPtr != nullptr);
  std::array<uint8, 3> actualColors = {};
  const auto readResult = colorsPtr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint8>(actualColors.data(), actualColors.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(readResult);
  CHECK(actualColors == expectedColors);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeVectorColorsFilter: Repeated unmasked execution creates only colors", "[SimplnxCore][ComputeVectorColorsFilter]")
{
  unit_test::LoadPlugins();

  DataStructure dataStructure;
  const DataPath vectorsPath({"Vectors"});
  auto vectorsStore = data_store_utilities::CreateDataStore<float32>(dataStructure, vectorsPath, {2}, {3});
  REQUIRE(Float32ArrayType::Create(dataStructure, vectorsPath.getTargetName(), vectorsStore) != nullptr);
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
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
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

    const auto* colorsPtr = dataStructure.getDataAs<UInt8ArrayType>(colorsPath);
    REQUIRE(colorsPtr != nullptr);
    CHECK(colorsPtr->getTupleShape() == ShapeType{2});
    CHECK(colorsPtr->getComponentShape() == ShapeType{3});
    std::array<uint8, 6> actualColors = {};
    const auto readResult = colorsPtr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint8>(actualColors.data(), actualColors.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(readResult);
    CHECK(actualColors == expectedColors);
    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ComputeVectorColorsFilter: Valid Filter Execution", "[SimplnxCore][ComputeVectorColorsFilter]")
{
  unit_test::LoadPlugins();

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "generate_vector_colors.tar.gz", "generate_vector_colors");
  DataStructure dataStructure = unit_test::LoadDataStructure(fs::path(fmt::format("{}/generate_vector_colors/6_6_generate_vector_colors.dream3d", unit_test::k_TestFilesDir)));
  {
    // Create the filter arguments for the calculation.
    ComputeVectorColorsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeVectorColorsFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(ComputeVectorColorsFilter::k_VectorsArrayPath_Key, std::make_any<DataPath>(EulerAnglesPath));
    args.insertOrAssign(ComputeVectorColorsFilter::k_CellVectorColorsArrayName_Key, std::make_any<std::string>(k_VecColorsNX));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  unit_test::CompareArrays<uint8>(dataStructure, EbsdPath.createChildPath("VectorColor"), EbsdPath.createChildPath(k_VecColorsNX));

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeVectorColorsFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeVectorColorsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeVectorColorsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeVectorColorsFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterListPtr);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilterPtr = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilterPtr != nullptr);

      const IFilter* filterPtr = pipelineFilterPtr->getFilter();
      REQUIRE(filterPtr != nullptr);
      REQUIRE(filterPtr->uuid() == FilterTraits<ComputeVectorColorsFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
      CHECK(args.value<bool>(ComputeVectorColorsFilter::k_UseMask_Key) == true);
      CHECK(args.value<DataPath>(ComputeVectorColorsFilter::k_VectorsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeVectorColorsFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeVectorColorsFilter::k_CellVectorColorsArrayName_Key) == "TestName");
    }
  }
}
