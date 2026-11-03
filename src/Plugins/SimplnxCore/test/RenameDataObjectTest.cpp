#include "SimplnxCore/Filters/RenameDataObjectFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/ScalarData.hpp"
#include "simplnx/Filter/Actions/RenameDataAction.hpp"
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
  const auto sourceId = dataStructure.getData(k_DataPath)->getId();
  auto* parent = dataStructure.getDataAs<DataGroup>(k_DataPath.getParent());
  REQUIRE(parent != nullptr);
  auto* sibling = DataGroup::Create(dataStructure, k_NewName, parent->getId());
  REQUIRE(sibling != nullptr);
  const auto siblingId = sibling->getId();
  auto* child = ScalarData<int32>::Create(dataStructure, "OverwriteChild", 37, siblingId);
  REQUIRE(child != nullptr);
  const auto childId = child->getId();
  Arguments args;

  args.insert(RenameDataObjectFilter::k_AllowOverwrite_Key, std::make_any<bool>(true));
  args.insert(RenameDataObjectFilter::k_NewName_Key, std::make_any<std::string>(k_NewName));
  args.insert(RenameDataObjectFilter::k_SourceDataObjectPath_Key, std::make_any<DataPath>(k_DataPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  RenameDataAction action(k_DataPath, k_NewName, true);
  auto actionResult = action.apply(dataStructure, IDataAction::Mode::Preflight);
  SIMPLNX_RESULT_REQUIRE_VALID(actionResult);
  REQUIRE(actionResult.warnings().size() == 1);
  CHECK(actionResult.warnings()[0].code == -6602);
  CHECK(dataStructure.getData(siblingId) != nullptr);
  CHECK(dataStructure.getDataAs<ScalarData<int32>>(childId)->getValue() == 37);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  CHECK(dataStructure.getData(siblingId) == nullptr);
  CHECK(dataStructure.getData(childId) == nullptr);
  REQUIRE(dataStructure.getData(k_DataPath.replaceName(k_NewName)) != nullptr);
  CHECK(dataStructure.getData(k_DataPath.replaceName(k_NewName))->getId() == sourceId);

  // Verify rename was successful
  {
    DataPath newPath({Constants::k_GroupAName, Constants::k_GroupCName, Constants::k_GroupDName, k_NewName});
    auto* dataObject = dataStructure.getData(newPath);
    REQUIRE(dataObject != nullptr);

    REQUIRE(dataObject->getName() == k_NewName);
  }

  // Verify the unrelated DataGroup (`H`) is preserved
  {
    DataPath oldHPath({Constants::k_GroupAName, Constants::k_GroupHName});
    auto* dataObject = dataStructure.getData(oldHPath);
    REQUIRE(dataObject != nullptr);
  }

  // Verify its child (`N`) is preserved
  {
    DataPath oldHChildPath({Constants::k_GroupAName, Constants::k_GroupHName, Constants::k_ArrayNName});
    auto* dataObject = dataStructure.getData(oldHChildPath);
    REQUIRE(dataObject != nullptr);
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

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(RenameDataObjectFilter::k_SourceDataObjectPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(RenameDataObjectFilter::k_NewName_Key) == "TestName");
    }
  }
}

