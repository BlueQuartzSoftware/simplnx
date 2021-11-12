#include "CreateImageGeometryAction.hpp"

#include <fmt/core.h>

#include "complex/DataStructure/Geometry/ImageGeom.hpp"

using namespace complex;

namespace complex
{
CreateImageGeometryAction::CreateImageGeometryAction(DataPath path, SizeVec3 dims, FloatVec3 origin, FloatVec3 spacing)
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

  ImageGeom::Create(dataStructure, name, id);

  return {};
}

const DataPath& CreateImageGeometryAction::path() const
{
  return m_Path;
}

const SizeVec3& CreateImageGeometryAction::dims() const
{
  return m_Dims;
}

const FloatVec3& CreateImageGeometryAction::origin() const
{
  return m_Origin;
}

const FloatVec3& CreateImageGeometryAction::spacing() const
{
  return m_Spacing;
}
} // namespace complex
