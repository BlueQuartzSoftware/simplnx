#pragma once

#include <optional>

#include "ComplexCore/utils/PSInfoGenerator.hpp"
#include "complex/Common/Array.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class ImageGeom
 * @brief
 */
class COMPLEX_EXPORT PSImageGeomInfoGenerator : public PSInfoGenerator
{
public:
  PSImageGeomInfoGenerator(const ImageGeom& geometry, const SizeVec3& numOfPartitionsPerAxis);

  std::optional<FloatVec3> getOrigin() const override;
  std::optional<FloatVec3> getPartitionLength() const override;
  IGeometry::LengthUnit getUnits() const override;

private:
  const ImageGeom& m_Geometry;
};
} // namespace complex
