#include "UpdateImageGeomAction.hpp"

#include <fmt/core.h>

#include "complex/Common/TypeTraits.hpp"
#include "complex/DataStructure/EmptyDataStore.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

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
  auto* image = dataStructure.getDataAs<ImageGeom>(path());
  if(image == nullptr)
  {
    return MakeErrorResult(-275, fmt::format("Unable to find ImageGeom at '{}'", path().toString()));
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
