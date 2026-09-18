#include "SimplnxCore/Filters/ComputeVolumeFractionsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <array>
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const std::string k_VolumeFractions("Volume Fractions");
const std::string k_VolumeFractionsNX("Volume Fractions NX");

const DataPath k_IncorrectCellPhasesPath({Constants::k_DataContainer, Constants::k_FeatureData, Constants::k_Phases});

const DataPath k_VolumeFractionsPath = Constants::k_CellEnsembleAttributeMatrixPath.createChildPath(k_VolumeFractions);
const DataPath k_VolumeFractionsPathNX = Constants::k_CellEnsembleAttributeMatrixPath.createChildPath(k_VolumeFractionsNX);

const fs::path k_BaseDataFilePath = fs::path(fmt::format("{}/6_6_volume_fraction_feature_count.dream3d", unit_test::k_TestFilesDir));
} // namespace

TEST_CASE("SimplnxCore::ComputeVolumeFractionsFilter: Valid filter execution", "[SimplnxCore::ComputeVolumeFractionsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  // Configure the filter arguments.
  ComputeVolumeFractionsFilter filter;
  Arguments args;

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_volume_fraction_feature_count.dream3d.tar.gz", "6_6_volume_fraction_feature_count.dream3d");

  DataStructure dataStructure = UnitTest::LoadDataStructure(k_BaseDataFilePath);
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(Constants::k_PhasesArrayPath));

  args.insertOrAssign(ComputeVolumeFractionsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(Constants::k_PhasesArrayPath));
  args.insertOrAssign(ComputeVolumeFractionsFilter::k_CellEnsembleAttributeMatrixPath_Key, std::make_any<DataPath>(Constants::k_CellEnsembleAttributeMatrixPath));
  args.insertOrAssign(ComputeVolumeFractionsFilter::k_VolFractionsArrayName_Key, std::make_any<std::string>(k_VolumeFractionsNX));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  auto& d3dVolumeFractionsArrayRef = dataStructure.getDataRefAs<Float32Array>(k_VolumeFractionsPath);
  auto& nxVolumeFractionsArrayRef = dataStructure.getDataRefAs<Float32Array>(k_VolumeFractionsPathNX);
  scope.requireExpectedStore(nxVolumeFractionsArrayRef);

  for(usize index = 0; index < d3dVolumeFractionsArrayRef.getSize(); index++)
  {
    float32 result = fabsf(d3dVolumeFractionsArrayRef[index] - nxVolumeFractionsArrayRef[index]);
    REQUIRE(result < UnitTest::EPSILON);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeVolumeFractionsFilter: InValid filter execution", "[SimplnxCore::ComputeVolumeFractionsFilter]")
{
  UnitTest::LoadPlugins();

  // Configure the filter arguments.
  ComputeVolumeFractionsFilter filter;
  Arguments args;

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_volume_fraction_feature_count.dream3d.tar.gz", "6_6_volume_fraction_feature_count.dream3d");

  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_volFractions_and_numFeatures_test.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(k_BaseDataFilePath);

  args.insertOrAssign(ComputeVolumeFractionsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_IncorrectCellPhasesPath));
  args.insertOrAssign(ComputeVolumeFractionsFilter::k_CellEnsembleAttributeMatrixPath_Key, std::make_any<DataPath>(Constants::k_CellEnsembleAttributeMatrixPath));
  args.insertOrAssign(ComputeVolumeFractionsFilter::k_VolFractionsArrayName_Key, std::make_any<std::string>(k_VolumeFractionsNX));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  auto& d3dVolumeFractionsArrayRef = dataStructure.getDataRefAs<Float32Array>(k_VolumeFractionsPath);
  auto& nxVolumeFractionsArrayRef = dataStructure.getDataRefAs<Float32Array>(k_VolumeFractionsPathNX);

  for(usize index = 0; index < d3dVolumeFractionsArrayRef.getSize(); index++)
  {
    float32 result = fabsf(d3dVolumeFractionsArrayRef[index] - nxVolumeFractionsArrayRef[index]);
    REQUIRE(result > UnitTest::EPSILON);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeVolumeFractionsFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeVolumeFractionsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeVolumeFractionsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeVolumeFractionsFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeVolumeFractionsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ComputeVolumeFractionsFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeVolumeFractionsFilter::k_CellEnsembleAttributeMatrixPath_Key) == DataPath({"DataContainer", "CellData"}));
      CHECK(args.value<std::string>(ComputeVolumeFractionsFilter::k_VolFractionsArrayName_Key) == "TestArray");
    }
  }
}
