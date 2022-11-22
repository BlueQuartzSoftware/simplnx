#pragma once

#include <optional>

#include "ComplexCore/utils/PSInfoGenerator.hpp"
#include "complex/Common/Array.hpp"
#include "complex/DataStructure/Geometry/RectGridGeom.hpp"

namespace complex
{
/**
 * @class ImageGeom
 * @brief
 */
class COMPLEXCORE_EXPORT PSRectGridGeomInfoGenerator : public PSInfoGenerator
{
public:
  PSRectGridGeomInfoGenerator(const RectGridGeom& geometry, const SizeVec3& numOfPartitionsPerAxis);
  ~PSRectGridGeomInfoGenerator() noexcept override = default;

  /**
   * @brief
   * @param other
   */
  PSRectGridGeomInfoGenerator(const PSRectGridGeomInfoGenerator& other) = default;

  /**
   * @brief
   * @param other
   */
  PSRectGridGeomInfoGenerator(PSRectGridGeomInfoGenerator&& other) = default;

  PSRectGridGeomInfoGenerator& operator=(const PSRectGridGeomInfoGenerator&) = delete;
  PSRectGridGeomInfoGenerator& operator=(PSRectGridGeomInfoGenerator&&) noexcept = delete;

  std::optional<FloatVec3> getOrigin() const override;
  std::optional<FloatVec3> getPartitionLength() const override;
  IGeometry::LengthUnit getUnits() const override;

private:
  const RectGridGeom& m_Geometry;
};
} // namespace complex
