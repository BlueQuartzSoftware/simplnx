#include "CopyDataObjectExcludingAction.hpp"

#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"

#include <cassert>

using namespace nx::core;

namespace
{
void PruneCopiedLinks(DataStructure& dataStructure, const BaseGroup& source, BaseGroup& copy, DataObject::IdType excludedId)
{
  for(const auto& [id, child] : source)
  {
    if(child->getId() == excludedId)
    {
      if(const auto* copiedChildPtr = copy[child->getName()]; copiedChildPtr != nullptr)
      {
        [[maybe_unused]] const bool removed = dataStructure.removeParent(copiedChildPtr->getId(), copy.getId());
        assert(removed);
      }
    }
    else if(const auto* sourceGroupPtr = dynamic_cast<const BaseGroup*>(child.get()); sourceGroupPtr != nullptr)
    {
      if(auto* copiedGroupPtr = dynamic_cast<BaseGroup*>(copy[child->getName()]); copiedGroupPtr != nullptr)
      {
        PruneCopiedLinks(dataStructure, *sourceGroupPtr, *copiedGroupPtr, excludedId);
      }
    }
  }
}
} // namespace

CopyDataObjectExcludingAction::CopyDataObjectExcludingAction(const DataPath& source, const DataPath& destination, const std::vector<DataPath>& allCreatedPaths, DataPath excludedPath)
: CopyDataObjectAction(source, destination, allCreatedPaths)
, m_ExcludedPath(std::move(excludedPath))
{
}

CopyDataObjectExcludingAction::~CopyDataObjectExcludingAction() noexcept = default;

Result<> CopyDataObjectExcludingAction::apply(DataStructure& dataStructure, Mode mode) const
{
  const auto excludedId = m_ExcludedPath.empty() ? std::nullopt : dataStructure.getId(m_ExcludedPath);
  auto result = CopyDataObjectAction::apply(dataStructure, mode);
  if(result.valid() && excludedId.has_value())
  {
    if(const auto* sourcePtr = dataStructure.getDataAs<BaseGroup>(path()); sourcePtr != nullptr)
    {
      auto& copy = dataStructure.getDataRefAs<BaseGroup>(newPath());
      PruneCopiedLinks(dataStructure, *sourcePtr, copy, *excludedId);
    }
  }
  return result;
}

IDataAction::UniquePointerType CopyDataObjectExcludingAction::clone() const
{
  return std::make_unique<CopyDataObjectExcludingAction>(path(), newPath(), getAllCreatedPaths(), m_ExcludedPath);
}
