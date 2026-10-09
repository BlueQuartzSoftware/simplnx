#include "SimplnxCore/Filters/CopyDataObjectFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Filter/Actions/CopyDataObjectAction.hpp"
#include "simplnx/Parameters/MultiPathSelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
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

TEST_CASE("SimplnxCore::CopyDataObject: Child names containing the geometry name are kept", "[SimplnxCore][CopyDataObject]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  auto* groupPtr = DataGroup::Create(dataStructure, "A");
  REQUIRE(groupPtr != nullptr);
  auto* childPtr = DataGroup::Create(dataStructure, "A child", groupPtr->getId());
  REQUIRE(childPtr != nullptr);
  auto* arrayPtr =
      Int32Array::Create(dataStructure, "x", DataStoreUtilities::CreateDataStore<int32>(dataStructure, childPtr->getDataPaths().front().createChildPath("x"), {2}, {1}), childPtr->getId());
  REQUIRE(arrayPtr != nullptr);
  arrayPtr->fill(7);
  CopyDataObjectFilter filter;
  Arguments args;
  args.insertOrAssign(CopyDataObjectFilter::k_DataPath_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{DataPath({"A"})}));
  args.insertOrAssign(CopyDataObjectFilter::k_NewPathSuffix_Key, std::make_any<std::string>("_copy"));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  SECTION("Declared recursive paths keep child names")
  {
    const CopyDataObjectAction* copyActionPtr = nullptr;
    for(const auto& action : preflightResult.outputActions.value().actions)
    {
      const auto* candidatePtr = dynamic_cast<const CopyDataObjectAction*>(action.get());
      if(candidatePtr != nullptr && candidatePtr->path() == DataPath({"A"}))
      {
        copyActionPtr = candidatePtr;
        break;
      }
    }
    REQUIRE(copyActionPtr != nullptr);
    const auto createdPaths = copyActionPtr->getAllCreatedPaths();
    REQUIRE(std::find(createdPaths.begin(), createdPaths.end(), DataPath({"A_copy", "A child"})) != createdPaths.end());
    REQUIRE(std::find(createdPaths.begin(), createdPaths.end(), DataPath({"A_copy", "A child", "x"})) != createdPaths.end());
    REQUIRE(std::find(createdPaths.begin(), createdPaths.end(), DataPath({"A_copy", "A_copy child"})) == createdPaths.end());
  }
  SECTION("Copied objects keep child names")
  {
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(dataStructure.getDataAs<Int32Array>(DataPath({"A_copy", "A child", "x"})) != nullptr);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CopyDataObject: Recursive paths use the new parent", "[SimplnxCore][CopyDataObject]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  auto* oldParentPtr = DataGroup::Create(dataStructure, "OldParent");
  REQUIRE(oldParentPtr != nullptr);
  auto* groupPtr = DataGroup::Create(dataStructure, "A", oldParentPtr->getId());
  REQUIRE(groupPtr != nullptr);
  auto* childPtr = DataGroup::Create(dataStructure, "child", groupPtr->getId());
  REQUIRE(childPtr != nullptr);
  auto* newParentPtr = DataGroup::Create(dataStructure, "NewParent");
  REQUIRE(newParentPtr != nullptr);

  const DataPath sourcePath({"OldParent", "A"});
  const DataPath copiedPath({"NewParent", "A_copy"});
  const DataPath copiedChildPath = copiedPath.createChildPath("child");
  CopyDataObjectFilter filter;
  Arguments args;
  args.insertOrAssign(CopyDataObjectFilter::k_DataPath_Key, std::make_any<MultiPathSelectionParameter::ValueType>(MultiPathSelectionParameter::ValueType{sourcePath}));
  args.insertOrAssign(CopyDataObjectFilter::k_UseNewParent_Key, true);
  args.insertOrAssign(CopyDataObjectFilter::k_NewPath_Key, std::make_any<DataPath>(DataPath({"NewParent"})));
  args.insertOrAssign(CopyDataObjectFilter::k_NewPathSuffix_Key, std::make_any<std::string>("_copy"));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.value().actions.size() == 1);
  const auto* copyActionPtr = dynamic_cast<const CopyDataObjectAction*>(preflightResult.outputActions.value().actions.front().get());
  REQUIRE(copyActionPtr != nullptr);
  REQUIRE(copyActionPtr->path() == sourcePath);
  REQUIRE(copyActionPtr->newPath() == copiedPath);
  const auto createdPaths = copyActionPtr->getAllCreatedPaths();
  REQUIRE(createdPaths.size() == 2);
  REQUIRE(std::find(createdPaths.begin(), createdPaths.end(), copiedPath) != createdPaths.end());
  REQUIRE(std::find(createdPaths.begin(), createdPaths.end(), copiedChildPath) != createdPaths.end());
  REQUIRE(std::find(createdPaths.begin(), createdPaths.end(), DataPath({"OldParent", "A_copy", "child"})) == createdPaths.end());

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const auto* copiedGroupPtr = dataStructure.getDataAs<DataGroup>(copiedPath);
  const auto* copiedChildPtr = dataStructure.getDataAs<DataGroup>(copiedChildPath);
  REQUIRE(copiedGroupPtr != nullptr);
  REQUIRE(copiedChildPtr != nullptr);
  REQUIRE(copiedGroupPtr->getId() != groupPtr->getId());
  REQUIRE(copiedChildPtr->getId() != childPtr->getId());
  REQUIRE(dataStructure.getDataAs<DataGroup>(DataPath({"OldParent", "A_copy"})) == nullptr);
  REQUIRE(dataStructure.getDataAs<DataGroup>(DataPath({"OldParent", "A_copy", "child"})) == nullptr);
  REQUIRE(dataStructure.getDataAs<DataGroup>(sourcePath) == groupPtr);
  REQUIRE(dataStructure.getDataAs<DataGroup>(sourcePath.createChildPath("child")) == childPtr);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
