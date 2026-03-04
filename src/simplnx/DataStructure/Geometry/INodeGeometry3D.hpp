#pragma once

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @class INodeGeometry3D
 * @brief Pure virtual interface for node-based geometries with polyhedra.
 *
 * Contains the string constants and pure virtual methods that form the
 * public contract for 3D node geometries.
 */
class SIMPLNX_EXPORT INodeGeometry3D
{
public:
  static inline constexpr StringLiteral k_PolyhedronDataName = "Polyhedron Data";
  static inline constexpr StringLiteral k_SharedPolyhedronListName = "Shared Polyhedron List";

  virtual ~INodeGeometry3D() noexcept = default;

  /**
   * @brief Pure-Virtual intended to find the shared faces of each element
   * in the geometry and store it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when a Shared Face
   * Array exists and recalculate is `false`
   * @return Result<>
   */
  virtual Result<> findFaces(bool recalculate) = 0;

  /**
   * @brief Creates and assigns the unshared face list array for the current values.
   */
  virtual Result<> findUnsharedFaces(bool recalculate) = 0;

  /**
   * @brief Returns the number of vertices in the cell.
   * @return
   */
  virtual usize getNumberOfVerticesPerCell() const = 0;

protected:
  INodeGeometry3D() = default;
  INodeGeometry3D(const INodeGeometry3D&) = default;
  INodeGeometry3D(INodeGeometry3D&&) = default;
  INodeGeometry3D& operator=(const INodeGeometry3D&) = default;
  INodeGeometry3D& operator=(INodeGeometry3D&&) noexcept = default;
};
} // namespace nx::core
