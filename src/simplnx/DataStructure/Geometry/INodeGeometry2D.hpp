#pragma once

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @class INodeGeometry2D
 * @brief Pure virtual interface for node-based geometries with faces.
 *
 * Contains the string constants and pure virtual methods that form the
 * public contract for 2D node geometries.
 */
class SIMPLNX_EXPORT INodeGeometry2D
{
public:
  static inline constexpr StringLiteral k_FaceAttributeMatrixName = "Face Data";
  static inline constexpr StringLiteral k_FaceFeatureAttributeMatrixName = "Face Feature Data";
  static inline constexpr StringLiteral k_SharedFacesListName = "Shared Faces List";

  virtual ~INodeGeometry2D() noexcept = default;

  /**
   * @brief
   * @return
   */
  virtual usize getNumberOfVerticesPerFace() const = 0;

  /**
   * @brief
   * @return IGeometry::StatusCode
   */
  virtual Result<> findEdges(bool recalculate) = 0;

  /**
   * @brief
   * @return IGeometry::StatusCode
   */
  virtual Result<> findUnsharedEdges(bool recalculate) = 0;

protected:
  INodeGeometry2D() = default;
  INodeGeometry2D(const INodeGeometry2D&) = default;
  INodeGeometry2D(INodeGeometry2D&&) = default;
  INodeGeometry2D& operator=(const INodeGeometry2D&) = default;
  INodeGeometry2D& operator=(INodeGeometry2D&&) noexcept = default;
};
} // namespace nx::core
