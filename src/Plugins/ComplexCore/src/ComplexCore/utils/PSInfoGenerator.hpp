#pragma once

#include <optional>

#include "complex/Common/Array.hpp"
#include "complex/DataStructure/AbstractDataStore.hpp"
#include "complex/DataStructure/Geometry/IGeometry.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class ImageGeom
 * @brief
 */
class COMPLEX_EXPORT PSInfoGenerator
{
public:
  virtual std::optional<FloatVec3> getOrigin() const = 0;
  virtual std::optional<FloatVec3> getPartitionLength() const = 0;
  virtual IGeometry::LengthUnit getUnits() const = 0;

  const SizeVec3& numOfPartitionsPerAxis() const;

  FloatVec3 calculatePartitionLengthsUsingBounds(const FloatVec3& ll, const FloatVec3& ur) const;

protected:
  PSInfoGenerator(SizeVec3 numOfPartitionsPerAxis);

private:
  SizeVec3 m_NumberOfPartitionsPerAxis;
};

} // namespace complex
