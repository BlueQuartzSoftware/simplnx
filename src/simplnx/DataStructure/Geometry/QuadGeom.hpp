#pragma once

#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @class QuadGeom
 * @brief
 */
class SIMPLNX_EXPORT QuadGeom : public INodeGeometry2D
{
public:
  friend class DataStructure;

  static inline constexpr usize k_NumEdgeVerts = 2;
  static inline constexpr usize k_NumFaceVerts = 4;
  static inline constexpr usize k_NumVerts = 4;
  static inline constexpr StringLiteral k_VoxelSizes = "Quad Areas";
  static inline constexpr StringLiteral k_EltsContainingVert = "Quads Containing Vert";
  static inline constexpr StringLiteral k_EltNeighbors = "Quad Neighbors";
  static inline constexpr StringLiteral k_EltCentroids = "Quad Centroids";
  static inline constexpr StringLiteral k_UnsharedEdges = "Unshared Edge List";
  static inline constexpr StringLiteral k_TypeName = "QuadGeom";
  /**
   * @brief
   * @param dataStructure
   * @param name
   * @param parentId = {}
   * @return QuadGeom*
   */
  static QuadGeom* Create(DataStructure& dataStructure, std::string name, const std::optional<IdType>& parentId = {});

  /**
   * @brief
   * @param dataStructure
   * @param name
   * @param importId
   * @param parentId = {}
   * @return QuadGeom*
   */
  static QuadGeom* Import(DataStructure& dataStructure, std::string name, IdType importId, const std::optional<IdType>& parentId = {});

  /**
   * @brief
   * @param other
   */
  QuadGeom(const QuadGeom& other) = default;

  /**
   * @brief
   * @param other
   */
  QuadGeom(QuadGeom&& other) = default;

  ~QuadGeom() noexcept override = default;

  QuadGeom& operator=(const QuadGeom&) = delete;
  QuadGeom& operator=(QuadGeom&&) noexcept = delete;

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
   * @return usize
   */
  usize getNumberOfCells() const override;

  /**
   * @brief
   * @return
   */
  usize getNumberOfVerticesPerFace() const override;

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

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findEdges() override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findUnsharedEdges() override;

protected:
  /**
   * @brief
   * @param dataStructure
   * @param name
   */
  QuadGeom(DataStructure& dataStructure, std::string name);

  /**
   * @brief
   * @param dataStructure
   * @param name
   * @param importId
   */
  QuadGeom(DataStructure& dataStructure, std::string name, IdType importId);
};
} // namespace nx::core
