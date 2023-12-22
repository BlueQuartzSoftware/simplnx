#pragma once

#include "simplnx/DataStructure/Geometry/INodeGeometry1D.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @class EdgeGeom
 * @brief
 */
class SIMPLNX_EXPORT EdgeGeom : public INodeGeometry1D
{
public:
  friend class DataStructure;

  static inline constexpr usize k_NumVerts = 2;
  static inline constexpr usize k_NumEdgeVerts = 2;
  static inline constexpr StringLiteral k_VoxelSizes = "Edge Lengths";
  static inline constexpr StringLiteral k_EltsContainingVert = "Edges Containing Vert";
  static inline constexpr StringLiteral k_EltNeighbors = "Edge Neighbors";
  static inline constexpr StringLiteral k_EltCentroids = "Edge Centroids";
  static inline constexpr StringLiteral k_TypeName = "EdgeGeom";

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
  IGeometry::Type getGeomType() const override;

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return
   */
  DataObject::Type getDataObjectType() const override;

  /**
   * @brief Returns an enumeration of the class or subclass GroupType. Used for quick comparison or type deduction
   * @return
   */
  GroupType getGroupType() const override;

  /**
   * @brief Returns typename of the DataObject as a std::string.
   * @return std::string
   */
  std::string getTypeName() const override;

  /**
   * @brief
   * @return DataObject*
   */
  DataObject* shallowCopy() override;

  /**
   * @brief
   * @return DataObject*
   */
  std::shared_ptr<DataObject> deepCopy(const DataPath& copyPath) override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findElementSizes() override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findElementsContainingVert() override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findElementNeighbors() override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findElementCentroids() override;

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
