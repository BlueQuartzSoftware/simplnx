#include "CreateImageGeometryAction.hpp"

#include <fmt/core.h>

#include "complex/DataStructure/Geometry/ImageGeom.hpp"

using namespace complex;

namespace complex
{
CreateImageGeometryAction::CreateImageGeometryAction(DataPath path, DimensionType dims, OriginType origin, SpacingType spacing)
: m_GeometryPath(std::move(path))
, m_Dims(std::move(dims))
, m_Origin(std::move(origin))
, m_Spacing(std::move(spacing))
{
}

CreateImageGeometryAction::~CreateImageGeometryAction() noexcept = default;

Result<> CreateImageGeometryAction::apply(DataStructure& dataStructure, Mode mode) const
{
  // Check for empty Geometry DataPath
  if(m_GeometryPath.empty())
  {
    return MakeErrorResult(-220, "CreateImageGeometryAction: Geometry Path cannot be empty");
  }
  // Check if the Geometry Path already exists
  BaseGroup* parentObject = dataStructure.getDataAs<BaseGroup>(m_GeometryPath);
  if(parentObject != nullptr)
  {
    return MakeErrorResult(-222, fmt::format("CreateImageGeometryAction: DataObject already exists at path '{}'", m_GeometryPath.toString()));
  }
  DataPath parentPath = m_GeometryPath.getParent();
  if(!parentPath.empty())
  {
    Result<LinkedPath> geomPath = dataStructure.makePath(parentPath);
    if(geomPath.invalid())
    {
      return MakeErrorResult(-223, fmt::format("CreateGeometry2DAction: Geometry could not be created at path:'{}'", m_GeometryPath.toString()));
    }
  }
  // Get the Parent ID
  if(!dataStructure.getId(parentPath).has_value())
  {
    return MakeErrorResult(-224, fmt::format("CreateGeometry2DAction: Parent Id was not available for path:'{}'", parentPath.toString()));
  }

  // Create the ImageGeometry
  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, m_GeometryPath.getTargetName(), dataStructure.getId(parentPath).value());
  imageGeom->setDimensions(m_Dims);
  imageGeom->setOrigin(m_Origin);
  imageGeom->setSpacing(m_Spacing);

  return {};
}

const DataPath& CreateImageGeometryAction::path() const
{
  return m_GeometryPath;
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
