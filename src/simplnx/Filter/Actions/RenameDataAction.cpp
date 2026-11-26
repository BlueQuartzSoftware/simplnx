#include "RenameDataAction.hpp"

#include "simplnx/Common/StringLiteralFormatting.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"

#include <fmt/core.h>

#include <optional>
#include <utility>

using namespace nx::core;

namespace
{
constexpr int32 k_OtherParentNameClash = -6605;
constexpr int32 k_RenameFailed = -6606;
constexpr int32 k_DetachTargetFailed = -6607;
constexpr int32 k_InvalidNewName = -6608;

/**
 * @brief Removes one placement and deletes descendants that have no remaining placement.
 * @param dataStructure The graph to modify.
 * @param id Identifies the object whose placement is removed.
 * @param parentId Identifies the selected parent, or zero for the root.
 * @param selectedPath The original selected path for failure diagnostics.
 * @return The first failure to remove a placement, or success.
 */
Result<> RemovePlacementRecursively(DataStructure& dataStructure, DataObject::IdType id, DataObject::IdType parentId, const DataPath& selectedPath)
{
  // Keep the detached parent alive while its child links are removed.
  const auto node = dataStructure.getSharedData(id);
  if(node == nullptr || !dataStructure.removeParent(id, parentId))
  {
    return MakeErrorResult(k_DetachTargetFailed, fmt::format("Could not remove the overwrite placement at '{}'. Choose a different name or remove the target first.", selectedPath.toString()));
  }

  // Root placements are not included in the object's parent IDs.
  if(!node->getParentIds().empty() || dataStructure.getData(DataPath({node->getName()})) == node.get())
  {
    return {};
  }

  auto* groupPtr = dynamic_cast<BaseGroup*>(node.get());
  if(groupPtr != nullptr)
  {
    const auto childIds = groupPtr->GetChildrenIds();
    for(const auto childId : childIds)
    {
      const auto* childPtr = dataStructure.getData(childId);
      auto result = RemovePlacementRecursively(dataStructure, childId, id, selectedPath.createChildPath(childPtr->getName()));
      if(result.invalid())
      {
        return result;
      }
    }
  }
  return {};
}
} // namespace

namespace nx::core
{
RenameDataAction::RenameDataAction(DataPath path, std::string newName, bool overwrite)
: m_NewName(std::move(newName))
, m_Path(std::move(path))
, m_Overwrite(overwrite)
{
}

RenameDataAction::~RenameDataAction() noexcept = default;

Result<> RenameDataAction::apply(DataStructure& dataStructure, Mode mode) const
{
  static constexpr StringLiteral k_Prefix = "RenameDataAction: ";
  if(!DataObject::IsValidName(m_NewName))
  {
    return MakeErrorResult(k_InvalidNewName, fmt::format("{}New Name '{}' is invalid for source '{}'. Enter a non-empty name without '/'.", k_Prefix, m_NewName, m_Path.toString()));
  }
  auto* dataObjectPtr = dataStructure.getData(m_Path);
  if(dataObjectPtr == nullptr)
  {
    std::string ss = fmt::format("{}Source DataPath '{}' does not exist to rename to {}", k_Prefix, m_Path.toString(), m_NewName);
    return MakeErrorResult(-6601, ss);
  }

  Result<> result = {};

  if(m_Overwrite)
  {
    if(dataObjectPtr->getName() == m_NewName)
    {
      return result;
    }

    std::vector<std::string> pathVec = m_Path.getPathVector();
    for(const auto& name : pathVec)
    {
      if(name == m_NewName)
      {
        std::string ss = fmt::format("{}The object that would be overwritten is a parent container to {} cannot rename to {}", k_Prefix, m_Path.getTargetName(), m_NewName);
        return MakeErrorResult(-6601, ss);
      }
    }

    const auto* selectedParentPtr = dataStructure.getData(m_Path.getParent());
    std::optional<DataPath> clashPath;
    const DataPath rootTargetPath({m_NewName});
    if(selectedParentPtr != nullptr && dataStructure.getData(DataPath({dataObjectPtr->getName()})) == dataObjectPtr && dataStructure.getData(rootTargetPath) != nullptr)
    {
      clashPath = rootTargetPath;
    }
    for(const auto parentId : dataObjectPtr->getParentIds())
    {
      if(selectedParentPtr != nullptr && parentId == selectedParentPtr->getId())
      {
        continue;
      }
      const auto* parentPtr = dataStructure.getDataAs<BaseGroup>(parentId);
      if(parentPtr != nullptr && parentPtr->contains(m_NewName))
      {
        clashPath = parentPtr->getDataPaths().front().createChildPath(m_NewName);
        break;
      }
    }

    if(clashPath.has_value())
    {
      return MakeErrorResult(k_OtherParentNameClash, fmt::format("{}Could not rename DataObject at '{}' to '{}': another parent has a child at '{}'. Choose a different name or remove '{}' first.",
                                                                 k_Prefix, m_Path.toString(), m_NewName, clashPath->toString(), clashPath->toString()));
    }

    auto* targetPtr = dataStructure.getData(m_Path.replaceName(m_NewName));
    if(targetPtr != nullptr)
    {
      const auto targetId = targetPtr->getId();
      if(mode == Mode::Preflight)
      {
        std::string ss = fmt::format("{}The object at '{}' will be replaced under the selected parent. Other placements and their children are kept. Objects left with no placement are deleted.",
                                     k_Prefix, m_Path.replaceName(m_NewName).toString());
        result.warnings().emplace_back(Warning{.code = -6602, .message = ss});
        return result;
      }
      auto removalResult = RemovePlacementRecursively(dataStructure, targetId, selectedParentPtr == nullptr ? 0 : selectedParentPtr->getId(), m_Path.replaceName(m_NewName));
      if(removalResult.invalid())
      {
        return removalResult;
      }
    }
  }
  else
  {
    DataPath parent = m_Path.getParent();
    auto* parentContainerPtr = dataStructure.getDataAs<BaseGroup>(m_Path.getParent());
    if(parentContainerPtr != nullptr)
    {
      std::vector<std::string> siblingNames = parentContainerPtr->GetChildrenNames();
      for(const auto& siblingName : siblingNames)
      {
        if(m_NewName == siblingName)
        {
          std::string ss = fmt::format("{}The new name '{}' conflicts with existing array name. Change name or enable 'Allow Overwrite'.", k_Prefix, m_NewName);
          return MakeErrorResult(-6603, ss);
        }
      }
    }

    if(!dataObjectPtr->canRename(m_NewName))
    {
      std::string ss = fmt::format("{}Could not rename DataObject at '{}' to '{}'", k_Prefix, m_Path.toString(), m_NewName);
      return MakeErrorResult(-6604, ss);
    }
  }

  if(!dataObjectPtr->rename(m_NewName))
  {
    return MakeErrorResult(k_RenameFailed, fmt::format("{}Could not rename DataObject at '{}' to '{}'. Choose a valid name that is unique under every parent.", k_Prefix, m_Path.toString(), m_NewName));
  }
  return result;
}

IDataAction::UniquePointer RenameDataAction::clone() const
{
  return std::make_unique<RenameDataAction>(m_Path, m_NewName, m_Overwrite);
}

std::string RenameDataAction::newName() const
{
  return m_NewName;
}

const DataPath& RenameDataAction::path() const
{
  return m_Path;
}

bool RenameDataAction::overwrite() const
{
  return m_Overwrite;
}

} // namespace nx::core
