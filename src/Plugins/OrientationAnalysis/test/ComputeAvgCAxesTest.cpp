#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

#include "simplnx/Core/Application.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "OrientationAnalysis/Filters/ComputeAvgCAxesFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace nx::core;
using namespace nx::core::Constants;
namespace fs = std::filesystem;

namespace compute_avg_caxis
{
const std::string k_ImageDataContainerName = "ImageDataContainer";
const DataPath k_ImageDataContainerPath({k_ImageDataContainerName});
const DataPath k_CellDataPath = k_ImageDataContainerPath.createChildPath("CellData");
const DataPath k_ParentIdsPath = k_CellDataPath.createChildPath("ParentIds");
const DataPath k_QuatsPath = k_CellDataPath.createChildPath("Quats");
const DataPath k_PhasesPath = k_CellDataPath.createChildPath("Phases");

const DataPath k_FeatureAttrMatPath = k_ImageDataContainerPath.createChildPath("ParentCellFeatureData");
const DataPath k_CrystalStructuresPath = k_ImageDataContainerPath.createChildPath("CellEnsembleData").createChildPath("CrystalStructures");
const std::string k_ParentAvgCAxisName = "Computed ParentAvgCAxes [NX]";
const std::string k_ParentAvgCAxisExemplarName = "ParentAvgCAxes [NX]";

} // namespace compute_avg_caxis

TEST_CASE("OrientationAnalysis::ComputeAvgCAxesFilter: Valid Filter Execution", "[OrientationAnalysis][ComputeAvgCAxesFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "7_2_AvgCAxis.tar.gz", "7_2_AvgCAxis");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/7_2_AvgCAxis/7_2_AvgCAxis.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeAvgCAxesFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ComputeAvgCAxesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(compute_avg_caxis::k_QuatsPath));
  args.insertOrAssign(ComputeAvgCAxesFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(compute_avg_caxis::k_ParentIdsPath));
  args.insertOrAssign(ComputeAvgCAxesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(compute_avg_caxis::k_PhasesPath));
  args.insertOrAssign(ComputeAvgCAxesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(compute_avg_caxis::k_CrystalStructuresPath));
  args.insertOrAssign(ComputeAvgCAxesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(compute_avg_caxis::k_FeatureAttrMatPath));
  args.insertOrAssign(ComputeAvgCAxesFilter::k_AvgCAxesArrayName_Key, std::make_any<std::string>(compute_avg_caxis::k_ParentAvgCAxisName));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  UnitTest::CompareFloatArraysWithNans<float32>(dataStructure, compute_avg_caxis::k_FeatureAttrMatPath.createChildPath(compute_avg_caxis::k_ParentAvgCAxisExemplarName),
                                                compute_avg_caxis::k_FeatureAttrMatPath.createChildPath(compute_avg_caxis::k_ParentAvgCAxisName), 5.0E-7f, false);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeAvgCAxesFilter: Invalid Filter Execution", "[OrientationAnalysis][ComputeAvgCAxesFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "caxis_data.tar.gz", "caxis_data");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/caxis_data/7_0_find_caxis_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  auto& crystalStructs = dataStructure.getDataRefAs<UInt32Array>(k_CrystalStructuresArrayPath);
  crystalStructs[1] = 1;

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeAvgCAxesFilter filter;
  Arguments args;

  // Invalid crystal structure type : should fail in execute
  args.insertOrAssign(ComputeAvgCAxesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsArrayPath));
  args.insertOrAssign(ComputeAvgCAxesFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsArrayPath));
  args.insertOrAssign(ComputeAvgCAxesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesArrayPath));
  args.insertOrAssign(ComputeAvgCAxesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));
  args.insertOrAssign(ComputeAvgCAxesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellFeatureDataPath));
  args.insertOrAssign(ComputeAvgCAxesFilter::k_AvgCAxesArrayName_Key, std::make_any<std::string>(compute_avg_caxis::k_ParentAvgCAxisName));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeAvgCAxesFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ComputeAvgCAxesFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeAvgCAxesFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeAvgCAxesFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeAvgCAxesFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ComputeAvgCAxesFilter::k_CellFeatureAttributeMatrixPath_Key) == DataPath({"DataContainer", "CellData"}));
      CHECK(args.value<DataPath>(ComputeAvgCAxesFilter::k_QuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeAvgCAxesFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", "CellData"}));
      CHECK(args.value<DataPath>(ComputeAvgCAxesFilter::k_FeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeAvgCAxesFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData"}));
      CHECK(args.value<std::string>(ComputeAvgCAxesFilter::k_AvgCAxesArrayName_Key) == "TestArray");
    }
  }
}
