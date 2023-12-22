#include "MoveDataAction.hpp"

#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <fmt/core.h>

using namespace nx::core;

namespace
{
constexpr int32 k_MissingDataObjectCode = -6401;
constexpr int32 k_HasParentCode = -6402;
constexpr int32 k_MissingParentObjectCode = -6403;
constexpr int32 k_FailedAddParentCode = -6404;
constexpr int32 k_FailedRemoveParentCode = -6405;
constexpr int32 k_FailedFindOldParentCode = -6406;
constexpr int32 k_RecursiveParentCode = -6407;
} // namespace

namespace nx::core
{
MoveDataAction::MoveDataAction(const DataPath& path, const DataPath& newParentPath)
: m_NewParentPath(newParentPath)
, m_Path(path)
{
}

MoveDataAction::~MoveDataAction() noexcept = default;

Result<> MoveDataAction::apply(DataStructure& dataStructure, Mode mode) const
{
  static constexpr StringLiteral prefix = "MoveDataAction: ";
  auto dataObject = dataStructure.getData(m_Path);
  if(dataObject == nullptr)
  {
    return MakeErrorResult(k_MissingDataObjectCode, fmt::format("{}Could not find DataObject at '{}'", prefix, m_Path.toString()));
  }

  if(m_Path == m_NewParentPath)
  {
    return MakeErrorResult(k_RecursiveParentCode, fmt::format("{}Cannot move data object at path '{}' into itself.", prefix, m_Path.toString()));
  }

  auto parentIds = dataObject->getParentIds();
  for(const auto parentId : parentIds)
  {
    auto parentPaths = dataStructure.getDataPathsForId(parentId);
    for(const auto& parentPath : parentPaths)
    {
      if(m_NewParentPath == parentPath)
      {
        return MakeErrorResult(k_HasParentCode, fmt::format("{}Could not move data from '{}' to '{}' as it already exists under the new parent", prefix, m_Path.toString(), parentPath.toString()));
      }
    }
  }
  const auto* newParent = dataStructure.getDataAs<BaseGroup>(m_NewParentPath);
  if(newParent == nullptr)
  {
    return MakeErrorResult(k_MissingParentObjectCode, fmt::format("{}Could not find a possible parent object at '{}'", prefix, m_NewParentPath.toString()));
  }

  if(!dataStructure.setAdditionalParent(dataObject->getId(), newParent->getId()))
  {
    return MakeErrorResult(k_FailedAddParentCode, fmt::format("{}Could not add new parent at '{}' to DataObject at '{}' ", prefix, m_NewParentPath.toString(), m_Path.toString()));
  }

  const DataPath parentPath = m_Path.getParent();
  const auto* oldParent = dataStructure.getDataAs<BaseGroup>(parentPath);
  // If no parent exists
  if(parentPath.empty())
  {
    if(!dataStructure.removeParent(dataObject->getId(), 0))
    {
      return MakeErrorResult(k_FailedRemoveParentCode, fmt::format("{}Could not remove parent at '{}'", prefix, parentPath.toString()));
    }
  }
  else
  {
    if(oldParent == nullptr)
    {
      return MakeErrorResult(k_FailedFindOldParentCode, fmt::format("{}Could not find a possible parent object at '{}'", prefix, parentPath.toString()));
    }
    if(!dataStructure.removeParent(dataObject->getId(), oldParent->getId()))
    {
      return MakeErrorResult(k_FailedRemoveParentCode, fmt::format("{}Could not remove parent at '{}'", prefix, parentPath.toString()));
    }
  }
  return {};
}

IDataAction::UniquePointer MoveDataAction::clone() const
{
  return std::make_unique<MoveDataAction>(m_Path, m_NewParentPath);
}

DataPath MoveDataAction::newParentPath() const
{
  return m_NewParentPath;
}

const DataPath& MoveDataAction::path() const
{
  return m_Path;
}
} // namespace nx::core
