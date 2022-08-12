#include "MoveDataAction.hpp"

#include <fmt/core.h>

#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

using namespace complex;

namespace
{
constexpr int32 k_MissingDataObjectCode = -40;
constexpr int32 k_HasParentCode = -41;
constexpr int32 k_MissingParentObjectCode = -42;
constexpr int32 k_FailedAddParentCode = -43;
constexpr int32 k_FailedRemoveParentCode = -44;
constexpr int32 k_FailedFindOldParentCode = -45;
} // namespace

namespace complex
{
MoveDataAction::MoveDataAction(const DataPath& path, const DataPath& newParentPath)
: m_NewParentPath(newParentPath)
, m_Path(path)
{
}

MoveDataAction::~MoveDataAction() noexcept = default;

Result<> MoveDataAction::apply(DataStructure& dataStructure, Mode mode) const
{
  auto dataObject = dataStructure.getData(m_Path);
  if(dataObject == nullptr)
  {
    std::string ss = fmt::format("Could not find DataObject at '{}'", m_Path.toString());
    return MakeErrorResult(k_MissingDataObjectCode, ss);
  }

  auto parentIds = dataObject->getParentIds();
  for(const auto parentId : parentIds)
  {
    auto parentPaths = dataStructure.getDataPathsForId(parentId);
    for(const auto parentPath : parentPaths)
    {
      if(m_NewParentPath == parentPath)
      {
        std::string ss = fmt::format("Could not move data from '{}' to '{}' as it already exists under the new parent", m_Path.toString(), parentPath.toString());
        return MakeErrorResult(k_HasParentCode, ss);
      }
    }
  }
  const auto* newParent = dataStructure.getDataAs<BaseGroup>(m_NewParentPath);
  if(newParent == nullptr)
  {
    std::string ss = fmt::format("Could not find a possible parent object at '{}'", m_NewParentPath.toString());
    return MakeErrorResult(k_MissingParentObjectCode, ss);
  }

  if(!dataStructure.setAdditionalParent(dataObject->getId(), newParent->getId()))
  {
    std::string ss = fmt::format("Could not add new parent at '{}' to DataObject at '{}' ", m_NewParentPath.toString(), m_Path.toString());
    return MakeErrorResult(k_FailedAddParentCode, ss);
  }

  const DataPath parentPath = m_Path.getParent();
  const auto* oldParent = dataStructure.getDataAs<BaseGroup>(parentPath);
  // If no parent exists
  if(parentPath.empty())
  {
    if(!dataStructure.removeParent(dataObject->getId(), 0))
    {
      std::string ss = fmt::format("Could not remove parent at '{}'", parentPath.toString());
      return MakeErrorResult(k_FailedRemoveParentCode, ss);
    }
  }
  else
  {
    if(oldParent == nullptr)
    {
      std::string ss = fmt::format("Could not find a possible parent object at '{}'", parentPath.toString());
      return MakeErrorResult(k_FailedFindOldParentCode, ss);
    }
    if(!dataStructure.removeParent(dataObject->getId(), oldParent->getId()))
    {
      std::string ss = fmt::format("Could not remove parent at '{}'", parentPath.toString());
      return MakeErrorResult(k_FailedRemoveParentCode, ss);
    }
  }
  return {};
}

DataPath MoveDataAction::newParentPath() const
{
  return m_NewParentPath;
}

const DataPath& MoveDataAction::path() const
{
  return m_Path;
}
} // namespace complex
