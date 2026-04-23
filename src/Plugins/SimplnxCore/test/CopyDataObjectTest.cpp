#include "SimplnxCore/Filters/CopyDataObjectFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/MultiPathSelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
const int32 k_TupleDimMismatchWarningCode = -27361;
}

TEST_CASE("SimplnxCore::CopyDataObjectFilter(Valid Execution)", "[SimplnxCore][CopyDataObjectFilter]")
{
  UnitTest::LoadPlugins();

  static const DataPath k_DataPath1({Constants::k_SmallIN100, "Phase Data"});
  static const DataPath k_DataPath2({Constants::k_SmallIN100, Constants::k_EbsdScanData, "Phases"});

  CopyDataObjectFilter filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  SECTION("Copy to Same Parent")
  {
    args.insert(CopyDataObjectFilter::k_DataPath_Key, std::make_any<MultiPathSelectionParameter::ValueType>({k_DataPath1, k_DataPath2}));
    args.insert(CopyDataObjectFilter::k_UseNewParent_Key, std::make_any<bool>(false));
    args.insert(CopyDataObjectFilter::k_NewPathSuffix_Key, std::make_any<std::string>("_COPY"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    const auto* path1Copy = dataStructure.getDataAs<DataGroup>(DataPath({Constants::k_SmallIN100, "Phase Data_COPY"}));
    const auto* path2Copy = dataStructure.getDataAs<Int32Array>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "Phases_COPY"}));
    REQUIRE(path1Copy != nullptr);
    REQUIRE(path2Copy != nullptr);
  }
  SECTION("Copy to New Parent")
  {
    static const DataPath k_CopyPath({Constants::k_SmallIN100});

    args.insert(CopyDataObjectFilter::k_DataPath_Key, std::make_any<MultiPathSelectionParameter::ValueType>({k_DataPath1, k_DataPath2}));
    args.insert(CopyDataObjectFilter::k_UseNewParent_Key, std::make_any<bool>(true));
    args.insert(CopyDataObjectFilter::k_NewPath_Key, std::make_any<DataPath>(k_CopyPath));
    args.insert(CopyDataObjectFilter::k_NewPathSuffix_Key, std::make_any<std::string>("_COPY"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    DataGroup* copiedDataGroup = dataStructure.getDataAs<DataGroup>(k_CopyPath.createChildPath(k_DataPath1.getTargetName() + "_COPY"));
    Int32Array* copiedArray = dataStructure.getDataAs<Int32Array>(k_CopyPath.createChildPath(k_DataPath2.getTargetName() + "_COPY"));
    REQUIRE(copiedDataGroup != nullptr);
    REQUIRE(copiedArray != nullptr);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CopyDataObjectFilter(Invalid Parameters)", "[SimplnxCore][CopyDataObjectFilter]")
{
  UnitTest::LoadPlugins();

  CopyDataObjectFilter filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  SECTION("Data to be copied does not exist")
  {
    const DataPath dataPath({Constants::k_SmallIN100, "Bad Data Foo"});

    args.insert(CopyDataObjectFilter::k_DataPath_Key, std::make_any<MultiPathSelectionParameter::ValueType>({dataPath}));

    auto result = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  }
  SECTION("Same parent copy data suffix is empty")
  {
    const DataPath dataPath({Constants::k_SmallIN100, "Phase Data"});
    const DataPath copyPath({Constants::k_SmallIN100, Constants::k_EbsdScanData});

    args.insert(CopyDataObjectFilter::k_DataPath_Key, std::make_any<MultiPathSelectionParameter::ValueType>({dataPath}));
    args.insert(CopyDataObjectFilter::k_UseNewParent_Key, std::make_any<bool>(false));
    args.insert(CopyDataObjectFilter::k_NewPathSuffix_Key, std::make_any<std::string>(""));

    auto result = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  }
  SECTION("Copy data new parent tuple mismatch")
  {
    auto* attributeMatrix = AttributeMatrix::Create(dataStructure, "TestAttributeMatrix", {10, 5, 1});

    const DataPath dataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "Phases"});
    const DataPath copyPath({"TestAttributeMatrix"});

    args.insert(CopyDataObjectFilter::k_DataPath_Key, std::make_any<MultiPathSelectionParameter::ValueType>({dataPath}));
    args.insert(CopyDataObjectFilter::k_UseNewParent_Key, std::make_any<bool>(true));
    args.insert(CopyDataObjectFilter::k_NewPath_Key, std::make_any<DataPath>(copyPath));
    args.insert(CopyDataObjectFilter::k_NewPathSuffix_Key, std::make_any<std::string>("_COPY"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);

    REQUIRE_FALSE(preflightResult.outputActions.warnings().empty());

    bool found = false;
    for(const auto& warning : preflightResult.outputActions.warnings())
    {
      if(warning.code == ::k_TupleDimMismatchWarningCode)
      {
        found = true;
      }
    }

    REQUIRE(found);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CopyDataObjectFilter: SIMPL Backwards Compatibility", "[SimplnxCore][CopyDataObjectFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "CopyDataObjectFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "CopyDataObjectFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<CopyDataObjectFilter>::uuid);

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<std::string>(CopyDataObjectFilter::k_NewPathSuffix_Key) == "TestName");
    }
  }
}
