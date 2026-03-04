#pragma once

#include "simplnx/DataStructure/Geometry/AbstractNodeGeometry2D.hpp"
#include "simplnx/simplnx_export.hpp"

#include <nonstd/span.hpp>

namespace nx::core
{
/**
 * @class TriangleGeom
 * @brief
 */
class SIMPLNX_EXPORT TriangleGeom : public AbstractNodeGeometry2D
{
public:
  friend class DataStructure;

  static constexpr usize k_NumEdgeVerts = 2;
  static constexpr usize k_NumFaceVerts = 3;
  static constexpr usize k_NumVerts = 3;
  static constexpr StringLiteral k_VoxelSizes = "Triangle Areas";
  static constexpr StringLiteral k_EltsContainingVert = "Triangles Containing Vert";
  static constexpr StringLiteral k_EltNeighbors = "Triangle Neighbors";
  static constexpr StringLiteral k_EltCentroids = "Triangle Centroids";
  static constexpr StringLiteral k_TypeName = "TriangleGeom";

  /**
   * @brief
   * @param dataStructure
   * @param name
   * @param parentId = {}
   * @return TriangleGeom*
   */
  static TriangleGeom* Create(DataStructure& dataStructure, std::string name, const std::optional<IdType>& parentId = {});

  /**
   * @brief
   * @param dataStructure
   * @param name
   * @param importId
   * @param parentId = {}
   * @return TriangleGeom*
   */
  static TriangleGeom* Import(DataStructure& dataStructure, std::string name, IdType importId, const std::optional<IdType>& parentId = {});

  /**
   * @brief
   * @param other
   */
  TriangleGeom(const TriangleGeom& other) = default;

  /**
   * @brief
   * @param other
   */
  TriangleGeom(TriangleGeom&& other) = default;

  ~TriangleGeom() noexcept override = default;

  TriangleGeom& operator=(const TriangleGeom&) = delete;
  TriangleGeom& operator=(TriangleGeom&&) noexcept = delete;

  /**
   * @brief Returns the type of geometry.
   * @return
   */
  AbstractGeometry::Type getGeomType() const override;

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return
   */
  AbstractDataObject::Type getDataObjectType() const override;

  /**
   * @brief Returns an enumeration of the class or subclass GroupType. Used for quick comparison or type deduction
   * @return
   */
  GroupType getGroupType() const override;

  /**
   * @brief Returns typename of the AbstractDataObject as a std::string.
   * @return std::string
   */
  std::string getTypeName() const override;

  /**
   * @brief
   * @return AbstractDataObject*
   */
  AbstractDataObject* shallowCopy() override;

  /**
   * @brief
   * @return AbstractDataObject*
   */
  std::shared_ptr<AbstractDataObject> deepCopy(const DataPath& copyPath) override;

  /**
   *
   * @return
   */
  usize getNumberOfVerticesPerEdge() const override;

  /**
   * @brief
   * @return usize
   */
  usize getNumberOfCells() const override;

  /**
   * @brief
   * @return
   */
  usize getNumberOfVerticesPerFace() const override;

  /**
   * @brief calculates the sizes of each triangle in the geometry
   * and stores it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when an
   * Element Sizes Array exists and recalculate is `false`
   * @return Result<>
   */
  Result<> findElementSizes(bool recalculate) override;

  /**
   * @brief
   * @param recalculate This will allow for skipping execution when an
   * `ElementDynamicList` exists and recalculate is `false`
   * @return Result<>
   */
  Result<> findElementsContainingVert(bool recalculate) override;

  /**
   * @brief finds the neighbors of each triangle in the geometry
   * and stores it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when an
   * `ElementDynamicList` exists and recalculate is `false`
   * @return Result<>
   */
  Result<> findElementNeighbors(bool recalculate) override;

  /**
   * @brief calculates the centroid of each triangle in the geometry
   * and stores it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when an
   * Element Centroids Array exists and recalculate is `false`
   * @return Result<>
   */
  Result<> findElementCentroids(bool recalculate) override;

  /**
   * @brief
   * @param pCoords
   * @return Point3D<float64>
   */
  Point3D<float64> getParametricCenter() const override;

  /**
   * @brief
   * @param pCoords
   * @param shape
   */
  void getShapeFunctions(const Point3D<float64>& pCoords, float64* shape) const override;

  /**
   * @brief finds the shared edges (no duplicates) of each triangle in the
   * geometry and stores it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when a
   * Shared Edge Array exists and recalculate is `false`
   * @return Result<>
   */
  Result<> findEdges(bool recalculate) override;

  /**
   * @brief finds the edges (including duplicates) of each triangle in the
   * geometry and stores it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when an
   * Unshared Edges Array exists and recalculate is `false`
   * @return Result<>
   */
  Result<> findUnsharedEdges(bool recalculate) override;

protected:
  /**
   * @brief
   * @param dataStructure
   * @param name
   */
  TriangleGeom(DataStructure& dataStructure, std::string name);

  /**
   * @brief
   * @param dataStructure
   * @param name
   * @param importId
   */
  TriangleGeom(DataStructure& dataStructure, std::string name, IdType importId);
};
} // namespace nx::core
