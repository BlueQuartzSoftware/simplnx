#include "UpdateImageGeomAction.hpp"

#include "complex/DataStructure/Geometry/ImageGeom.hpp"

#include <fmt/core.h>

using namespace complex;

namespace complex
{
UpdateImageGeomAction::UpdateImageGeomAction(const std::optional<FloatVec3>& origin, const std::optional<FloatVec3>& spacing, const DataPath& path)
: m_Origin(origin)
, m_Spacing(spacing)
, m_Path(path)
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

  if(shouldUpdateOrigin())
  {
    image->setOrigin(m_Origin.value());
  }
  if(shouldUpdateSpacing())
  {
    image->setSpacing(m_Spacing.value());
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
} // namespace complex
