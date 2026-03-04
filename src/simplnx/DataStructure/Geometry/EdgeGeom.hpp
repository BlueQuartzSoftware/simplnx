#pragma once

#include "simplnx/DataStructure/Geometry/AbstractNodeGeometry1D.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @class EdgeGeom
 * @brief
 */
class SIMPLNX_EXPORT EdgeGeom : public AbstractNodeGeometry1D
{
public:
  friend class DataStructure;

  static constexpr usize k_NumVerts = 2;
  static constexpr usize k_NumEdgeVerts = 2;
  static constexpr StringLiteral k_VoxelSizes = "Edge Lengths";
  static constexpr StringLiteral k_EltsContainingVert = "Edges Containing Vert";
  static constexpr StringLiteral k_EltNeighbors = "Edge Neighbors";
  static constexpr StringLiteral k_EltCentroids = "Edge Centroids";
  static constexpr StringLiteral k_TypeName = "EdgeGeom";

  /**
   * @brief
   * @param dataStructure
   * @param name
   * @param parentId = {}
   * @return EdgeGeom*
   */
  static EdgeGeom* Create(DataStructure& dataStructure, std::string name, const std::optional<IdType>& parentId = {});

  /**
   * @brief
   * @param dataStructure
   * @param name
   * @param importId
   * @param parentId = {}
   * @return EdgeGeom*
   */
  static EdgeGeom* Import(DataStructure& dataStructure, std::string name, IdType importId, const std::optional<IdType>& parentId = {});

  /**
   * @brief
   * @param other
   */
  EdgeGeom(const EdgeGeom& other) = default;

  /**
   * @brief
   * @param other
   */
  EdgeGeom(EdgeGeom&& other) = default;

  ~EdgeGeom() noexcept override = default;

  EdgeGeom& operator=(const EdgeGeom&) = delete;
  EdgeGeom& operator=(EdgeGeom&&) noexcept = delete;

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
   * @brief calculates the sizes of each edge in the geometry
   * and store it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when
   * an Element Sizes Array exists and recalculate is `false`
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
   * @brief finds the neighbors of each edge in the geometry
   * and store it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when an
   * `ElementDynamicList` exists and recalculate is `false`
   * @return Result<>
   */
  Result<> findElementNeighbors(bool recalculate) override;

  /**
   * @brief calculates the centroids of each edge in the geometry
   * and store it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when an Element
   * Centroids Array exists and recalculate is `false`
   * @return Result<>
   */
  Result<> findElementCentroids(bool recalculate) override;

  /**
   * @brief
   * @return Point3D<float64>
   */
  Point3D<float64> getParametricCenter() const override;

  /**
   * @brief
   * @param pCoords
   * @param shape
   */
  void getShapeFunctions(const Point3D<float64>& pCoords, float64* shape) const override;

protected:
  /**
   * @brief
   * @param dataStructure
   * @param name
   */
  EdgeGeom(DataStructure& dataStructure, std::string name);

  /**
   * @brief
   * @param dataStructure
   * @param name
   * @param importId
   */
  EdgeGeom(DataStructure& dataStructure, std::string name, IdType importId);
};
} // namespace nx::core
