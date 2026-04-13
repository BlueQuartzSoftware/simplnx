#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

#include "SimplnxCore/Filters/ComputeVectorColorsFilter.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
DataPath ebsdPath = DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData});
DataPath eulerAnglesPath = ebsdPath.createChildPath(Constants::k_EulerAngles);
const std::string k_VecColorsNX = "Vector Colors";
} // namespace

TEST_CASE("SimplnxCore::ComputeVectorColorsFilter: Valid Filter Execution", "[SimplnxCore][ComputeVectorColorsFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "generate_vector_colors.tar.gz", "generate_vector_colors");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/generate_vector_colors/6_6_generate_vector_colors.dream3d", unit_test::k_TestFilesDir)));
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ComputeVectorColorsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeVectorColorsFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(ComputeVectorColorsFilter::k_VectorsArrayPath_Key, std::make_any<DataPath>(eulerAnglesPath));
    args.insertOrAssign(ComputeVectorColorsFilter::k_CellVectorColorsArrayName_Key, std::make_any<std::string>(k_VecColorsNX));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  UnitTest::CompareArrays<uint8>(dataStructure, ebsdPath.createChildPath("VectorColor"), ebsdPath.createChildPath(k_VecColorsNX));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeVectorColorsFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeVectorColorsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeVectorColorsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeVectorColorsFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeVectorColorsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<bool>(ComputeVectorColorsFilter::k_UseMask_Key) == true);
      CHECK(args.value<DataPath>(ComputeVectorColorsFilter::k_VectorsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeVectorColorsFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeVectorColorsFilter::k_CellVectorColorsArrayName_Key) == "TestName");
    }
  }
}
