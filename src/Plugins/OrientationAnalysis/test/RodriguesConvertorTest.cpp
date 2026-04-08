#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include "OrientationAnalysis/Filters/RodriguesConvertorFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace nx::core;
namespace fs = std::filesystem;
namespace
{
const std::string k_InputArrayName = "Input";
const std::string k_ConvertedName = "Output";
const std::string k_Exemplar0 = "Exemplar0";

} // namespace

TEST_CASE("OrientationAnalysis::RodriguesConvertorFilter", "[OrientationAnalysis][RodriguesConvertorFilter]")
{
  UnitTest::LoadPlugins();

  // Instantiate the filter, a DataStructure object and an Arguments Object
  DataStructure dataStructure;

  // Build up a simple Float32Array and place default data into the array
  Float32Array* quats = UnitTest::CreateTestDataArray<float32>(dataStructure, k_InputArrayName, {4ULL}, {3ULL}, {});

  for(usize i = 0; i < 12; i++)
  {
    (*quats)[i] = static_cast<float32>(i);
  }
  Float32Array* exemplarData = UnitTest::CreateTestDataArray<float32>(dataStructure, k_Exemplar0, {4ULL}, {4ULL}, {});
  (*exemplarData)[0] = 0.0F;
  (*exemplarData)[1] = 0.447214F;
  (*exemplarData)[2] = 0.894427F;
  (*exemplarData)[3] = 2.23607F;
  (*exemplarData)[4] = 0.424264F;
  (*exemplarData)[5] = 0.565685F;
  (*exemplarData)[6] = 0.707107F;
  (*exemplarData)[7] = 7.07107F;
  (*exemplarData)[8] = 0.491539F;
  (*exemplarData)[9] = 0.573462F;
  (*exemplarData)[10] = 0.655386F;
  (*exemplarData)[11] = 12.2066F;
  (*exemplarData)[12] = 0.517893F;
  (*exemplarData)[13] = 0.575437F;
  (*exemplarData)[14] = 0.632980F;
  (*exemplarData)[15] = 17.3781F;
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    const RodriguesConvertorFilter filter;

    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RodriguesConvertorFilter::k_RodriguesDataArrayPath_Key, std::make_any<DataPath>(DataPath({k_InputArrayName})));
    args.insertOrAssign(RodriguesConvertorFilter::k_OutputDataArrayName_Key, std::make_any<std::string>(k_ConvertedName));
    args.insertOrAssign(RodriguesConvertorFilter::k_DeleteOriginalData_Key, std::make_any<bool>(false));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(DataPath({k_ConvertedName})));
    auto& outputArray = dataStructure.getDataRefAs<Float32Array>(DataPath({k_ConvertedName}));

    UnitTest::CompareDataArrays<float32>(*exemplarData, outputArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::RodriguesConvertorFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][RodriguesConvertorFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "RodriguesConvertorFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "RodriguesConvertorFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<RodriguesConvertorFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(RodriguesConvertorFilter::k_RodriguesDataArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(RodriguesConvertorFilter::k_OutputDataArrayName_Key) == "TestArray");
      CHECK(args.value<bool>(RodriguesConvertorFilter::k_DeleteOriginalData_Key) == true);
    }
  }
}
