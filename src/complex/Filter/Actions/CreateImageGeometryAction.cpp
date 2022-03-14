#include "CreateImageGeometryAction.hpp"

#include <fmt/core.h>

#include "complex/DataStructure/Geometry/ImageGeom.hpp"

using namespace complex;

namespace complex
{
CreateImageGeometryAction::CreateImageGeometryAction(const DataPath& path, const DimensionType& dims, const OriginType& origin, const SpacingType& spacing)
: IDataCreationAction(path)
, m_Dims(dims)
, m_Origin(origin)
, m_Spacing(spacing)
{
}

CreateImageGeometryAction::~CreateImageGeometryAction() noexcept = default;

Result<> CreateImageGeometryAction::apply(DataStructure& dataStructure, Mode mode) const
{
  // Check for empty Geometry DataPath
  if(getCreatedPath().empty())
  {
    return MakeErrorResult(-220, "CreateImageGeometryAction: Geometry Path cannot be empty");
  }
  // Check if the Geometry Path already exists
  BaseGroup* parentObject = dataStructure.getDataAs<BaseGroup>(getCreatedPath());
  if(parentObject != nullptr)
  {
    return MakeErrorResult(-222, fmt::format("CreateImageGeometryAction: DataObject already exists at path '{}'", getCreatedPath().toString()));
  }
  DataPath parentPath = getCreatedPath().getParent();
  if(!parentPath.empty())
  {
    Result<LinkedPath> geomPath = dataStructure.makePath(parentPath);
    if(geomPath.invalid())
    {
      return MakeErrorResult(-223, fmt::format("CreateGeometry2DAction: Geometry could not be created at path:'{}'", getCreatedPath().toString()));
    }
  }
  // Get the Parent ID
  if(!dataStructure.getId(parentPath).has_value())
  {
    return MakeErrorResult(-224, fmt::format("CreateGeometry2DAction: Parent Id was not available for path:'{}'", parentPath.toString()));
  }

  // Create the ImageGeometry
  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, getCreatedPath().getTargetName(), dataStructure.getId(parentPath).value());
  imageGeom->setDimensions(m_Dims);
  imageGeom->setOrigin(m_Origin);
  imageGeom->setSpacing(m_Spacing);

  return {};
}

DataPath CreateImageGeometryAction::path() const
{
  return getCreatedPath();
}

const CreateImageGeometryAction::DimensionType& CreateImageGeometryAction::dims() const
{
  return m_Dims;
}

const CreateImageGeometryAction::OriginType& CreateImageGeometryAction::origin() const
{
  return m_Origin;
}

const CreateImageGeometryAction::SpacingType& CreateImageGeometryAction::spacing() const
{
  return m_Spacing;
}
} // namespace complex
