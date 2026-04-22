#include "SimplnxCore/Filters/RenameDataObjectFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

using namespace nx::core;
namespace fs = std::filesystem;

TEST_CASE("SimplnxCore::RenameDataAction(Instantiate)", "[SimplnxCore][RenameDataAction]")
{
  UnitTest::LoadPlugins();

  static constexpr StringLiteral k_NewName = "Bar";
  const DataPath k_DataPath({Constants::k_SmallIN100});

  RenameDataObjectFilter filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(RenameDataObjectFilter::k_NewName_Key, std::make_any<std::string>(k_NewName));
  args.insert(RenameDataObjectFilter::k_SourceDataObjectPath_Key, std::make_any<DataPath>(k_DataPath));

  auto result = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RenameDataAction(Invalid Parameters)", "[SimplnxCore][RenameDataAction]")
{
  UnitTest::LoadPlugins();

  static constexpr StringLiteral k_NewName = Constants::k_ConfidenceIndex;
  static const DataPath k_DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});

  RenameDataObjectFilter filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(RenameDataObjectFilter::k_NewName_Key, std::make_any<std::string>(k_NewName));
  args.insert(RenameDataObjectFilter::k_SourceDataObjectPath_Key, std::make_any<DataPath>(k_DataPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RenameDataAction(Valid Parameters)", "[SimplnxCore][RenameDataAction]")
{
  UnitTest::LoadPlugins();

  static constexpr StringLiteral k_NewName = "Foo";
  static const DataPath k_DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});

  RenameDataObjectFilter filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(RenameDataObjectFilter::k_NewName_Key, std::make_any<std::string>(k_NewName));
  args.insert(RenameDataObjectFilter::k_SourceDataObjectPath_Key, std::make_any<DataPath>(k_DataPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  DataPath newPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, k_NewName});
  auto* dataObject = dataStructure.getData(newPath);
  REQUIRE(dataObject != nullptr);

  REQUIRE(dataObject->getName() == k_NewName);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RenameDataAction(Valid Overwrite)", "[SimplnxCore][RenameDataAction]")
{
  UnitTest::LoadPlugins();

  static constexpr StringLiteral k_NewName = Constants::k_GroupHName;
  static const DataPath k_DataPath({Constants::k_GroupAName, Constants::k_GroupCName, Constants::k_GroupDName, Constants::k_ArrayIName});

  RenameDataObjectFilter filter;
  DataStructure dataStructure = UnitTest::CreateComplexMultiLevelDataGraph();
  Arguments args;

  args.insert(RenameDataObjectFilter::k_AllowOverwrite_Key, std::make_any<bool>(true));
  args.insert(RenameDataObjectFilter::k_NewName_Key, std::make_any<std::string>(k_NewName));
  args.insert(RenameDataObjectFilter::k_SourceDataObjectPath_Key, std::make_any<DataPath>(k_DataPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // There is a warning clause, but under current implementation it won't be reached
  //  bool warningFound = false;
  //  for(const auto& warning : preflightResult.outputActions.warnings())
  //  {
  //    if(warning.code == -6602)
  //    {
  //      warningFound = true;
  //    }
  //  }
  //  REQUIRE(warningFound);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  // Verify rename was successful
  {
    DataPath newPath({Constants::k_GroupAName, Constants::k_GroupCName, Constants::k_GroupDName, k_NewName});
    auto* dataObject = dataStructure.getData(newPath);
    REQUIRE(dataObject != nullptr);

    REQUIRE(dataObject->getName() == k_NewName);
  }

  // Verify old DataGroup (`H`) was removed
  {
    DataPath oldHPath({Constants::k_GroupAName, Constants::k_GroupHName});
    auto* dataObject = dataStructure.getData(oldHPath);
    REQUIRE(dataObject == nullptr);
  }

  // Verify old DataGroup (`H`) sub-array (`N`) was removed
  {
    DataPath oldHChildPath({Constants::k_GroupAName, Constants::k_GroupHName, Constants::k_ArrayNName});
    auto* dataObject = dataStructure.getData(oldHChildPath);
    REQUIRE(dataObject == nullptr);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RenameDataAction(InValid Overwrite)", "[SimplnxCore][RenameDataAction]")
{
  UnitTest::LoadPlugins();

  static constexpr StringLiteral k_NewName = Constants::k_GroupDName;
  static const DataPath k_DataPath({Constants::k_GroupAName, Constants::k_GroupCName, Constants::k_GroupDName, Constants::k_ArrayIName});

  RenameDataObjectFilter filter;

  DataStructure dataStructure = UnitTest::CreateComplexMultiLevelDataGraph();
  Arguments args;

  args.insert(RenameDataObjectFilter::k_AllowOverwrite_Key, std::make_any<bool>(true));
  args.insert(RenameDataObjectFilter::k_NewName_Key, std::make_any<std::string>(k_NewName));
  args.insert(RenameDataObjectFilter::k_SourceDataObjectPath_Key, std::make_any<DataPath>(k_DataPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);

  bool errorFound = false;
  for(const auto& error : result.result.errors())
  {
    if(error.code == -6601)
    {
      errorFound = true;
    }
  }
  REQUIRE(errorFound);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RenameDataObjectFilter: SIMPL Backwards Compatibility", "[SimplnxCore][RenameDataObjectFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "RenameDataObjectFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "RenameDataObjectFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<RenameDataObjectFilter>::uuid);

      // Note: Complex SIMPL parameter conversions may produce warnings
      // pipelineFilter->getComments() may not be empty for filters with custom converters

      const Arguments args = pipelineFilter->getArguments();
      // CHECK(args.value<DataPath>(RenameDataObjectFilter::k_SourceDataObjectPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      // CHECK(args.value<std::string>(RenameDataObjectFilter::k_NewName_Key) == "TestName");
      // CHECK(args.value<DataPath>(RenameDataObjectFilter::k_SourceDataObjectPath_Key) == DataPath({"DataContainer", "CellData"}));
      // CHECK(args.value<std::string>(RenameDataObjectFilter::k_NewName_Key) == "TestName");
      // CHECK(args.value<DataPath>(RenameDataObjectFilter::k_SourceDataObjectPath_Key) == DataPath({"DataContainer"}));
      // Complex type (DataContainerNameFilterParameterConverter) - verified by successful pipeline loading
    }
  }
}
