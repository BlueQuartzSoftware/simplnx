#include "UpdateImageGeomAction.hpp"

#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"

#include <fmt/core.h>

using namespace nx::core;

namespace nx::core
{
UpdateImageGeomAction::UpdateImageGeomAction(const std::optional<FloatVec3>& origin, const std::optional<FloatVec3>& spacing, const DataPath& path, bool centerOrigin)
: m_Origin(origin)
, m_Spacing(spacing)
, m_Path(path)
, m_CenterOrigin(centerOrigin)
{
}

UpdateImageGeomAction::~UpdateImageGeomAction() noexcept = default;

Result<> UpdateImageGeomAction::apply(DataStructure& dataStructure, Mode mode) const
{
  static constexpr StringLiteral prefix = "UpdateImageGeomAction: ";
  auto* image = dataStructure.getDataAs<ImageGeom>(path());
  if(image == nullptr)
  {
    return MakeErrorResult(-6701, fmt::format("{}Unable to find ImageGeom at '{}'", prefix, path().toString()));
  }

  if(shouldUpdateSpacing())
  {
    image->setSpacing(m_Spacing.value());
  }

  if(shouldUpdateOrigin())
  {
    image->setOrigin(m_Origin.value());
  }

  // This must be done last since spacing affects it and the origin may not be set resulting in invalid access
  if(m_CenterOrigin)
  {
    Point3Df origin = image->getOrigin();
    BoundingBox3Df bounds = image->getBoundingBoxf();
    Point3Df centerPoint = bounds.center();
    Point3Df minPoint = bounds.getMinPoint();

    for(uint8 i = 0; i < 3; i++)
    {
      // absolute center - absolute min point = absolute offset
      float32 offset = std::abs(centerPoint[i]) - std::abs(minPoint[i]);
      // subtract absolute offset and save point into "new" origin
      origin[i] -= offset;
    }
    image->setOrigin(origin);
  }

  return {};
}

IDataAction::UniquePointer UpdateImageGeomAction::clone() const
{
  return std::make_unique<UpdateImageGeomAction>(m_Origin, m_Spacing, m_Path);
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
