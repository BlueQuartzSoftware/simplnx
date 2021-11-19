#include "EmptyAction.hpp"

#include <fmt/core.h>

#include "complex/DataStructure/Geometry/ImageGeom.hpp"

using namespace complex;

namespace complex
{

EmptyAction::~EmptyAction() noexcept = default;

Result<> EmptyAction::apply(DataStructure& dataStructure, Mode mode) const
{
  return {};
}

} // namespace complex
