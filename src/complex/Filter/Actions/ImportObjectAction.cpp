#include "ImportObjectAction.hpp"

#include <fmt/core.h>

#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/EmptyDataStore.hpp"

using namespace complex;

namespace
{
constexpr complex::int32 k_InsertFailureError = -2;
} // namespace

namespace complex
{
ImportObjectAction::ImportObjectAction(const std::shared_ptr<DataObject>& importObject, const DataPath& path)
: IDataCreationAction(path)
, m_ImportData(importObject)
{
}

ImportObjectAction::~ImportObjectAction() noexcept = default;

Result<> ImportObjectAction::apply(DataStructure& dataStructure, Mode mode) const
{
  auto importData = std::shared_ptr<DataObject>(getImportObject()->deepCopy());
  // Clear all children before inserting into the DataStructure
  if(auto importGroup = std::dynamic_pointer_cast<BaseGroup>(importData))
  {
    importGroup->clear();
  }

  if(!dataStructure.insert(importData, getCreatedPath().getParent()))
  {
    return {nonstd::make_unexpected(std::vector<Error>{{k_InsertFailureError, fmt::format("Unable to import DataObject at '{}'", getCreatedPath().toString())}})};
  }

  return {};
}

std::shared_ptr<DataObject> ImportObjectAction::getImportObject() const
{
  return m_ImportData;
}

DataPath ImportObjectAction::path() const
{
  return getCreatedPath();
}
} // namespace complex