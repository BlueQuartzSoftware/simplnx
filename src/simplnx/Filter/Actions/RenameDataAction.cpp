#include "RenameDataAction.hpp"

#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <fmt/core.h>

#include <optional>

using namespace nx::core;

namespace
{
constexpr int32 k_OtherParentNameClash = -6605;
constexpr int32 k_RenameFailed = -6606;
constexpr int32 k_DetachTargetFailed = -6607;

Result<> TerminateNodesRecursively(DataStructure& dataStructure, DataObject::IdType id, IDataAction::Mode mode, const bool checkDependence)
{
  Result<> result = {};
  DataObject* node = dataStructure.getData(id);

  if(checkDependence)
  {
    auto parentIds = node->getParentIds();
    if(parentIds.size() > 1)
    {
      return {};
    }
  }

  auto* baseGroup = dataStructure.getDataAs<BaseGroup>(id);
  if(baseGroup != nullptr)
  {
    auto childIds = baseGroup->GetChildrenIds();
    if(!childIds.empty())
    {
      for(const auto& childId : childIds)
      {
        result = MergeResults(result, TerminateNodesRecursively(dataStructure, childId, mode, checkDependence));
      }
    }
  }

  dataStructure.removeData(node->getId());

  return result;
}
} // namespace

namespace nx::core
{
RenameDataAction::RenameDataAction(const DataPath& path, const std::string& newName, bool overwrite)
: m_NewName(newName)
, m_Path(path)
, m_Overwrite(overwrite)
{
}

RenameDataAction::~RenameDataAction() noexcept = default;

Result<> RenameDataAction::apply(DataStructure& dataStructure, Mode mode) const
{
  static constexpr StringLiteral prefix = "RenameDataAction: ";
  auto dataObject = dataStructure.getData(m_Path);
  if(dataObject == nullptr)
  {
    std::string ss = fmt::format("{}Source DataPath '{}' does not exist to rename to {}", prefix, m_Path.toString(), m_NewName);
    return MakeErrorResult(-6601, ss);
  }

  Result<> result = {};

  if(m_Overwrite)
  {
    if(dataObject->getName() == m_NewName)
    {
      return result;
    }

    std::vector<std::string> pathVec = m_Path.getPathVector();
    for(const auto& name : pathVec)
    {
      if(name == m_NewName)
      {
        std::string ss = fmt::format("{}The object that would be overwritten is a parent container to {} cannot rename to {}", prefix, m_Path.getTargetName(), m_NewName);
        return MakeErrorResult(-6601, ss);
      }
    }

    const auto* selectedParent = dataStructure.getData(m_Path.getParent());
    std::optional<DataPath> clashPath;
    const DataPath rootTargetPath({m_NewName});
    if(selectedParent != nullptr && dataStructure.getData(DataPath({dataObject->getName()})) == dataObject && dataStructure.getData(rootTargetPath) != nullptr)
    {
      clashPath = rootTargetPath;
    }
    for(const auto parentId : dataObject->getParentIds())
    {
      if(selectedParent != nullptr && parentId == selectedParent->getId())
      {
        continue;
      }
      const auto* parent = dataStructure.getDataAs<BaseGroup>(parentId);
      if(parent != nullptr && parent->contains(m_NewName))
      {
        clashPath = parent->getDataPaths().front().createChildPath(m_NewName);
        break;
      }
    }

    if(clashPath.has_value())
    {
      return MakeErrorResult(k_OtherParentNameClash, fmt::format("{}Could not rename DataObject at '{}' to '{}': another parent has a child at '{}'. Choose a different name or remove '{}' first.",
                                                                 prefix, m_Path.toString(), m_NewName, clashPath->toString(), clashPath->toString()));
    }

    auto* target = dataStructure.getData(m_Path.replaceName(m_NewName));
    if(target != nullptr)
    {
      const auto targetId = target->getId();
      if(mode == Mode::Preflight)
      {
        std::string ss = fmt::format("{}Another object exists with that name, will overwrite destroying other DataObject at '{}' and replacing it with '{}'", prefix, m_NewName, m_Path.toString());
        result.warnings().emplace_back(Warning{-6602, ss});
        return result;
      }
      // The root placement is not included in the object's parent IDs.
      const bool hasOtherParents = target->getParentIds().size() > (selectedParent == nullptr ? 0 : 1) || (selectedParent != nullptr && dataStructure.getData(DataPath({m_NewName})) == target);
      if(hasOtherParents)
      {
        if(!dataStructure.removeParent(targetId, selectedParent == nullptr ? 0 : selectedParent->getId()))
        {
          return MakeErrorResult(k_DetachTargetFailed,
                                 fmt::format("{}Could not detach overwrite target at '{}'. Choose a different name or remove the target first.", prefix, m_Path.replaceName(m_NewName).toString()));
        }
      }
      else if(dataStructure.getDataAs<BaseGroup>(targetId) != nullptr)
      {
        // Shared descendants remain in their other parents.
        result = MergeResults(result, ::TerminateNodesRecursively(dataStructure, targetId, mode, true));
      }
      else
      {
        dataStructure.removeData(targetId);
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
          std::string ss = fmt::format("{}The new name '{}' conflicts with existing array name. Change name or enable 'Allow Overwrite'.", prefix, m_NewName);
          return MakeErrorResult(-6603, ss);
        }
      }
    }

    if(!dataObject->canRename(m_NewName))
    {
      std::string ss = fmt::format("{}Could not rename DataObject at '{}' to '{}'", prefix, m_Path.toString(), m_NewName);
      return MakeErrorResult(-6604, ss);
    }
  }

  if(!dataObject->rename(m_NewName))
  {
    return MakeErrorResult(k_RenameFailed, fmt::format("{}Could not rename DataObject at '{}' to '{}'. Choose a valid name that is unique under every parent.", prefix, m_Path.toString(), m_NewName));
  }
  return result;
}

IDataAction::UniquePointer RenameDataAction::clone() const
{
  return std::make_unique<RenameDataAction>(m_Path, m_NewName);
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
