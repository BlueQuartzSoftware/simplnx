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
using namespace nx::core::unit_test;
namespace fs = std::filesystem;

TEST_CASE("OrientationAnalysis::CreateEnsembleInfoFilter: Invalid filter execution", "[OrientationAnalysis][CreateEnsembleInfoFilter]")
{
  unit_test::LoadPlugins();

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
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.output_actions);
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
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.output_actions);
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
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
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
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  }

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::CreateEnsembleInfoFilter: Valid filter execution", "[OrientationAnalysis][CreateEnsembleInfoFilter]")
{
  unit_test::LoadPlugins();

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "so3_cubic_high_ipf_001.tar.gz", "so3_cubic_high_ipf_001.dream3d");

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
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/so3_cubic_high_ipf_001.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = LoadDataStructure(exemplarFilePath);
  const DataPath exemplarAttributeMatrixPath({"ImageDataContainer", "CellEnsembleData"});

  const auto* attributeMatrixPtr = dataStructure.getDataAs<AttributeMatrix>(cellEnsembleAttributeMatrixPath);
  REQUIRE(attributeMatrixPtr != nullptr);

  const auto* exemplarAttributeMatrixPtr = exemplarDataStructure.getDataAs<AttributeMatrix>(exemplarAttributeMatrixPath);
  REQUIRE(attributeMatrixPtr->getShape() == exemplarAttributeMatrixPtr->getShape());

  const auto* crystalStructuresPtr = dataStructure.getDataAs<UInt32ArrayType>(cellEnsembleAttributeMatrixPath.createChildPath(crystalStructuresName));
  const auto* phaseTypesPtr = dataStructure.getDataAs<UInt32ArrayType>(cellEnsembleAttributeMatrixPath.createChildPath(phaseTypesName));
  const auto* phaseNamesPtr = dataStructure.getDataAs<StringArray>(cellEnsembleAttributeMatrixPath.createChildPath(phaseNamesName));
  REQUIRE(crystalStructuresPtr != nullptr);
  REQUIRE(phaseTypesPtr != nullptr);
  REQUIRE(phaseTypesPtr != nullptr);

  const auto& exemplarCrystalStructures = exemplarDataStructure.getDataRefAs<UInt32ArrayType>(exemplarAttributeMatrixPath.createChildPath(crystalStructuresName));
  const auto& exemplarPhaseTypes = exemplarDataStructure.getDataRefAs<UInt32ArrayType>(exemplarAttributeMatrixPath.createChildPath(phaseTypesName));
  const auto& exemplarPhaseNames = exemplarDataStructure.getDataRefAs<StringArray>(exemplarAttributeMatrixPath.createChildPath(phaseNamesName));
  REQUIRE(exemplarCrystalStructures.getSize() == crystalStructuresPtr->getSize());
  REQUIRE(exemplarPhaseTypes.getSize() == phaseTypesPtr->getSize());
  REQUIRE(exemplarPhaseNames.getSize() == phaseNamesPtr->getSize());

  usize numElements = exemplarCrystalStructures.getSize();
  for(usize i = 0; i < numElements; ++i)
  {
    REQUIRE((*crystalStructuresPtr)[i] == exemplarCrystalStructures[i]);
    REQUIRE((*phaseTypesPtr)[i] == exemplarPhaseTypes[i]);
    REQUIRE((*phaseNamesPtr)[i] == exemplarPhaseNames[i]);
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::CreateEnsembleInfoFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][CreateEnsembleInfoFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "CreateEnsembleInfoFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "CreateEnsembleInfoFilter.json"},
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
      REQUIRE(filterPtr->uuid() == FilterTraits<CreateEnsembleInfoFilter>::uuid);

      const Arguments args = pipelineFilterPtr->getArguments();
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
