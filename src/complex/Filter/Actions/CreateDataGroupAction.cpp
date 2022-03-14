#include "CreateDataGroupAction.hpp"

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/LinkedPath.hpp"

#include <fmt/core.h>

namespace complex
{
//------------------------------------------------------------------------------
CreateDataGroupAction::CreateDataGroupAction(const DataPath& path)
: IDataCreationAction(path)
{
}

CreateDataGroupAction::~CreateDataGroupAction() noexcept = default;

Result<> CreateDataGroupAction::apply(DataStructure& dataStructure, Mode mode) const
{
  Result<LinkedPath> linkedPath = dataStructure.makePath(getCreatedPath());
  if(!linkedPath.valid())
  {
    return MakeErrorResult(-2, fmt::format("Path '{}' was not created.", getCreatedPath().toString()));
  }
  // Assuming nothing went wrong, return an empty Result<> object
  return {};
}
} // namespace complex
