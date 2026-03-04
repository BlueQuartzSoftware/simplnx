#pragma once

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @class INodeGeometry1D
 * @brief Pure virtual interface for node-based geometries with edges.
 *
 * Contains the string constants and pure virtual methods that form the
 * public contract for 1D node geometries.
 */
class SIMPLNX_EXPORT INodeGeometry1D
{
public:
  static constexpr StringLiteral k_EdgeAttributeMatrixName = "Edge Data";
  static constexpr StringLiteral k_EdgeFeatureAttributeMatrix = "Edge Feature Data";
  static constexpr StringLiteral k_SharedEdgeListName = "Shared Edge List";
  static constexpr StringLiteral k_UnsharedEdgesListName = "Unshared Edge List";
  static constexpr StringLiteral k_UnsharedFacesListName = "Unshared Face List";

  static constexpr usize k_NumEdgeVerts = 2;

  virtual ~INodeGeometry1D() noexcept = default;

  /**
   * @brief
   * @return
   */
  virtual usize getNumberOfVerticesPerEdge() const = 0;

  /**
   * @brief
   * @return IGeometry::StatusCode
   */
  virtual Result<> findElementsContainingVert(bool recalculate) = 0;

  /**
   * @brief
   * @return IGeometry::StatusCode
   */
  virtual Result<> findElementNeighbors(bool recalculate) = 0;

  /**
   * @brief
   * @return IGeometry::StatusCode
   */
  virtual Result<> findElementCentroids(bool recalculate) = 0;

protected:
  INodeGeometry1D() = default;
  INodeGeometry1D(const INodeGeometry1D&) = default;
  INodeGeometry1D(INodeGeometry1D&&) = default;
  INodeGeometry1D& operator=(const INodeGeometry1D&) = default;
  INodeGeometry1D& operator=(INodeGeometry1D&&) noexcept = default;
};
} // namespace nx::core
