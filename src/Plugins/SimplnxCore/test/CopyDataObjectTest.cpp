#include "SimplnxCore/Filters/CopyDataObjectFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
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
#include <string>
#include <vector>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
const int32 k_TupleDimMismatchWarningCode = -27361;
}

TEST_CASE("SimplnxCore::CopyDataObjectFilter(Valid Execution)", "[SimplnxCore][CopyDataObjectFilter]")
{
  unit_test::LoadPlugins();

  static const DataPath kDataPath1({constants::k_SmallIN100, "Phase Data"});
  static const DataPath kDataPath2({constants::k_SmallIN100, constants::k_EbsdScanData, "Phases"});

  CopyDataObjectFilter filter;
  DataStructure dataStructure = unit_test::CreateDataStructure();
  Arguments args;

  SECTION("Copy to Same Parent")
  {
    args.insert(CopyDataObjectFilter::k_DataPath_Key, std::make_any<MultiPathSelectionParameter::ValueType>({kDataPath1, kDataPath2}));
    args.insert(CopyDataObjectFilter::k_UseNewParent_Key, std::make_any<bool>(false));
    args.insert(CopyDataObjectFilter::k_NewPathSuffix_Key, std::make_any<std::string>("_COPY"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    const auto* path1CopyPtr = dataStructure.getDataAs<DataGroup>(DataPath({constants::k_SmallIN100, "Phase Data_COPY"}));
    const auto* path2CopyPtr = dataStructure.getDataAs<Int32ArrayType>(DataPath({constants::k_SmallIN100, constants::k_EbsdScanData, "Phases_COPY"}));
    REQUIRE(path1CopyPtr != nullptr);
    REQUIRE(path2CopyPtr != nullptr);
  }
  SECTION("Copy to New Parent")
  {
    static const DataPath kCopyPath({constants::k_SmallIN100});

    args.insert(CopyDataObjectFilter::k_DataPath_Key, std::make_any<MultiPathSelectionParameter::ValueType>({kDataPath1, kDataPath2}));
    args.insert(CopyDataObjectFilter::k_UseNewParent_Key, std::make_any<bool>(true));
    args.insert(CopyDataObjectFilter::k_NewPath_Key, std::make_any<DataPath>(kCopyPath));
    args.insert(CopyDataObjectFilter::k_NewPathSuffix_Key, std::make_any<std::string>("_COPY"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    DataGroup* copiedDataGroupPtr = dataStructure.getDataAs<DataGroup>(kCopyPath.createChildPath(kDataPath1.getTargetName() + "_COPY"));
    Int32ArrayType* copiedArrayPtr = dataStructure.getDataAs<Int32ArrayType>(kCopyPath.createChildPath(kDataPath2.getTargetName() + "_COPY"));
    REQUIRE(copiedDataGroupPtr != nullptr);
    REQUIRE(copiedArrayPtr != nullptr);
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CopyDataObjectFilter(Invalid Parameters)", "[SimplnxCore][CopyDataObjectFilter]")
{
  unit_test::LoadPlugins();

  CopyDataObjectFilter filter;
  DataStructure dataStructure = unit_test::CreateDataStructure();
  Arguments args;

  SECTION("Data to be copied does not exist")
  {
    const DataPath dataPath({constants::k_SmallIN100, "Bad Data Foo"});

    args.insert(CopyDataObjectFilter::k_DataPath_Key, std::make_any<MultiPathSelectionParameter::ValueType>({dataPath}));

    auto result = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
  }
  SECTION("Same parent copy data suffix is empty")
  {
    const DataPath dataPath({constants::k_SmallIN100, "Phase Data"});
    const DataPath copyPath({constants::k_SmallIN100, constants::k_EbsdScanData});

    args.insert(CopyDataObjectFilter::k_DataPath_Key, std::make_any<MultiPathSelectionParameter::ValueType>({dataPath}));
    args.insert(CopyDataObjectFilter::k_UseNewParent_Key, std::make_any<bool>(false));
    args.insert(CopyDataObjectFilter::k_NewPathSuffix_Key, std::make_any<std::string>(""));

    auto result = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
  }
  SECTION("Copy data new parent tuple mismatch")
  {
    auto* attributeMatrixPtr = AttributeMatrix::Create(dataStructure, "TestAttributeMatrix", {10, 5, 1});

    const DataPath dataPath({constants::k_SmallIN100, constants::k_EbsdScanData, "Phases"});
    const DataPath copyPath({"TestAttributeMatrix"});

    args.insert(CopyDataObjectFilter::k_DataPath_Key, std::make_any<MultiPathSelectionParameter::ValueType>({dataPath}));
    args.insert(CopyDataObjectFilter::k_UseNewParent_Key, std::make_any<bool>(true));
    args.insert(CopyDataObjectFilter::k_NewPath_Key, std::make_any<DataPath>(copyPath));
    args.insert(CopyDataObjectFilter::k_NewPathSuffix_Key, std::make_any<std::string>("_COPY"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);

    REQUIRE_FALSE(preflightResult.output_actions.warnings().empty());

    bool found = false;
    for(const auto& warning : preflightResult.output_actions.warnings())
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

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CopyDataObjectFilter: SIMPL Backwards Compatibility", "[SimplnxCore][CopyDataObjectFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "CopyDataObjectFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "CopyDataObjectFilter.json"},
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
      REQUIRE(filterPtr->uuid() == FilterTraits<CopyDataObjectFilter>::uuid);

      const Arguments args = pipelineFilterPtr->getArguments();
      CHECK(args.value<std::string>(CopyDataObjectFilter::k_NewPathSuffix_Key) == "TestName");
    }
  }
}

TEST_CASE("SimplnxCore::CopyDataObject: Child names containing the geometry name are kept", "[SimplnxCore][CopyDataObject]")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure;
  auto* groupPtr = DataGroup::Create(dataStructure, "A");
  REQUIRE(groupPtr != nullptr);
  auto* childPtr = DataGroup::Create(dataStructure, "A child", groupPtr->getId());
  REQUIRE(childPtr != nullptr);
  auto* arrayPtr =
      Int32ArrayType::Create(dataStructure, "x", data_store_utilities::CreateDataStore<int32>(dataStructure, childPtr->getDataPaths().front().createChildPath("x"), {2}, {1}), childPtr->getId());
  REQUIRE(arrayPtr != nullptr);
  arrayPtr->fill(7);
  CopyDataObjectFilter filter;
  Arguments args;
  args.insertOrAssign(CopyDataObjectFilter::k_DataPath_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{DataPath({"A"})}));
  args.insertOrAssign(CopyDataObjectFilter::k_NewPathSuffix_Key, std::make_any<std::string>("_copy"));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

  SECTION("Declared recursive paths keep child names")
  {
    const CopyDataObjectAction* copyActionPtr = nullptr;
    for(const auto& action : preflightResult.output_actions.value().actions)
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
    REQUIRE(dataStructure.getDataAs<Int32ArrayType>(DataPath({"A_copy", "A child", "x"})) != nullptr);
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CopyDataObject: Recursive paths use the new parent", "[SimplnxCore][CopyDataObject]")
{
  unit_test::LoadPlugins();
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
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  REQUIRE(preflightResult.output_actions.value().actions.size() == 1);
  const auto* copyActionPtr = dynamic_cast<const CopyDataObjectAction*>(preflightResult.output_actions.value().actions.front().get());
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
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CopyDataObjectFilter: StringArray copy has independent values", "[SimplnxCore][CopyDataObjectFilter][StringArray]")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure;
  auto* original = StringArray::CreateWithValues(dataStructure, "Original", {2}, {"a", "b"});
  REQUIRE(original != nullptr);
  CopyDataObjectFilter filter;
  Arguments args;
  args.insert(CopyDataObjectFilter::k_DataPath_Key, std::make_any<MultiPathSelectionParameter::ValueType>(MultiPathSelectionParameter::ValueType{DataPath({"Original"})}));
  args.insert(CopyDataObjectFilter::k_UseNewParent_Key, std::make_any<bool>(false));
  args.insert(CopyDataObjectFilter::k_NewPathSuffix_Key, std::make_any<std::string>("_COPY"));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  auto* copy = dataStructure.getDataAs<StringArray>(DataPath({"Original_COPY"}));
  REQUIRE(copy != nullptr);
  REQUIRE(copy != original);
  REQUIRE(copy->getTupleShape() == ShapeType{2});
  REQUIRE(copy->values() == std::vector<std::string>{"a", "b"});
  (*copy)[0] = "z";
  REQUIRE((*copy)[0] == "z");
  REQUIRE((*original)[0] == "a");
  (*original)[1] = "y";
  REQUIRE((*original)[1] == "y");
  REQUIRE((*copy)[1] == "b");
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
