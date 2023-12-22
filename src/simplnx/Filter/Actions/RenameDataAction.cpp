#include "RenameDataAction.hpp"

#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <fmt/core.h>

using namespace nx::core;

namespace nx::core
{
RenameDataAction::RenameDataAction(const DataPath& path, const std::string& newName)
: m_NewName(newName)
, m_Path(path)
{
}

RenameDataAction::~RenameDataAction() noexcept = default;

Result<> RenameDataAction::apply(DataStructure& dataStructure, Mode mode) const
{
  static constexpr StringLiteral prefix = "RenameDataAction: ";
  auto dataObject = dataStructure.getData(m_Path);
  if(dataObject == nullptr)
  {
    std::string ss = fmt::format("{}Source DataPath '{}' does not exist to rename to {}", prefix, m_Path.toString(), m_NewName);
    return MakeErrorResult(-6601, ss);
  }

  if(!dataObject->canRename(m_NewName))
  {
    std::string ss = fmt::format("{}Could not rename DataObject at '{}' to '{}'", prefix, m_Path.toString(), m_NewName);
    return MakeErrorResult(-6602, ss);
  }

  dataObject->rename(m_NewName);
  return {};
}

IDataAction::UniquePointer RenameDataAction::clone() const
{
  return std::make_unique<RenameDataAction>(m_Path, m_NewName);
}

std::string RenameDataAction::newName() const
{
  return m_NewName;
}

const DataPath& RenameDataAction::path() const
{
  return m_Path;
}
} // namespace nx::core
