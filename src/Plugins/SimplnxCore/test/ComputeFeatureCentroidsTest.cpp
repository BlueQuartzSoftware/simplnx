#include "SimplnxCore/Filters/ComputeFeatureCentroidsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/Dream3dImportParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::UnitTest;
using namespace nx::core::Constants;

TEST_CASE("SimplnxCore::ComputeFeatureCentroidsFilter", "[SimplnxCore][ComputeFeatureCentroidsFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_stats_test_v2.tar.gz", "6_6_stats_test_v2.dream3d");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_stats_test_v2.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(baseDataFilePath);

  const std::string k_CentroidsNX("Centroids NX");

  // Instantiate ComputeFeatureCentroidsFilter
  {
    ComputeFeatureCentroidsFilter filter;
    Arguments args;

    const DataPath k_FeatureIdsArrayPath2({k_DataContainer, k_CellData, k_FeatureIds});
    const DataPath k_CentroidsNXArrayPath({k_DataContainer, k_CellFeatureData, k_CentroidsNX});
    const DataPath k_FeatureAttributeMatrix({k_DataContainer, k_CellFeatureData});
    const DataPath k_SelectedImageGeometry({k_DataContainer});

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeFeatureCentroidsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsArrayPath2));
    args.insertOrAssign(ComputeFeatureCentroidsFilter::k_CentroidsArrayName_Key, std::make_any<std::string>(k_CentroidsNX));
    args.insertOrAssign(ComputeFeatureCentroidsFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_FeatureAttributeMatrix));
    args.insertOrAssign(ComputeFeatureCentroidsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_SelectedImageGeometry));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  {
    const DataPath k_CentroidsArrayPath({k_DataContainer, k_CellFeatureData, k_Centroids});
    const DataPath k_CentroidsNXArrayPath({k_DataContainer, k_CellFeatureData, k_CentroidsNX});

    const auto& k_CentroidsArray = dataStructure.getDataRefAs<IDataArray>(k_CentroidsArrayPath);
    const auto& k_CentroidsNXArray = dataStructure.getDataRefAs<IDataArray>(k_CentroidsNXArrayPath);

    CompareDataArrays<float>(k_CentroidsArray, k_CentroidsNXArray);
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_feature_centroids.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureCentroidsFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeFeatureCentroidsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeFeatureCentroidsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeFeatureCentroidsFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeFeatureCentroidsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ComputeFeatureCentroidsFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ComputeFeatureCentroidsFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      // Complex type (DataArrayCreationToAMFilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<std::string>(ComputeFeatureCentroidsFilter::k_CentroidsArrayName_Key) == "TestArray");
    }
  }
}
