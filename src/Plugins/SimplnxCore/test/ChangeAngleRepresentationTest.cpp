#include "SimplnxCore/Filters/ChangeAngleRepresentationFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <array>
#include <catch2/catch.hpp>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <nonstd/span.hpp>

using namespace nx::core;
namespace fs = std::filesystem;

TEST_CASE("SimplnxCore::ChangeAngleRepresentationFilter: Invalid Execution", "[OrientationAnalysis][ChangeAngleRepresentationFilter]")
{
  // Configure the filter arguments.
  ChangeAngleRepresentationFilter filter;
  DataStructure dataStructure;
  Arguments args;

  // This should fail
  args.insertOrAssign(ChangeAngleRepresentationFilter::k_ConversionType_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(ChangeAngleRepresentationFilter::k_AnglesArrayPath_Key, std::make_any<DataPath>(DataPath{}));

  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.invalid());

  // This should fail because parameter is out of range
  args.insertOrAssign(ChangeAngleRepresentationFilter::k_ConversionType_Key, std::make_any<ChoicesParameter::ValueType>(2));
  preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.invalid());

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ChangeAngleRepresentationFilter: Degrees To Radians")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  UnitTest::LoadPlugins();

  // Configure the filter arguments.
  ChangeAngleRepresentationFilter filter;
  DataStructure dataStructure;
  Arguments args;

  DataGroup* topLevelGroup = DataGroup::Create(dataStructure, Constants::k_SmallIN100);
  DataGroup* scanData = DataGroup::Create(dataStructure, Constants::k_EbsdScanData, topLevelGroup->getId());

  std::vector<size_t> tupleShape = {10};
  std::vector<size_t> componentShape = {3};

  Float32Array* angles = UnitTest::CreateTestDataArray<float>(dataStructure, Constants::k_EulerAngles, tupleShape, componentShape, scanData->getId());

  for(size_t t = 0; t < tupleShape[0]; t++)
  {
    for(size_t c = 0; c < componentShape[0]; c++)
    {
      (*angles)[t * componentShape[0] + c] = static_cast<float>(t * c);
    }
  }

  // This should fail
  args.insertOrAssign(ChangeAngleRepresentationFilter::k_ConversionType_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(ChangeAngleRepresentationFilter::k_AnglesArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_EulerAngles})));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Check the results
  float d2r = nx::core::numbers::pi / 180.0f;
  for(size_t t = 0; t < tupleShape[0]; t++)
  {
    for(size_t c = 0; c < componentShape[0]; c++)
    {
      REQUIRE((*angles)[t * componentShape[0] + c] == static_cast<float>(t * c) * d2r);
    }
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ChangeAngleRepresentationFilter: Radians To Degrees")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  UnitTest::LoadPlugins();

  // Configure the filter arguments.
  ChangeAngleRepresentationFilter filter;
  DataStructure dataStructure;
  Arguments args;

  DataGroup* topLevelGroup = DataGroup::Create(dataStructure, Constants::k_SmallIN100);
  DataGroup* scanData = DataGroup::Create(dataStructure, Constants::k_EbsdScanData, topLevelGroup->getId());

  std::vector<size_t> tupleShape = {10};
  std::vector<size_t> componentShape = {3};

  Float32Array* angles = UnitTest::CreateTestDataArray<float>(dataStructure, Constants::k_EulerAngles, tupleShape, componentShape, scanData->getId());

  for(size_t t = 0; t < tupleShape[0]; t++)
  {
    for(size_t c = 0; c < componentShape[0]; c++)
    {
      (*angles)[t * componentShape[0] + c] = static_cast<float>(t * c);
    }
  }

  // This should fail
  args.insertOrAssign(ChangeAngleRepresentationFilter::k_ConversionType_Key, std::make_any<ChoicesParameter::ValueType>(1));
  args.insertOrAssign(ChangeAngleRepresentationFilter::k_AnglesArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_EulerAngles})));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Check the results
  float r2d = 180.0f / nx::core::numbers::pi;
  for(size_t t = 0; t < tupleShape[0]; t++)
  {
    for(size_t c = 0; c < componentShape[0]; c++)
    {
      REQUIRE((*angles)[t * componentShape[0] + c] == static_cast<float>(t * c) * r2d);
    }
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ChangeAngleRepresentationFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ChangeAngleRepresentationFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ChangeAngleRepresentationFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ChangeAngleRepresentationFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ChangeAngleRepresentationFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<ChoicesParameter::ValueType>(ChangeAngleRepresentationFilter::k_ConversionType_Key) == 0);
      CHECK(args.value<DataPath>(ChangeAngleRepresentationFilter::k_AnglesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
    }
  }
}
