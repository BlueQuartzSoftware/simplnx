#include "ImportObjectAction.hpp"

#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/DataStructure/DataArray.hpp"

#include <fmt/core.h>

using namespace complex;

namespace
{
constexpr complex::int32 k_InsertFailureError = -6301;
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
  static constexpr StringLiteral prefix = "ImportObjectAction: ";
  const auto importData = std::shared_ptr<DataObject>(getImportObject()->shallowCopy());
  // Clear all children before inserting into the DataStructure
  if(const auto importGroup = std::dynamic_pointer_cast<BaseGroup>(importData); importGroup != nullptr)
  {
    importGroup->clear();
  }

  if(!dataStructure.insert(importData, getCreatedPath().getParent()))
  {
    return {nonstd::make_unexpected(std::vector<Error>{{k_InsertFailureError, fmt::format("{}Unable to import DataObject at '{}'", prefix, getCreatedPath().toString())}})};
  }

  return {};
}

IDataAction::UniquePointer ImportObjectAction::clone() const
{
  return std::make_unique<ImportObjectAction>(m_ImportData, getCreatedPath());
}

std::shared_ptr<DataObject> ImportObjectAction::getImportObject() const
{
  return m_ImportData;
}

DataPath ImportObjectAction::path() const
{
  return getCreatedPath();
}

std::vector<DataPath> ImportObjectAction::getAllCreatedPaths() const
{
  return {getCreatedPath()};
}
} // namespace complex
