#include "UpdateImageGeomAction.hpp"

#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"

#include <fmt/core.h>

#include <utility>

using namespace nx::core;

namespace nx::core
{
UpdateImageGeomAction::UpdateImageGeomAction(const std::optional<FloatVec3>& origin, const std::optional<FloatVec3>& spacing, DataPath  path, bool centerOrigin)
: m_Origin(origin)
, m_Spacing(spacing)
, m_Path(std::move(path))
, m_CenterOrigin(centerOrigin)
{
}

UpdateImageGeomAction::~UpdateImageGeomAction() noexcept = default;

Result<> UpdateImageGeomAction::apply(DataStructure& dataStructure, Mode /*mode*/) const
{
  static constexpr StringLiteral prefix = "UpdateImageGeomAction: ";
  auto* image = dataStructure.getDataAs<ImageGeom>(path());
  if(image == nullptr)
  {
    return MakeErrorResult(-6701, fmt::format("{}Unable to find ImageGeom at '{}'", prefix, path().toString()));
  }

  const auto addGeometryPath = [&](Result<> result) {
    for(auto& error : result.errors())
    {
      error.message = fmt::format("{}Geometry at '{}': {}", prefix, path().toString(), error.message);
    }
    return result;
  };
  const FloatVec3 spacing = m_Spacing.value_or(image->getSpacing());
  FloatVec3 origin = m_Origin.value_or(image->getOrigin());
  if(auto result = ImageGeom::ValidateSpacing(spacing); result.invalid())
  {
    return addGeometryPath(std::move(result));
  }
  if(auto result = ImageGeom::ValidateOrigin(origin); result.invalid())
  {
    return addGeometryPath(std::move(result));
  }
  if(m_CenterOrigin)
  {
    const SizeVec3 dims = image->getDimensions();
    const BoundingBox3Df bounds(std::array<float32, 6>{origin[0], origin[1], origin[2], origin[0] + dims[0] * spacing[0], origin[1] + dims[1] * spacing[1], origin[2] + dims[2] * spacing[2]});
    const Point3Df center = bounds.center();
    const Point3Df min = bounds.getMinPoint();
    for(usize axis = 0; axis < 3; ++axis)
    {
      origin[axis] -= center[axis] - min[axis];
    }
    if(auto result = ImageGeom::ValidateOrigin(origin); result.invalid())
    {
      return addGeometryPath(std::move(result));
    }
  }

  if(shouldUpdateSpacing())
  {
    if(auto result = image->setSpacing(spacing); result.invalid())
    {
      return addGeometryPath(std::move(result));
    }
  }
  if(shouldUpdateOrigin() || m_CenterOrigin)
  {
    if(auto result = image->setOrigin(origin); result.invalid())
    {
      return addGeometryPath(std::move(result));
    }
  }

  return {};
}

IDataAction::UniquePointer UpdateImageGeomAction::clone() const
{
  return std::make_unique<UpdateImageGeomAction>(m_Origin, m_Spacing, m_Path, m_CenterOrigin);
}

bool UpdateImageGeomAction::shouldUpdateOrigin() const
{
  return m_Origin.has_value();
}

bool UpdateImageGeomAction::shouldUpdateSpacing() const
{
  return m_Spacing.has_value();
}

const std::optional<FloatVec3>& UpdateImageGeomAction::origin() const
{
  return m_Origin;
}

const std::optional<FloatVec3>& UpdateImageGeomAction::spacing() const
{
  return m_Spacing;
}

const DataPath& UpdateImageGeomAction::path() const
{
  return m_Path;
}
} // namespace nx::core
