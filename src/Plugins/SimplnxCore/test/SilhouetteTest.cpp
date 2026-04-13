#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/SilhouetteFilter.hpp"

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
const DataPath k_QuadGeomPath = DataPath({Constants::k_DataContainer});
const DataPath k_CellPath = k_QuadGeomPath.createChildPath(Constants::k_CellData);

const DataPath k_MedoidsClusterIdsPath = k_CellPath.createChildPath("MedoidsClusterIds");
const DataPath k_MeansClusterIdsPath = k_CellPath.createChildPath("MeansClusterIds");

const std::string k_MedoidsSilhouetteName = "MedoidsSilhouette";
const std::string k_MeansSilhouetteName = "MeansSilhouette";

const DataPath k_MedoidsSilhouettePath = k_CellPath.createChildPath(k_MedoidsSilhouetteName);
const DataPath k_MeansSilhouettePath = k_CellPath.createChildPath(k_MeansSilhouetteName);

const DataPath k_MedoidsSilhouettePathNX = k_CellPath.createChildPath(k_MedoidsSilhouetteName + "NX");
const DataPath k_MeansSilhouettePathNX = k_CellPath.createChildPath(k_MeansSilhouetteName + "NX");
} // namespace

TEST_CASE("SimplnxCore::SilhouetteFilter: Medoids Test", "[SimplnxCore][SilhouetteFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "k_files_v2.tar.gz", "k_files_v2");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/k_files_v2/7_0_silhouette_exemplar.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    SilhouetteFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(SilhouetteFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(SilhouetteFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(k_CellPath.createChildPath("DAMAGE")));
    args.insertOrAssign(SilhouetteFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_MedoidsClusterIdsPath));
    args.insertOrAssign(SilhouetteFilter::k_SilhouetteArrayPath_Key, std::make_any<DataPath>(k_MedoidsSilhouettePathNX));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  UnitTest::CompareArrays<float64>(dataStructure, k_MedoidsSilhouettePath, k_MedoidsSilhouettePathNX);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::SilhouetteFilter: Means Test", "[SimplnxCore][SilhouetteFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "k_files_v2.tar.gz", "k_files_v2");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/k_files_v2/7_0_silhouette_exemplar.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    SilhouetteFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(SilhouetteFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(SilhouetteFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(k_CellPath.createChildPath("DAMAGE")));
    args.insertOrAssign(SilhouetteFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_MeansClusterIdsPath));
    args.insertOrAssign(SilhouetteFilter::k_SilhouetteArrayPath_Key, std::make_any<DataPath>(k_MeansSilhouettePathNX));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  UnitTest::CompareArrays<float64>(dataStructure, k_MeansSilhouettePath, k_MeansSilhouettePathNX);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::SilhouetteFilter: SIMPL Backwards Compatibility", "[SimplnxCore][SilhouetteFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "SilhouetteFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "SilhouetteFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<SilhouetteFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<ChoicesParameter::ValueType>(SilhouetteFilter::k_DistanceMetric_Key) == 0);
      CHECK(args.value<bool>(SilhouetteFilter::k_UseMask_Key) == true);
      CHECK(args.value<DataPath>(SilhouetteFilter::k_SelectedArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(SilhouetteFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(SilhouetteFilter::k_FeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      // Complex type (DataArrayCreationFilterParameterConverter) - verified by successful pipeline loading
    }
  }
}
