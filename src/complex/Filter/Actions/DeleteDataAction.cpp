#include "DeleteDataAction.hpp"

#include <fmt/core.h>

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/EmptyDataStore.hpp"

using namespace complex;

namespace
{
constexpr int32 k_TargetNotFoundErrorCode = -3245;

Result<> RemoveFromParent(DataStructure& dataStructure, const DataPath& path, IDataAction::Mode mode)
{
  DataObject* targetObject = dataStructure.getData(path);
  if(targetObject == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{{k_TargetNotFoundErrorCode, fmt::format("Cound not find \"{}\" within DataStructure", path.toString())}})};
  }

  const auto parentPath = path.getParent();
  DataObject::IdType parentId = 0;
  if(!parentPath.empty())
  {
    auto parentObject = dataStructure.getData(parentPath);
    parentId = parentObject->getId();
  }
  dataStructure.removeParent(targetObject->getId(), parentId);
}
}

namespace complex
{
DeleteDataAction::DeleteDataAction(const DataPath& path)
: m_Path(path)
{
}

DeleteDataAction::~DeleteDataAction() noexcept = default;

Result<> DeleteDataAction::apply(DataStructure& dataStructure, Mode mode) const
{
  return RemoveFromParent(dataStructure, path(), mode);
}

DataPath DeleteDataAction::path() const
{
  return m_Path;
}
} // namespace complex