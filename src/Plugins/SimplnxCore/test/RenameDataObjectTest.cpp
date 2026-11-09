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
#include <utility>
#include <vector>

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
  CHECK(actionResult.warnings()[0].message.find(k_DataPath.replaceName(k_NewName).toString()) != std::string::npos);
  CHECK(actionResult.warnings()[0].message.find("Other placements and their children are kept.") != std::string::npos);
  CHECK(actionResult.warnings()[0].message.find("Objects left with no placement are deleted.") != std::string::npos);
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
  REQUIRE(dataStructure.getData(DataPath({"GroupA", "Target"})) != nullptr);
  CHECK(dataStructure.getData(DataPath({"GroupA", "Target"}))->getId() == unrelatedId);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RenameDataAction other parent clash", "[SimplnxCore][RenameDataAction]")
{
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  const bool hasSibling = GENERATE(false, true);
  CAPTURE(mode, hasSibling);
  auto dataStructure = UnitTest::CreateComplexMultiLevelDataGraph();
  const DataPath sourcePath({Constants::k_GroupAName, Constants::k_GroupCName, Constants::k_GroupEName, Constants::k_ArrayJName});
  const DataPath clashPath({Constants::k_GroupAName, Constants::k_GroupCName, Constants::k_GroupDName, Constants::k_ArrayIName});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(sourcePath));
  auto& source = dataStructure.getDataRefAs<Float32Array>(sourcePath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(clashPath));
  auto& clash = dataStructure.getDataRefAs<UInt8Array>(clashPath);
  source[0] = 22.0f;
  clash[0] = 11;
  const auto sourceId = source.getId();
  const auto clashId = clash.getId();
  const auto parentIds = source.getParentIds();
  std::optional<DataObject::IdType> siblingId;
  if(hasSibling)
  {
    const auto* parent = dataStructure.getData(sourcePath.getParent());
    REQUIRE(parent != nullptr);
    auto* sibling = ScalarData<int32>::Create(dataStructure, Constants::k_ArrayIName, 33, parent->getId());
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
    CHECK(result.errors()[0].code == -6605);
    CHECK(result.errors()[0].message.find("Choose a different name or remove") != std::string::npos);
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
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RenameDataAction preserves overwrite target in another parent", "[SimplnxCore][RenameDataAction]")
{
  const bool groupTarget = GENERATE(false, true);
  const usize placement = GENERATE(usize{0}, usize{1}, usize{2});
  const bool topLevel = placement == 1;
  const bool sharedAtRoot = placement == 2;
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  CAPTURE(groupTarget, placement, mode);
  DataStructure dataStructure;
  auto* selectedParent = DataGroup::Create(dataStructure, "Selected");
  auto* otherParent = DataGroup::Create(dataStructure, "Other");
  REQUIRE(selectedParent != nullptr);
  REQUIRE(otherParent != nullptr);
  const std::optional<DataObject::IdType> parentId = topLevel ? std::nullopt : std::make_optional(selectedParent->getId());
  auto* source = ScalarData<int32>::Create(dataStructure, "Source", 22, parentId);
  REQUIRE(source != nullptr);
  const auto sourceId = source->getId();
  const std::optional<DataObject::IdType> targetParentId = sharedAtRoot ? std::nullopt : parentId;
  DataObject* target = nullptr;
  std::optional<DataObject::IdType> childId;
  if(groupTarget)
  {
    target = DataGroup::Create(dataStructure, "Target", targetParentId);
    REQUIRE(target != nullptr);
    auto* child = ScalarData<int32>::Create(dataStructure, "Child", 37, target->getId());
    REQUIRE(child != nullptr);
    childId = child->getId();
  }
  else
  {
    target = ScalarData<int32>::Create(dataStructure, "Target", 33, targetParentId);
    REQUIRE(target != nullptr);
  }
  const auto targetId = target->getId();
  REQUIRE(dataStructure.setAdditionalParent(targetId, sharedAtRoot ? selectedParent->getId() : otherParent->getId()));
  const DataPath sourcePath = topLevel ? DataPath({"Source"}) : DataPath({"Selected", "Source"});
  const DataPath targetPath = sourcePath.replaceName("Target");
  const DataPath otherPath = sharedAtRoot ? DataPath({"Target"}) : DataPath({"Other", "Target"});
  RenameDataAction action(sourcePath, "Target", true);
  auto result = action.apply(dataStructure, mode);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE(dataStructure.getData(otherPath) != nullptr);
  CHECK(dataStructure.getData(otherPath)->getId() == targetId);
  REQUIRE(dataStructure.getData(targetPath) != nullptr);
  if(mode == IDataAction::Mode::Preflight)
  {
    REQUIRE(result.warnings().size() == 1);
    CHECK(result.warnings()[0].code == -6602);
    CHECK(dataStructure.getData(targetPath)->getId() == targetId);
    REQUIRE(dataStructure.getData(sourcePath) != nullptr);
    CHECK(dataStructure.getData(sourcePath)->getId() == sourceId);
    CHECK(target->getParentIds().size() == (topLevel || sharedAtRoot ? 1 : 2));
  }
  else
  {
    CHECK(dataStructure.getData(targetPath)->getId() == sourceId);
    CHECK(dataStructure.getData(sourcePath) == nullptr);
    CHECK(source->getValue() == 22);
    if(sharedAtRoot)
    {
      CHECK(target->getParentIds().empty());
    }
    else
    {
      REQUIRE(target->getParentIds().size() == 1);
      CHECK(target->getParentIds().front() == otherParent->getId());
    }
  }
  if(childId.has_value())
  {
    const auto* child = dataStructure.getDataAs<ScalarData<int32>>(*childId);
    REQUIRE(child != nullptr);
    CHECK(child->getValue() == 37);
    REQUIRE(dataStructure.getData(otherPath.createChildPath("Child")) != nullptr);
    CHECK(dataStructure.getData(otherPath.createChildPath("Child"))->getId() == *childId);
  }
  else
  {
    const auto* scalar = dataStructure.getDataAs<ScalarData<int32>>(targetId);
    REQUIRE(scalar != nullptr);
    CHECK(scalar->getValue() == 33);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RenameDataObjectFilter: Invalid nested names preserve every placement", "[SimplnxCore][RenameDataObjectFilter]")
{
  const bool sourceAtRoot = GENERATE(false, true);
  const bool overwrite = GENERATE(false, true);
  const std::string newName = GENERATE(std::string(""), std::string("Bad/Name"));
  const int32 invocation = GENERATE(0, 1, 2, 3);
  CAPTURE(sourceAtRoot, overwrite, newName, invocation);
  DataStructure dataStructure;
  auto* parent = DataGroup::Create(dataStructure, "P");
  REQUIRE(parent != nullptr);
  const std::optional<DataObject::IdType> sourceParentId = sourceAtRoot ? std::nullopt : std::make_optional(parent->getId());
  auto* source = ScalarData<int32>::Create(dataStructure, "Source", 22, sourceParentId);
  auto* other = ScalarData<int32>::Create(dataStructure, "Other", 11, parent->getId());
  REQUIRE(source != nullptr);
  REQUIRE(other != nullptr);
  if(sourceAtRoot)
  {
    REQUIRE(dataStructure.setAdditionalParent(source->getId(), parent->getId()));
  }
  const auto sourceId = source->getId();
  const auto otherId = other->getId();
  const DataPath sourcePath({"P", "Source"});
  const auto objectIds = dataStructure.getAllDataObjectIds();
  const auto paths = dataStructure.getAllDataPaths();
  const auto rootObjects = dataStructure.getTopLevelData();
  std::vector<std::pair<DataObject::IdType, DataObject::ParentCollectionType>> parentLists;
  std::vector<std::pair<DataObject::IdType, const DataObject*>> objects;
  for(const auto id : objectIds)
  {
    const auto* object = dataStructure.getData(id);
    REQUIRE(object != nullptr);
    parentLists.emplace_back(id, object->getParentIds());
    objects.emplace_back(id, object);
  }

  std::vector<Error> errors;
  if(invocation < 2)
  {
    RenameDataObjectFilter filter;
    Arguments args;
    args.insert(RenameDataObjectFilter::k_AllowOverwrite_Key, std::make_any<bool>(overwrite));
    args.insert(RenameDataObjectFilter::k_NewName_Key, std::make_any<std::string>(newName));
    args.insert(RenameDataObjectFilter::k_SourceDataObjectPath_Key, std::make_any<DataPath>(sourcePath));
    if(invocation == 0)
    {
      const auto result = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
      errors = result.outputActions.errors();
    }
    else
    {
      const auto result = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
      errors = result.result.errors();
    }
  }
  else
  {
    RenameDataAction action(sourcePath, newName, overwrite);
    const auto mode = invocation == 2 ? IDataAction::Mode::Preflight : IDataAction::Mode::Execute;
    const auto result = action.apply(dataStructure, mode);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    errors = result.errors();
  }
  REQUIRE(errors.size() == 1);
  CHECK(errors[0].code == (invocation < 2 ? -27350 : -6608));
  CHECK(errors[0].message.find("New Name '" + newName + "'") != std::string::npos);
  CHECK(errors[0].message.find(sourcePath.toString()) != std::string::npos);
  CHECK(errors[0].message.find("Enter a non-empty name without '/'.") != std::string::npos);
  CHECK(dataStructure.getAllDataObjectIds() == objectIds);
  CHECK(dataStructure.getAllDataPaths() == paths);
  CHECK(dataStructure.getTopLevelData() == rootObjects);
  for(const auto& [id, object] : objects)
  {
    REQUIRE(dataStructure.getData(id) == object);
  }
  for(const auto& [id, parents] : parentLists)
  {
    REQUIRE(dataStructure.getData(id) != nullptr);
    CHECK(dataStructure.getData(id)->getParentIds() == parents);
  }
  REQUIRE(dataStructure.getDataAs<ScalarData<int32>>(sourceId) != nullptr);
  REQUIRE(dataStructure.getDataAs<ScalarData<int32>>(otherId) != nullptr);
  CHECK(dataStructure.getDataAs<ScalarData<int32>>(sourceId)->getName() == "Source");
  CHECK(dataStructure.getDataAs<ScalarData<int32>>(sourceId)->getValue() == 22);
  CHECK(dataStructure.getDataAs<ScalarData<int32>>(otherId)->getValue() == 11);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RenameDataAction rejects a clash at the source root placement", "[SimplnxCore][RenameDataAction]")
{
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  const bool hasSibling = GENERATE(false, true);
  CAPTURE(mode, hasSibling);
  DataStructure dataStructure;
  auto* selectedParent = DataGroup::Create(dataStructure, "Selected");
  REQUIRE(selectedParent != nullptr);
  auto* source = ScalarData<int32>::Create(dataStructure, "Source", 22);
  auto* clash = ScalarData<int32>::Create(dataStructure, "Target", 11);
  REQUIRE(source != nullptr);
  REQUIRE(clash != nullptr);
  REQUIRE(dataStructure.setAdditionalParent(source->getId(), selectedParent->getId()));
  const auto sourceId = source->getId();
  const auto clashId = clash->getId();
  std::optional<DataObject::IdType> siblingId;
  if(hasSibling)
  {
    auto* sibling = ScalarData<int32>::Create(dataStructure, "Target", 33, selectedParent->getId());
    REQUIRE(sibling != nullptr);
    siblingId = sibling->getId();
  }
  const auto objectIds = dataStructure.getAllDataObjectIds();
  RenameDataAction action(DataPath({"Selected", "Source"}), "Target", true);
  const auto result = action.apply(dataStructure, mode);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors()[0].code == -6605);
  CHECK(result.errors()[0].message.find("child at 'Target'") != std::string::npos);
  CHECK(dataStructure.getAllDataObjectIds() == objectIds);
  CHECK(dataStructure.getData(DataPath({"Source"})) == source);
  CHECK(dataStructure.getData(DataPath({"Selected", "Source"})) == source);
  CHECK(dataStructure.getData(DataPath({"Target"})) == clash);
  CHECK(source->getId() == sourceId);
  CHECK(clash->getId() == clashId);
  CHECK(source->getName() == "Source");
  CHECK(source->getValue() == 22);
  CHECK(clash->getValue() == 11);
  if(siblingId.has_value())
  {
    const auto* sibling = dataStructure.getDataAs<ScalarData<int32>>(*siblingId);
    REQUIRE(sibling != nullptr);
    CHECK(sibling->getValue() == 33);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RenameDataObjectFilter: Overwrite preserves retained descendant subtrees", "[SimplnxCore][RenameDataObjectFilter]")
{
  const int32 retainedPlacement = GENERATE(0, 1, 2);
  CAPTURE(retainedPlacement);
  DataStructure dataStructure;
  auto* parent = DataGroup::Create(dataStructure, "P");
  auto* otherParent = DataGroup::Create(dataStructure, "Other");
  REQUIRE(parent != nullptr);
  REQUIRE(otherParent != nullptr);
  auto* source = ScalarData<int32>::Create(dataStructure, "Source", 22, parent->getId());
  auto* target = DataGroup::Create(dataStructure, "Target", parent->getId());
  REQUIRE(source != nullptr);
  REQUIRE(target != nullptr);
  const auto sourceId = source->getId();
  const auto targetId = target->getId();
  auto* deleted = DataGroup::Create(dataStructure, "Deleted", targetId);
  REQUIRE(deleted != nullptr);
  auto* gone = ScalarData<int32>::Create(dataStructure, "Gone", 11, deleted->getId());
  REQUIRE(gone != nullptr);
  const auto deletedId = deleted->getId();
  const auto goneId = gone->getId();
  const std::optional<DataObject::IdType> sharedParentId = retainedPlacement == 0 ? std::nullopt : std::make_optional(retainedPlacement == 1 ? otherParent->getId() : targetId);
  auto* shared = DataGroup::Create(dataStructure, "Shared", sharedParentId);
  REQUIRE(shared != nullptr);
  const auto sharedId = shared->getId();
  auto* keep = ScalarData<int32>::Create(dataStructure, "Keep", 37, sharedId);
  REQUIRE(keep != nullptr);
  const auto keepId = keep->getId();
  if(retainedPlacement != 2)
  {
    REQUIRE(dataStructure.setAdditionalParent(sharedId, targetId));
  }

  RenameDataObjectFilter filter;
  Arguments args;
  args.insert(RenameDataObjectFilter::k_AllowOverwrite_Key, std::make_any<bool>(true));
  args.insert(RenameDataObjectFilter::k_NewName_Key, std::make_any<std::string>("Target"));
  args.insert(RenameDataObjectFilter::k_SourceDataObjectPath_Key, std::make_any<DataPath>(DataPath({"P", "Source"})));
  const auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE(dataStructure.getData(DataPath({"P", "Target"})) != nullptr);
  CHECK(dataStructure.getData(DataPath({"P", "Target"}))->getId() == sourceId);
  CHECK(dataStructure.getData(DataPath({"P", "Source"})) == nullptr);
  CHECK(dataStructure.getData(targetId) == nullptr);
  CHECK(dataStructure.getData(deletedId) == nullptr);
  CHECK(dataStructure.getData(goneId) == nullptr);
  CHECK(dataStructure.getData(DataPath({"P", "Target", "Deleted"})) == nullptr);
  CHECK(dataStructure.getData(DataPath({"P", "Target", "Shared"})) == nullptr);
  if(retainedPlacement == 2)
  {
    CHECK(dataStructure.getData(sharedId) == nullptr);
    CHECK(dataStructure.getData(keepId) == nullptr);
  }
  else
  {
    const DataPath retainedPath = retainedPlacement == 0 ? DataPath({"Shared"}) : DataPath({"Other", "Shared"});
    REQUIRE(dataStructure.getData(retainedPath) != nullptr);
    CHECK(dataStructure.getData(retainedPath)->getId() == sharedId);
    REQUIRE(dataStructure.getData(retainedPath.createChildPath("Keep")) != nullptr);
    CHECK(dataStructure.getData(retainedPath.createChildPath("Keep"))->getId() == keepId);
    const auto* retained = dataStructure.getData(sharedId);
    REQUIRE(retained != nullptr);
    if(retainedPlacement == 0)
    {
      CHECK(retained->getParentIds().empty());
    }
    else
    {
      REQUIRE(retained->getParentIds().size() == 1);
      CHECK(retained->getParentIds().front() == otherParent->getId());
    }
    const auto* retainedKeep = dataStructure.getDataAs<ScalarData<int32>>(keepId);
    REQUIRE(retainedKeep != nullptr);
    CHECK(retainedKeep->getValue() == 37);
    REQUIRE(retainedKeep->getParentIds().size() == 1);
    CHECK(retainedKeep->getParentIds().front() == sharedId);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