TEST_CASE("SimplnxCore::RenameDataAction sibling overwrite regression", "[SimplnxCore][RenameDataAction]")
{
  UnitTest::LoadPlugins();
  const bool topLevel = GENERATE(false, true);
  const bool hasSibling = GENERATE(false, true);
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  CAPTURE(topLevel, hasSibling, mode);
  DataStructure dataStructure;
  auto* groupA = DataGroup::Create(dataStructure, "GroupA");
  REQUIRE(groupA != nullptr);
  auto* unrelated = ScalarData<int32>::Create(dataStructure, "Target", 11, groupA->getId());
  REQUIRE(unrelated != nullptr);
  const auto unrelatedId = unrelated->getId();
  auto* groupB = DataGroup::Create(dataStructure, "GroupB");
  REQUIRE(groupB != nullptr);
  const std::optional<DataObject::IdType> parentId = topLevel ? std::nullopt : std::make_optional(groupB->getId());
  auto* source = ScalarData<int32>::Create(dataStructure, "Source", 22, parentId);
  REQUIRE(source != nullptr);
  const auto sourceId = source->getId();
  std::optional<DataObject::IdType> siblingId;
  if(hasSibling)
  {
    auto* sibling = ScalarData<int32>::Create(dataStructure, "Target", 33, parentId);
    REQUIRE(sibling != nullptr);
    siblingId = sibling->getId();
  }
  const DataPath sourcePath = topLevel ? DataPath({"Source"}) : DataPath({"GroupB", "Source"});
  RenameDataObjectFilter filter;
  Arguments args;
  args.insert(RenameDataObjectFilter::k_AllowOverwrite_Key, std::make_any<bool>(true));
  args.insert(RenameDataObjectFilter::k_NewName_Key, std::make_any<std::string>("Target"));
  args.insert(RenameDataObjectFilter::k_SourceDataObjectPath_Key, std::make_any<DataPath>(sourcePath));
  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  if(mode == IDataAction::Mode::Preflight)
  {
    // Apply the action explicitly: filter preflight only constructs output actions.
    RenameDataAction action(sourcePath, "Target", true);
    auto result = action.apply(dataStructure, mode);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    CHECK(result.warnings().size() == (hasSibling ? 1 : 0));
    if(hasSibling)
    {
      REQUIRE_FALSE(result.warnings().empty());
      CHECK(result.warnings()[0].code == -6602);
    }
  }
  else
  {
    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    REQUIRE(dataStructure.getData(sourcePath.replaceName("Target")) != nullptr);
    CHECK(dataStructure.getData(sourcePath.replaceName("Target"))->getId() == sourceId);
    CHECK(dataStructure.getData(sourcePath) == nullptr);
    CHECK(dataStructure.getDataAs<ScalarData<int32>>(sourceId)->getValue() == 22);
    if(siblingId.has_value())
    {
      CHECK(dataStructure.getData(*siblingId) == nullptr);
    }
  }
  REQUIRE(dataStructure.getDataAs<ScalarData<int32>>(unrelatedId) != nullptr);
  CHECK(dataStructure.getDataAs<ScalarData<int32>>(unrelatedId)->getValue() == 11);
  CHECK(dataStructure.getData(DataPath({"GroupA", "Target"}))->getId() == unrelatedId);
}

TEST_CASE("SimplnxCore::RenameDataAction other parent clash", "[SimplnxCore][RenameDataAction]")
{
  UnitTest::LoadPlugins();
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  const bool hasSibling = GENERATE(false, true);
  CAPTURE(mode, hasSibling);
  auto dataStructure = UnitTest::CreateComplexMultiLevelDataGraph();
  const DataPath sourcePath({Constants::k_GroupAName, Constants::k_GroupCName, Constants::k_GroupEName, Constants::k_ArrayJName});
  const DataPath clashPath({Constants::k_GroupAName, Constants::k_GroupCName, Constants::k_GroupDName, Constants::k_ArrayIName});
  auto& source = dataStructure.getDataRefAs<Float32Array>(sourcePath);
  auto& clash = dataStructure.getDataRefAs<UInt8Array>(clashPath);
  source[0] = 22.0f;
  clash[0] = 11;
  const auto sourceId = source.getId();
  const auto clashId = clash.getId();
  const auto parentIds = source.getParentIds();
  std::optional<DataObject::IdType> siblingId;
  if(hasSibling)
  {
    auto* sibling = ScalarData<int32>::Create(dataStructure, Constants::k_ArrayIName, 33, dataStructure.getData(sourcePath.getParent())->getId());
    REQUIRE(sibling != nullptr);
    siblingId = sibling->getId();
  }
  const auto objectIds = dataStructure.getAllDataObjectIds();
  RenameDataAction action(sourcePath, Constants::k_ArrayIName, true);
  auto result = action.apply(dataStructure, mode);
  CHECK(result.invalid());
  if(result.invalid())
  {
    REQUIRE(result.errors().size() == 1);
    CHECK(result.errors()[0].code == -6604);
    CHECK(result.errors()[0].message.find("Could not rename") != std::string::npos);
    CHECK(result.errors()[0].message.find(clashPath.toString()) != std::string::npos);
  }
  CHECK(dataStructure.getAllDataObjectIds() == objectIds);
  REQUIRE(dataStructure.getData(sourcePath) != nullptr);
  CHECK(dataStructure.getData(sourcePath)->getId() == sourceId);
  CHECK(source.getName() == Constants::k_ArrayJName);
  CHECK(source.getParentIds() == parentIds);
  CHECK(source[0] == 22.0f);
  REQUIRE(dataStructure.getData(clashPath) != nullptr);
  CHECK(dataStructure.getData(clashPath)->getId() == clashId);
  CHECK(clash[0] == 11);
  if(siblingId.has_value())
  {
    REQUIRE(dataStructure.getDataAs<ScalarData<int32>>(*siblingId) != nullptr);
    CHECK(dataStructure.getDataAs<ScalarData<int32>>(*siblingId)->getValue() == 33);
  }
}
