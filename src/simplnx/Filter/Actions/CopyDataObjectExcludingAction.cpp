#include "CopyDataObjectExcludingAction.hpp"

#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"

using namespace nx::core;

CopyDataObjectExcludingAction::CopyDataObjectExcludingAction(const DataPath& source, const DataPath& destination, const std::vector<DataPath>& allCreatedPaths, DataPath excludedPath)
: CopyDataObjectAction(source, destination, allCreatedPaths)
, m_ExcludedPath(std::move(excludedPath))
{
}

CopyDataObjectExcludingAction::~CopyDataObjectExcludingAction() noexcept = default;

Result<> CopyDataObjectExcludingAction::apply(DataStructure& dataStructure, Mode mode) const
{
  const auto excluded = dataStructure.getSharedData(m_ExcludedPath);
  if(excluded == nullptr)
  {
    // Nothing links to a missing object, so a plain copy excludes it.
    return CopyDataObjectAction::apply(dataStructure, mode);
  }
  std::vector<std::pair<BaseGroup*, DataMap>> parentMaps;
  for(const auto parentId : excluded->getParentIds())
  {
    auto* parent = dataStructure.getDataAs<BaseGroup>(parentId);
    parentMaps.emplace_back(parent, parent->getDataMap());
  }
  // Keep parent IDs intact. Only this copy reads the temporary view.
  const auto restore = MakeScopeGuard([&parentMaps]() noexcept {
    for(auto& [parent, dataMap] : parentMaps)
    {
      parent->getDataMap() = std::move(dataMap);
    }
  });
  for(auto& [parent, dataMap] : parentMaps)
  {
    parent->getDataMap().remove(excluded->getId());
  }
  return CopyDataObjectAction::apply(dataStructure, mode);
}

IDataAction::UniquePointer CopyDataObjectExcludingAction::clone() const
{
  return std::make_unique<CopyDataObjectExcludingAction>(path(), newPath(), getAllCreatedPaths(), m_ExcludedPath);
}
