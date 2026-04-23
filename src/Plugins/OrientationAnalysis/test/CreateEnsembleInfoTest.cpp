#include "OrientationAnalysis/Filters/CreateEnsembleInfoFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/EnsembleInfoParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

using namespace nx::core;
using namespace nx::core::UnitTest;
namespace fs = std::filesystem;

TEST_CASE("OrientationAnalysis::CreateEnsembleInfoFilter: Invalid filter execution", "[OrientationAnalysis][CreateEnsembleInfoFilter]")
{
  UnitTest::LoadPlugins();

  // Instantiate the filter, a DataStructure object and an Arguments Object
  CreateEnsembleInfoFilter filter;
  DataStructure dataStructure;
  Arguments args;

  SECTION("Missing ensemble phase data")
  {
    // Create default Parameters for the filter.
    args.insertOrAssign(CreateEnsembleInfoFilter::k_Ensemble_Key, std::make_any<EnsembleInfoParameter::ValueType>(EnsembleInfoParameter::ValueType{}));
    args.insertOrAssign(CreateEnsembleInfoFilter::k_CellEnsembleAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"EnsembleAttributeMatrix"})));
    args.insertOrAssign(CreateEnsembleInfoFilter::k_CrystalStructuresArrayName_Key, std::make_any<std::string>("CrystalStructures"));
    args.insertOrAssign(CreateEnsembleInfoFilter::k_PhaseTypesArrayName_Key, std::make_any<std::string>("PhaseTypes"));
    args.insertOrAssign(CreateEnsembleInfoFilter::k_PhaseNamesArrayName_Key, std::make_any<std::string>("PhaseNames"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }

  SECTION("Missing attribute matrix path")
  {
    // Create default Parameters for the filter.
    args.insertOrAssign(CreateEnsembleInfoFilter::k_Ensemble_Key, std::make_any<EnsembleInfoParameter::ValueType>(EnsembleInfoParameter::ValueType{{"Cubic-High m-3m", "Primary", "Cubic"}}));
    args.insertOrAssign(CreateEnsembleInfoFilter::k_CellEnsembleAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CreateEnsembleInfoFilter::k_CrystalStructuresArrayName_Key, std::make_any<std::string>("CrystalStructures"));
    args.insertOrAssign(CreateEnsembleInfoFilter::k_PhaseTypesArrayName_Key, std::make_any<std::string>("PhaseTypes"));
    args.insertOrAssign(CreateEnsembleInfoFilter::k_PhaseNamesArrayName_Key, std::make_any<std::string>("PhaseNames"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }

  SECTION("Invalid crystal structure ensemble phase data")
  {
    // Create default Parameters for the filter.
    args.insertOrAssign(CreateEnsembleInfoFilter::k_Ensemble_Key, std::make_any<EnsembleInfoParameter::ValueType>(EnsembleInfoParameter::ValueType{{"", "Primary", "Cubic"}}));
    args.insertOrAssign(CreateEnsembleInfoFilter::k_CellEnsembleAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"EnsembleAttributeMatrix"})));
    args.insertOrAssign(CreateEnsembleInfoFilter::k_CrystalStructuresArrayName_Key, std::make_any<std::string>("CrystalStructures"));
    args.insertOrAssign(CreateEnsembleInfoFilter::k_PhaseTypesArrayName_Key, std::make_any<std::string>("PhaseTypes"));
    args.insertOrAssign(CreateEnsembleInfoFilter::k_PhaseNamesArrayName_Key, std::make_any<std::string>("PhaseNames"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  }

  SECTION("Invalid phase type ensemble phase data")
  {
    // Create default Parameters for the filter.
    args.insertOrAssign(CreateEnsembleInfoFilter::k_Ensemble_Key, std::make_any<EnsembleInfoParameter::ValueType>(EnsembleInfoParameter::ValueType{{"Cubic-High m-3m", "Uknown", "Cubic"}}));
    args.insertOrAssign(CreateEnsembleInfoFilter::k_CellEnsembleAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"EnsembleAttributeMatrix"})));
    args.insertOrAssign(CreateEnsembleInfoFilter::k_CrystalStructuresArrayName_Key, std::make_any<std::string>("CrystalStructures"));
    args.insertOrAssign(CreateEnsembleInfoFilter::k_PhaseTypesArrayName_Key, std::make_any<std::string>("PhaseTypes"));
    args.insertOrAssign(CreateEnsembleInfoFilter::k_PhaseNamesArrayName_Key, std::make_any<std::string>("PhaseNames"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  }

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::CreateEnsembleInfoFilter: Valid filter execution", "[OrientationAnalysis][CreateEnsembleInfoFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "so3_cubic_high_ipf_001.tar.gz", "so3_cubic_high_ipf_001.dream3d");

  // Instantiate the filter, a DataStructure object and an Arguments Object
  CreateEnsembleInfoFilter filter;
  DataStructure dataStructure;
  Arguments args;

  const DataPath cellEnsembleAttributeMatrixPath({"EnsembleAttributeMatrix"});
  const std::string crystalStructuresName = "CrystalStructures";
  const std::string phaseTypesName = "PhaseTypes";
  const std::string phaseNamesName = "PhaseNames";

  // Create Parameters for the filter.
  args.insertOrAssign(CreateEnsembleInfoFilter::k_Ensemble_Key, std::make_any<EnsembleInfoParameter::ValueType>(EnsembleInfoParameter::ValueType{{"Cubic-High m-3m", "Primary", "Cubic"}}));
  args.insertOrAssign(CreateEnsembleInfoFilter::k_CellEnsembleAttributeMatrixPath_Key, std::make_any<DataPath>(cellEnsembleAttributeMatrixPath));
  args.insertOrAssign(CreateEnsembleInfoFilter::k_CrystalStructuresArrayName_Key, std::make_any<std::string>(crystalStructuresName));
  args.insertOrAssign(CreateEnsembleInfoFilter::k_PhaseTypesArrayName_Key, std::make_any<std::string>(phaseTypesName));
  args.insertOrAssign(CreateEnsembleInfoFilter::k_PhaseNamesArrayName_Key, std::make_any<std::string>(phaseNamesName));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/so3_cubic_high_ipf_001.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = LoadDataStructure(exemplarFilePath);
  const DataPath exemplarAttributeMatrixPath({"ImageDataContainer", "CellEnsembleData"});

  const auto* attributeMatrix = dataStructure.getDataAs<AttributeMatrix>(cellEnsembleAttributeMatrixPath);
  REQUIRE(attributeMatrix != nullptr);

  const auto* exemplarAttributeMatrix = exemplarDataStructure.getDataAs<AttributeMatrix>(exemplarAttributeMatrixPath);
  REQUIRE(attributeMatrix->getShape() == exemplarAttributeMatrix->getShape());

  const auto* crystalStructures = dataStructure.getDataAs<UInt32Array>(cellEnsembleAttributeMatrixPath.createChildPath(crystalStructuresName));
  const auto* phaseTypes = dataStructure.getDataAs<UInt32Array>(cellEnsembleAttributeMatrixPath.createChildPath(phaseTypesName));
  const auto* phaseNames = dataStructure.getDataAs<StringArray>(cellEnsembleAttributeMatrixPath.createChildPath(phaseNamesName));
  REQUIRE(crystalStructures != nullptr);
  REQUIRE(phaseTypes != nullptr);
  REQUIRE(phaseTypes != nullptr);

  const auto& exemplarCrystalStructures = exemplarDataStructure.getDataRefAs<UInt32Array>(exemplarAttributeMatrixPath.createChildPath(crystalStructuresName));
  const auto& exemplarPhaseTypes = exemplarDataStructure.getDataRefAs<UInt32Array>(exemplarAttributeMatrixPath.createChildPath(phaseTypesName));
  const auto& exemplarPhaseNames = exemplarDataStructure.getDataRefAs<StringArray>(exemplarAttributeMatrixPath.createChildPath(phaseNamesName));
  REQUIRE(exemplarCrystalStructures.getSize() == crystalStructures->getSize());
  REQUIRE(exemplarPhaseTypes.getSize() == phaseTypes->getSize());
  REQUIRE(exemplarPhaseNames.getSize() == phaseNames->getSize());

  usize numElements = exemplarCrystalStructures.getSize();
  for(usize i = 0; i < numElements; ++i)
  {
    REQUIRE((*crystalStructures)[i] == exemplarCrystalStructures[i]);
    REQUIRE((*phaseTypes)[i] == exemplarPhaseTypes[i]);
    REQUIRE((*phaseNames)[i] == exemplarPhaseNames[i]);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::CreateEnsembleInfoFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][CreateEnsembleInfoFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "CreateEnsembleInfoFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "CreateEnsembleInfoFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<CreateEnsembleInfoFilter>::uuid);

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<std::string>(CreateEnsembleInfoFilter::k_CrystalStructuresArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(CreateEnsembleInfoFilter::k_PhaseTypesArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(CreateEnsembleInfoFilter::k_PhaseNamesArrayName_Key) == "TestName");
      CHECK(args.value<DataPath>(CreateEnsembleInfoFilter::k_CellEnsembleAttributeMatrixPath_Key) == DataPath({"DataContainer", "CellEnsembleData"}));
      const auto ensembleValue = args.value<EnsembleInfoParameter::ValueType>(CreateEnsembleInfoFilter::k_Ensemble_Key);
      REQUIRE(ensembleValue.size() == 1);
      CHECK(ensembleValue[0][0] == "Cubic-High m-3m");
      CHECK(ensembleValue[0][1] == "Primary");
      CHECK(ensembleValue[0][2] == "Primary");
    }
  }
}
