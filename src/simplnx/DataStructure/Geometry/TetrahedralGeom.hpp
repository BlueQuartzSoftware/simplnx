#pragma once

#include "simplnx/DataStructure/Geometry/INodeGeometry3D.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @class TetrahedralGeom
 * @brief
 */
class SIMPLNX_EXPORT TetrahedralGeom : public INodeGeometry3D
{
public:
  friend class DataStructure;

  static inline constexpr usize k_NumEdgeVerts = 2;
  static inline constexpr usize k_NumFaceVerts = 3;
  static inline constexpr usize k_NumVerts = 4;
  static inline constexpr StringLiteral k_VoxelSizes = "Tet Volumes";
  static inline constexpr StringLiteral k_EltsContainingVert = "Elements Containing Vert";
  static inline constexpr StringLiteral k_EltNeighbors = "Tet Neighbors";
  static inline constexpr StringLiteral k_EltCentroids = "Tet Centroids";
  static inline constexpr StringLiteral k_UnsharedEdges = "Unshared Edge List";
  static inline constexpr StringLiteral k_UnsharedFaces = "Unshared Face List";
  static inline constexpr StringLiteral k_TypeName = "TetrahedralGeom";

  /**
   * @brief
   * @param dataStructure
   * @param name
   * @param parentId = {}
   * @return TetrahedralGeom*
   */
  static TetrahedralGeom* Create(DataStructure& dataStructure, std::string name, const std::optional<IdType>& parentId = {});

  /**
   * @brief
   * @param dataStructure
   * @param name
   * @param importId
   * @param parentId = {}
   * @return TetrahedralGeom*
   */
  static TetrahedralGeom* Import(DataStructure& dataStructure, std::string name, IdType importId, const std::optional<IdType>& parentId = {});

  /**
   * @brief
   * @param other
   */
  TetrahedralGeom(const TetrahedralGeom& other) = default;

  /**
   * @brief
   * @param other
   */
  TetrahedralGeom(TetrahedralGeom&& other) = default;

  ~TetrahedralGeom() noexcept override = default;

  TetrahedralGeom& operator=(const TetrahedralGeom&) = delete;
  TetrahedralGeom& operator=(TetrahedralGeom&&) noexcept = delete;

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
   * @return
   */
  usize getNumberOfVerticesPerFace() const override;

  /**
   * @brief
   * @return
   */
  usize getNumberOfVerticesPerCell() const override;

  /**
   * @brief Returns the number of tetrahedrons in this geometry
   * @return usize
   */
  usize getNumberOfCells() const override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findElementSizes(bool recalculate) override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findElementsContainingVert(bool recalculate) override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findElementNeighbors(bool recalculate) override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findElementCentroids(bool recalculate) override;

  /**
   * @brief
   * @param pCoords
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
  StatusCode findEdges(bool recalculate) override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findFaces(bool recalculate) override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findUnsharedEdges(bool recalculate) override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findUnsharedFaces(bool recalculate) override;

protected:
  /**
   * @brief
   * @param dataStructure
   * @param name
   */
  TetrahedralGeom(DataStructure& dataStructure, std::string name);

  /**
   * @brief
   * @param dataStructure
   * @param name
   * @param importId
   */
  TetrahedralGeom(DataStructure& dataStructure, std::string name, IdType importId);
};
} // namespace nx::core
