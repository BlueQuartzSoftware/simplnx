#include "complex/Filter/Actions/CreateGridMontageAction.hpp"
#include "complex/DataStructure/Montage/GridMontage.hpp"

namespace complex
{
CreateGridMontageAction::CreateGridMontageAction(const DataPath& path, const DimensionType& dims, const OriginType& origin, const SpacingType& spacing)
: IDataCreationAction(path)
, m_Dims(dims)
, m_Origin(origin)
, m_Spacing(spacing)
{
}

Result<> CreateGridMontageAction::apply(DataStructure& dataStructure, Mode mode) const
{
  static constexpr StringLiteral prefix = "CreateGridMontageAction: ";
  // Check for empty Geometry DataPath
  if(getCreatedPath().empty())
  {
    return MakeErrorResult(-5701, fmt::format("{}Geometry Path cannot be empty", prefix));
  }
  // Check if the Geometry Path already exists
  auto* parentObject = dataStructure.getDataAs<BaseGroup>(getCreatedPath());
  if(parentObject != nullptr)
  {
    return MakeErrorResult(-5702, fmt::format("{}DataObject already exists at path '{}'", prefix, getCreatedPath().toString()));
  }
  DataPath parentPath = getCreatedPath().getParent();
  if(!parentPath.empty())
  {
    Result<LinkedPath> geomPath = dataStructure.makePath(parentPath);
    if(geomPath.invalid())
    {
      return MakeErrorResult(-5703, fmt::format("{}Geometry could not be created at path:'{}'", prefix, getCreatedPath().toString()));
    }
  }
  // Get the Parent ID
  if(!dataStructure.getId(parentPath).has_value())
  {
    return MakeErrorResult(-5704, fmt::format("{}Parent Id was not available for path:'{}'", prefix, parentPath.toString()));
  }

  // Create the ImageGeometry
  GridMontage* gridMontage = GridMontage::Create(dataStructure, getCreatedPath().getTargetName(), dataStructure.getId(parentPath));
  if(gridMontage == nullptr)
  {
    return MakeErrorResult(-5705, fmt::format("{}Failed to create GridMontage:'{}'", prefix, getCreatedPath().toString()));
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
} // namespace complex
