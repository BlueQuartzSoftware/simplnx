#include "UpdateImageGeomAction.hpp"

#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"

#include <fmt/core.h>

using namespace nx::core;

namespace
{
constexpr int32 k_AssignedSpacing = -86028;
constexpr int32 k_AssignedOrigin = -86029;
constexpr int32 k_CenteredOrigin = -86030;
} // namespace

namespace nx::core
{
UpdateImageGeomAction::UpdateImageGeomAction(const std::optional<FloatVec3>& origin, const std::optional<FloatVec3>& spacing, const DataPath& path, bool centerOrigin, bool validateAssignedValues)
: m_Origin(origin)
, m_Spacing(spacing)
, m_Path(path)
, m_CenterOrigin(centerOrigin)
, m_ValidateAssignedValues(validateAssignedValues)
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

  const FloatVec3 spacing = m_Spacing.value_or(image->getSpacing());
  FloatVec3 origin = m_Origin.value_or(image->getOrigin());
  const auto dims = image->getDimensions();
  const FloatVec3 assignedOrigin = origin;
  if(m_CenterOrigin)
  {
    const BoundingBox3Df bounds(std::array<float32, 6>{origin[0], origin[1], origin[2], origin[0] + dims[0] * spacing[0], origin[1] + dims[1] * spacing[1], origin[2] + dims[2] * spacing[2]});
    const auto center = bounds.center();
    const auto min = bounds.getMinPoint();
    for(usize axis = 0; axis < 3; ++axis)
    {
      origin[axis] -= center[axis] - min[axis];
    }
  }
  if(m_ValidateAssignedValues && mode == Mode::Execute)
  {
    for(usize axis = 0; axis < 3; ++axis)
    {
      if(m_Spacing.has_value() && (!std::isfinite(spacing[axis]) || !(spacing[axis] > 0)))
      {
        return MakeErrorResult(k_AssignedSpacing, fmt::format("Image Geometry '{}' {}: assigned spacing {} (existing spacing {}, computed result {}) must be finite and greater than 0.",
                                                              path().toString(), "XYZ"[axis], spacing[axis], image -> getSpacing()[axis], spacing[axis]));
      }
      if(m_Origin.has_value() && !std::isfinite(assignedOrigin[axis]))
      {
        return MakeErrorResult(k_AssignedOrigin, fmt::format("Image Geometry '{}' {}: assigned origin {} (existing origin {}, computed result {}) must be finite.", path().toString(), "XYZ"[axis],
                                                             assignedOrigin[axis], image -> getOrigin()[axis], origin[axis]));
      }
      if(m_CenterOrigin && !std::isfinite(origin[axis]))
      {
        return MakeErrorResult(k_CenteredOrigin, fmt::format("Image Geometry '{}' {}: centering the origin {} with spacing {} and dimension {} produces {}. The computed origin must be finite.",
                                                             path().toString(), "XYZ"[axis], assignedOrigin[axis], spacing[axis], dims[axis], origin[axis]));
      }
    }
  }
  if(shouldUpdateSpacing())
  {
    image->setSpacing(spacing);
  }
  if(shouldUpdateOrigin() || m_CenterOrigin)
  {
    image->setOrigin(origin);
  }

  return {};
}

IDataAction::UniquePointer UpdateImageGeomAction::clone() const
{
  return std::make_unique<UpdateImageGeomAction>(m_Origin, m_Spacing, m_Path, m_CenterOrigin, m_ValidateAssignedValues);
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
