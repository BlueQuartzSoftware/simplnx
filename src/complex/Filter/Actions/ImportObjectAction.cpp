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
: m_ImportData(importObject)
, m_Path(path)
{
}

ImportObjectAction::~ImportObjectAction() noexcept = default;

Result<> ImportObjectAction::apply(DataStructure& dataStructure, Mode mode) const
{
  auto importData = std::shared_ptr<DataObject>(m_ImportData->deepCopy());
  // Clear all children before inserting into the DataStructure
  if(auto importGroup = std::dynamic_pointer_cast<BaseGroup>(importData))
  {
    importGroup->clear();
  }

  if(!dataStructure.insert(importData, m_Path.getParent()))
  {
    return {nonstd::make_unexpected(std::vector<Error>{{k_InsertFailureError, fmt::format("Unable to import DataObject at \"{}\"", m_Path.toString())}})};
  }

  return {};
}

std::shared_ptr<DataObject> ImportObjectAction::getImportObject() const
{
  return m_ImportData;
}

DataPath ImportObjectAction::path() const
{
  return m_Path;
}
} // namespace complex