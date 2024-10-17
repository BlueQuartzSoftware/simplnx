#include "CreateDataGroupAction.hpp"

#include "simplnx/Common/StringLiteralFormatting.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/LinkedPath.hpp"

#include <fmt/core.h>

namespace nx::core
{
//------------------------------------------------------------------------------
CreateDataGroupAction::CreateDataGroupAction(const DataPath& path)
: IDataCreationAction(path)
{
}

CreateDataGroupAction::~CreateDataGroupAction() noexcept = default;

Result<> CreateDataGroupAction::apply(DataStructure& dataStructure, Mode mode) const
{
  static constexpr StringLiteral prefix = "CreateDataGroupAction: ";
  Result<LinkedPath> linkedPath = dataStructure.makePath(getCreatedPath());
  if(!linkedPath.valid())
  {
    return MakeErrorResult(-5301, fmt::format("{}Path '{}' was not created.", prefix, getCreatedPath().toString()));
  }
  // Assuming nothing went wrong, return an empty Result<> object
  return {};
}

IDataAction::UniquePointer CreateDataGroupAction::clone() const
{
  return std::make_unique<CreateDataGroupAction>(getCreatedPath());
}

std::vector<DataPath> CreateDataGroupAction::getAllCreatedPaths() const
{
  return {getCreatedPath()};
}
} // namespace nx::core
