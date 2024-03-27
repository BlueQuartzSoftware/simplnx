#include "RenameDataAction.hpp"

#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <fmt/core.h>

using namespace nx::core;

namespace nx::core
{
RenameDataAction::RenameDataAction(const DataPath& path, const std::string& newName, bool overwrite)
: m_NewName(newName)
, m_Path(path)
, m_Overwrite(overwrite)
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

  int validRename = dataObject->canRename(m_NewName);
  if(validRename == 0)
  {
    std::string ss = fmt::format("{}Could not rename DataObject at '{}' to '{}'", prefix, m_Path.toString(), m_NewName);
    return MakeErrorResult(-6602, ss);
  }
  if(validRename == 2)
  {
    if(m_Overwrite)
    {
      if(mode == Mode::Preflight)
      {
        std::string ss = fmt::format("{}Another object exists with that name, will overwrite destroying other DataObject at '{}' and replacing it with '{}'", prefix, m_NewName, m_Path.toString());
        Result<>{}.warnings().emplace_back(Warning{-6603,ss});
      }
    }
    else
    {
      std::string ss = fmt::format("{}Another object exists with that name, will not rename DataObject at '{}' to '{}'", prefix, m_Path.toString(), m_NewName);
      return MakeErrorResult(-6604, ss);
    }
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

bool RenameDataAction::overwrite() const
{
  return m_Overwrite;
}

} // namespace nx::core
