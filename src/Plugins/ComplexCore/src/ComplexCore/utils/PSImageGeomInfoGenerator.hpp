#pragma once

#include <optional>

#include "ComplexCore/utils/PSInfoGenerator.hpp"
#include "complex/Common/Array.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"

namespace complex
{
/**
 * @class ImageGeom
 * @brief
 */
class COMPLEXCORE_EXPORT PSImageGeomInfoGenerator : public PSInfoGenerator
{
public:
  PSImageGeomInfoGenerator(const ImageGeom& geometry, const SizeVec3& numOfPartitionsPerAxis);
  ~PSImageGeomInfoGenerator() noexcept override = default;

  /**
   * @brief
   * @param other
   */
  PSImageGeomInfoGenerator(const PSImageGeomInfoGenerator& other) = default;

  /**
   * @brief
   * @param other
   */
  PSImageGeomInfoGenerator(PSImageGeomInfoGenerator&& other) = default;

  PSImageGeomInfoGenerator& operator=(const PSImageGeomInfoGenerator&) = delete;
  PSImageGeomInfoGenerator& operator=(PSImageGeomInfoGenerator&&) noexcept = delete;

  std::optional<FloatVec3> getOrigin() const override;
  std::optional<FloatVec3> getPartitionLength() const override;
  IGeometry::LengthUnit getUnits() const override;

private:
  const ImageGeom& m_Geometry;
};
} // namespace complex
