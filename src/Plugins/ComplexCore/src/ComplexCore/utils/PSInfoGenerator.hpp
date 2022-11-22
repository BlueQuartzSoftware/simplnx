#pragma once

#include <optional>

#include "ComplexCore/ComplexCore_export.hpp"
#include "complex/Common/Array.hpp"
#include "complex/DataStructure/AbstractDataStore.hpp"
#include "complex/DataStructure/Geometry/IGeometry.hpp"

namespace complex
{
/**
 * @class ImageGeom
 * @brief
 */
class COMPLEXCORE_EXPORT PSInfoGenerator
{
public:
  virtual ~PSInfoGenerator() noexcept = default;

  /**
   * @brief
   * @param other
   */
  PSInfoGenerator(const PSInfoGenerator& other) = default;

  /**
   * @brief
   * @param other
   */
  PSInfoGenerator(PSInfoGenerator&& other) = default;

  PSInfoGenerator& operator=(const PSInfoGenerator&) = delete;
  PSInfoGenerator& operator=(PSInfoGenerator&&) noexcept = delete;

  virtual std::optional<FloatVec3> getOrigin() const = 0;
  virtual std::optional<FloatVec3> getPartitionLength() const = 0;
  virtual IGeometry::LengthUnit getUnits() const = 0;

  const SizeVec3& numOfPartitionsPerAxis() const;

  FloatVec3 calculatePartitionLengthsUsingBounds(const FloatVec3& ll, const FloatVec3& ur) const;

protected:
  PSInfoGenerator(const SizeVec3& numOfPartitionsPerAxis);

private:
  SizeVec3 m_NumberOfPartitionsPerAxis;
};

} // namespace complex
