#include "SimplnxCore/Filters/ExtractComponentAsArrayFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const std::string k_ExtractedComponents("Extracted Components");

const DataPath k_QuatsPath({Constants::k_DataContainer, Constants::k_EbsdScanData, Constants::k_Quats});
const DataPath k_ExtractedComponentsPath({Constants::k_DataContainer, Constants::k_EbsdScanData, k_ExtractedComponents});

const fs::path k_BaseDataFilePath = fs::path(fmt::format("{}/6_6_find_feature_centroids.dream3d", unit_test::k_TestFilesDir));
} // namespace

TEST_CASE("SimplnxCore::ExtractComponentAsArrayFilter: Valid filter execution", "[SimplnxCore][ExtractComponentAsArrayFilter]")
{
  UnitTest::LoadPlugins();

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ExtractComponentAsArrayFilter filter;

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_find_feature_centroids.tar.gz", "6_6_find_feature_centroids.dream3d");

  DataStructure alteredDs = UnitTest::LoadDataStructure(k_BaseDataFilePath);
  const int32 removeCompIndex = 1;

  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ExtractComponentAsArrayFilter::k_MoveComponentsToNewArray_Key, std::make_any<bool>(true));
  args.insertOrAssign(ExtractComponentAsArrayFilter::k_RemoveComponentsFromArray_Key, std::make_any<bool>(true));
  args.insertOrAssign(ExtractComponentAsArrayFilter::k_CompNumber_Key, std::make_any<int32>(removeCompIndex));
  args.insertOrAssign(ExtractComponentAsArrayFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(k_QuatsPath));
  args.insertOrAssign(ExtractComponentAsArrayFilter::k_NewArrayName_Key, std::make_any<std::string>(k_ExtractedComponents));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(alteredDs, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(alteredDs, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Load a clean copy of the datastructure prior to resize because original array is terminated after execution
  DataStructure unalteredDs = UnitTest::LoadDataStructure(k_BaseDataFilePath);
  const auto& originalQuatsArray = unalteredDs.getDataRefAs<Float32Array>(k_QuatsPath); // clean array [exemplar]

  const auto& reducedQuatsArray = alteredDs.getDataRefAs<Float32Array>(k_QuatsPath);
  const auto& extractedComponentsArray = alteredDs.getDataRefAs<Float32Array>(k_ExtractedComponentsPath);

  const usize originalTupleCount = originalQuatsArray.getNumberOfTuples();
  REQUIRE(originalTupleCount == reducedQuatsArray.getNumberOfTuples());
  REQUIRE(originalTupleCount == extractedComponentsArray.getNumberOfTuples());

  const usize originalCompCount = originalQuatsArray.getNumberOfComponents();
  const usize reducedCompCount = reducedQuatsArray.getNumberOfComponents();
  REQUIRE((originalCompCount - 1) == reducedCompCount);
  REQUIRE(1 == extractedComponentsArray.getNumberOfComponents());

  usize extractedIndex = 0;
  for(usize tupleIndex = 0; tupleIndex < originalTupleCount; tupleIndex++)
  {
    for(usize compIndex = 0; compIndex < originalCompCount; compIndex++)
    {
      usize originalIndex = tupleIndex * originalCompCount + compIndex;
      usize reducedIndex = tupleIndex * reducedCompCount + compIndex;
      if(compIndex == removeCompIndex)
      {
        REQUIRE(originalQuatsArray[originalIndex] == extractedComponentsArray[extractedIndex]);
        extractedIndex++;
      }
      else
      {
        if(compIndex > removeCompIndex)
        {
          REQUIRE(originalQuatsArray[originalIndex] == reducedQuatsArray[reducedIndex - 1]); // account for having one less comp
        }
        else
        {
          REQUIRE(originalQuatsArray[originalIndex] == reducedQuatsArray[reducedIndex]);
        }
      }
    }
  }

  UnitTest::CheckArraysInheritTupleDims(alteredDs);
}

TEST_CASE("SimplnxCore::ExtractComponentAsArrayFilter: InValid filter execution", "[SimplnxCore][ExtractComponentAsArrayFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_find_feature_centroids.tar.gz", "6_6_find_feature_centroids.dream3d");

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ExtractComponentAsArrayFilter filter;
  DataStructure dataStructure = UnitTest::LoadDataStructure(k_BaseDataFilePath);
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ExtractComponentAsArrayFilter::k_MoveComponentsToNewArray_Key, std::make_any<bool>(true));
  args.insertOrAssign(ExtractComponentAsArrayFilter::k_RemoveComponentsFromArray_Key, std::make_any<bool>(true));
  args.insertOrAssign(ExtractComponentAsArrayFilter::k_CompNumber_Key, std::make_any<int32>(5)); // Invalid
  args.insertOrAssign(ExtractComponentAsArrayFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(k_QuatsPath));
  args.insertOrAssign(ExtractComponentAsArrayFilter::k_NewArrayName_Key, std::make_any<std::string>(k_ExtractedComponents));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(!preflightResult.outputActions.valid());

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ExtractComponentAsArrayFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ExtractComponentAsArrayFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ExtractComponentAsArrayFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ExtractComponentAsArrayFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ExtractComponentAsArrayFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<int32>(ExtractComponentAsArrayFilter::k_CompNumber_Key) == 5);
      CHECK(args.value<DataPath>(ExtractComponentAsArrayFilter::k_SelectedArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ExtractComponentAsArrayFilter::k_NewArrayName_Key) == "TestName");
    }
  }
}
