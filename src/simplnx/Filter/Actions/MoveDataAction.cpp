#include "MoveDataAction.hpp"

#include "simplnx/Common/StringLiteralFormatting.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"

#include <fmt/core.h>

#include <utility>

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
MoveDataAction::MoveDataAction(DataPath path, DataPath newParentPath)
: m_NewParentPath(std::move(newParentPath))
, m_Path(std::move(path))
{
}

MoveDataAction::~MoveDataAction() noexcept = default;

Result<> MoveDataAction::apply(DataStructure& dataStructure, Mode /*mode*/) const
{
  static constexpr StringLiteralType k_Prefix = "MoveDataAction: ";
  auto* dataObjectPtr = dataStructure.getData(m_Path);
  if(dataObjectPtr == nullptr)
  {
    return MakeErrorResult(k_MissingDataObjectCode, fmt::format("{}Could not find DataObject at '{}'", k_Prefix, m_Path.toString()));
  }

  if(m_Path == m_NewParentPath)
  {
    return MakeErrorResult(k_RecursiveParentCode, fmt::format("{}Cannot move data object at path '{}' into itself.", k_Prefix, m_Path.toString()));
  }

  auto parentIds = dataObjectPtr->getParentIds();
  for(const auto parentId : parentIds)
  {
    auto parentPaths = dataStructure.getDataPathsForId(parentId);
    for(const auto& parentPath : parentPaths)
    {
      if(m_NewParentPath == parentPath)
      {
        return MakeErrorResult(k_HasParentCode, fmt::format("{}Could not move data from '{}' to '{}' as it already exists under the new parent", k_Prefix, m_Path.toString(), parentPath.toString()));
      }
    }
  }
  const auto* newParentPtr = dataStructure.getDataAs<BaseGroup>(m_NewParentPath);
  if(newParentPtr == nullptr)
  {
    return MakeErrorResult(k_MissingParentObjectCode, fmt::format("{}Could not find a possible parent object at '{}'", k_Prefix, m_NewParentPath.toString()));
  }

  if(!dataStructure.setAdditionalParent(dataObjectPtr->getId(), newParentPtr->getId()))
  {
    return MakeErrorResult(k_FailedAddParentCode, fmt::format("{}Could not add new parent at '{}' to DataObject at '{}' ", k_Prefix, m_NewParentPath.toString(), m_Path.toString()));
  }

  const DataPath parentPath = m_Path.getParent();
  const auto* oldParentPtr = dataStructure.getDataAs<BaseGroup>(parentPath);
  // If no parent exists
  if(parentPath.empty())
  {
    if(!dataStructure.removeParent(dataObjectPtr->getId(), 0))
    {
      return MakeErrorResult(k_FailedRemoveParentCode, fmt::format("{}Could not remove parent at '{}'", k_Prefix, parentPath.toString()));
    }
  }
  else
  {
    if(oldParentPtr == nullptr)
    {
      return MakeErrorResult(k_FailedFindOldParentCode, fmt::format("{}Could not find a possible parent object at '{}'", k_Prefix, parentPath.toString()));
    }
    if(!dataStructure.removeParent(dataObjectPtr->getId(), oldParentPtr->getId()))
    {
      return MakeErrorResult(k_FailedRemoveParentCode, fmt::format("{}Could not remove parent at '{}'", k_Prefix, parentPath.toString()));
    }
  }
  return {};
}

IDataAction::UniquePointerType MoveDataAction::clone() const
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
