#include "RenameDataAction.hpp"

#include <fmt/core.h>

#include "complex/Common/TypeTraits.hpp"
#include "complex/DataStructure/EmptyDataStore.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

using namespace complex;

namespace complex
{
RenameDataAction::RenameDataAction(const DataPath& path, const std::string& newName)
: m_NewName(newName)
, m_Path(path)
{
}

RenameDataAction::~RenameDataAction() noexcept = default;

Result<> RenameDataAction::apply(DataStructure& dataStructure, Mode mode) const
{
  auto dataObject = dataStructure.getData(m_Path);
  if(dataObject == nullptr)
  {
    std::string ss = fmt::format("Could not find DataObject at '{}'", m_Path.toString());
    return MakeErrorResult(-90, ss);
  }

  if(!dataObject->canRename(m_NewName))
  {
    std::string ss = fmt::format("Could not rename DataObject at '{}' to '{}'", m_Path.toString(), m_NewName);
    return MakeErrorResult(-91, ss);
  }

  dataObject->rename(m_NewName);
  return {};
}

std::string RenameDataAction::newName() const
{
  return m_NewName;
}

const DataPath& RenameDataAction::path() const
{
  return m_Path;
}
} // namespace complex
