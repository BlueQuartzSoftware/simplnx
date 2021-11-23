#include "CreateImageGeometryAction.hpp"

#include <fmt/core.h>

#include "complex/DataStructure/Geometry/ImageGeom.hpp"

using namespace complex;

namespace complex
{
CreateImageGeometryAction::CreateImageGeometryAction(DataPath path, DimensionType dims, OriginType origin, SpacingType spacing)
: m_Path(std::move(path))
, m_Dims(std::move(dims))
, m_Origin(std::move(origin))
, m_Spacing(std::move(spacing))
{
}

CreateImageGeometryAction::~CreateImageGeometryAction() noexcept = default;

Result<> CreateImageGeometryAction::apply(DataStructure& dataStructure, Mode mode) const
{
  if(m_Path.empty())
  {
    return MakeErrorResult(-1, "CreateImageGeometryAction: Path cannot be empty");
  }

  usize size = m_Path.getLength();
  std::string name = size == 1 ? m_Path[0] : m_Path[size - 1];

  std::optional<DataObject::IdType> id;
  if(size > 1)
  {
    DataPath parentPath = m_Path.getParent();
    id = dataStructure.getId(parentPath);
    if(!id.has_value())
    {
      return MakeErrorResult(-2, "CreateImageGeometryAction: Invalid path");
    }
  }

  BaseGroup* dataObject = dataStructure.getDataAs<BaseGroup>(id);
  if(dataObject->contains(name))
  {
    return MakeErrorResult(-200, fmt::format("CreateImageGeometryAction: DataObject already exists at path '{}'", m_Path.toString()));
  }

  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, name, id);
  imageGeom->setDimensions(m_Dims);
  imageGeom->setOrigin(m_Origin);
  imageGeom->setSpacing(m_Spacing);

  return {};
}

const DataPath& CreateImageGeometryAction::path() const
{
  return m_Path;
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
