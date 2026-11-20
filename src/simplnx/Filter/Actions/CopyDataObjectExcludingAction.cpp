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
      if(const auto* copiedChild = copy[child->getName()]; copiedChild != nullptr)
      {
        [[maybe_unused]] const bool removed = dataStructure.removeParent(copiedChild->getId(), copy.getId());
        assert(removed);
      }
    }
    else if(const auto* sourceGroup = dynamic_cast<const BaseGroup*>(child.get()); sourceGroup != nullptr)
    {
      if(auto* copiedGroup = dynamic_cast<BaseGroup*>(copy[child->getName()]); copiedGroup != nullptr)
      {
        PruneCopiedLinks(dataStructure, *sourceGroup, *copiedGroup, excludedId);
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
    if(const auto* source = dataStructure.getDataAs<BaseGroup>(path()); source != nullptr)
    {
      auto& copy = dataStructure.getDataRefAs<BaseGroup>(newPath());
      PruneCopiedLinks(dataStructure, *source, copy, *excludedId);
    }
  }
  return result;
}

IDataAction::UniquePointer CopyDataObjectExcludingAction::clone() const
{
  return std::make_unique<CopyDataObjectExcludingAction>(path(), newPath(), getAllCreatedPaths(), m_ExcludedPath);
}
