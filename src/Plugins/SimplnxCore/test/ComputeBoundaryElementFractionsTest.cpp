#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

#include "simplnx/Core/Application.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/ComputeBoundaryElementFractionsFilter.hpp"

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
const std::string k_BCFName = "Boundary Cell Fractions";

const DataPath k_FeatureDataAMPath = DataPath({Constants::k_SmallIN100, Constants::k_Grain_Data});

const DataPath k_ExemplarBCFPath = k_FeatureDataAMPath.createChildPath(" Surface Element Fractions");
const DataPath k_GeneratedBCFPath = k_FeatureDataAMPath.createChildPath(k_BCFName);
} // namespace

TEST_CASE("SimplnxCore::ComputeBoundaryElementFractionsFilter: Valid Filter Execution", "[SimplnxCore][ComputeBoundaryElementFractionsFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_find_feature_boundary_element_fractions.tar.gz", "6_6_find_feature_boundary_element_fractions");

  DataStructure dataStructure =
      UnitTest::LoadDataStructure(fs::path(fmt::format("{}/6_6_find_feature_boundary_element_fractions/6_6_find_feature_boundary_element_fractions.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ComputeBoundaryElementFractionsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeBoundaryElementFractionsFilter::k_FeatureIdsArrayPath_Key,
                        std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_FeatureIds})));
    args.insertOrAssign(ComputeBoundaryElementFractionsFilter::k_BoundaryCellsArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "BoundaryCells"})));
    args.insertOrAssign(ComputeBoundaryElementFractionsFilter::k_FeatureDataAMPath_Key, std::make_any<DataPath>(k_FeatureDataAMPath));
    args.insertOrAssign(ComputeBoundaryElementFractionsFilter::k_BoundaryCellFractionsArrayName_Key, std::make_any<std::string>(::k_BCFName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  UnitTest::CompareArrays<float32>(dataStructure, k_ExemplarBCFPath, k_GeneratedBCFPath);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeBoundaryElementFractionsFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeBoundaryElementFractionsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeBoundaryElementFractionsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeBoundaryElementFractionsFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeBoundaryElementFractionsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ComputeBoundaryElementFractionsFilter::k_FeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeBoundaryElementFractionsFilter::k_FeatureDataAMPath_Key) == DataPath({"DataContainer", "CellData"}));
      CHECK(args.value<DataPath>(ComputeBoundaryElementFractionsFilter::k_BoundaryCellsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeBoundaryElementFractionsFilter::k_BoundaryCellFractionsArrayName_Key) == "TestArray");
    }
  }
}
