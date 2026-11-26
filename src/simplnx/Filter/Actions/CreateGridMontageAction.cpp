#include "simplnx/Filter/Actions/CreateGridMontageAction.hpp"

#include "simplnx/DataStructure/Montage/GridMontage.hpp"
#include <utility>

namespace nx::core
{
CreateGridMontageAction::CreateGridMontageAction(const DataPath& path, DimensionType dims, OriginType origin, SpacingType spacing)
: IDataCreationAction(path)
, m_Dims(std::move(dims))
, m_Origin(std::move(origin))
, m_Spacing(std::move(spacing))
{
}

Result<> CreateGridMontageAction::apply(DataStructure& dataStructure, Mode /*mode*/) const
{
  static constexpr StringLiteral k_Prefix = "CreateGridMontageAction: ";
  // Check for empty Geometry DataPath
  if(getCreatedPath().empty())
  {
    return MakeErrorResult(-5901, fmt::format("{}Geometry Path cannot be empty", k_Prefix));
  }
  // Check if the Geometry Path already exists
  auto* parentObjectPtr = dataStructure.getDataAs<BaseGroup>(getCreatedPath());
  if(parentObjectPtr != nullptr)
  {
    return MakeErrorResult(-5902, fmt::format("{}DataObject already exists at path '{}'", k_Prefix, getCreatedPath().toString()));
  }
  DataPath parentPath = getCreatedPath().getParent();
  if(!parentPath.empty())
  {
    Result<LinkedPath> geomPath = dataStructure.makePath(parentPath);
    if(geomPath.invalid())
    {
      return MakeErrorResult(-5903, fmt::format("{}Geometry could not be created at path:'{}'", k_Prefix, getCreatedPath().toString()));
    }
  }
  // Get the Parent ID
  if(!dataStructure.getId(parentPath).has_value())
  {
    return MakeErrorResult(-5904, fmt::format("{}Parent Id was not available for path:'{}'", k_Prefix, parentPath.toString()));
  }

  // Create the grid montage
  GridMontage* gridMontagePtr = GridMontage::Create(dataStructure, getCreatedPath().getTargetName(), dataStructure.getId(parentPath));
  if(gridMontagePtr == nullptr)
  {
    return MakeErrorResult(-5905, fmt::format("{}Failed to create GridMontage:'{}'", k_Prefix, getCreatedPath().toString()));
  }

  return {};
}

IDataAction::UniquePointer CreateGridMontageAction::clone() const
{
  return std::make_unique<CreateGridMontageAction>(getCreatedPath(), m_Dims, m_Origin, m_Spacing);
}

DataPath CreateGridMontageAction::path() const
{
  return getCreatedPath();
}

const CreateGridMontageAction::DimensionType& CreateGridMontageAction::dims() const
{
  return m_Dims;
}

const CreateGridMontageAction::OriginType& CreateGridMontageAction::origin() const
{
  return m_Origin;
}

const CreateGridMontageAction::SpacingType& CreateGridMontageAction::spacing() const
{
  return m_Spacing;
}

std::vector<DataPath> CreateGridMontageAction::getAllCreatedPaths() const
{
  return {getCreatedPath()};
}
} // namespace nx::core
