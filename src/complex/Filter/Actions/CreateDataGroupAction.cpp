#include "CreateDataGroupAction.hpp"

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/LinkedPath.hpp"

#include <fmt/core.h>

namespace complex
{
//------------------------------------------------------------------------------
CreateDataGroupAction::CreateDataGroupAction(const DataPath& path)
: m_Path(path)
{
}

CreateDataGroupAction::~CreateDataGroupAction() noexcept = default;

DataPath CreateDataGroupAction::parentPath() const
{
  return m_Path;
}

Result<> CreateDataGroupAction::apply(DataStructure& dataStructure, Mode mode) const
{
  Result<LinkedPath> linkedPath = dataStructure.makePath(m_Path);
  if(!linkedPath.valid())
  {
    return MakeErrorResult(-2, fmt::format("Path '{}' was not created.", m_Path.toString()));
  }
  // Assuming nothing went wrong, return an empty Result<> object
  return {};
}
} // namespace complex
