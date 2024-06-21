#pragma once

#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"

namespace nx::core
{
class SIMPLNX_EXPORT INodeGeometry3D : public INodeGeometry2D
{
public:
  static inline constexpr StringLiteral k_PolyhedronDataName = "Polyhedron Data";
  static inline constexpr StringLiteral k_TriangleFaceList = "Shared Tri List";
  static inline constexpr StringLiteral k_QuadFaceList = "Shared Quad List";
  static inline constexpr StringLiteral k_TypeName = "INodeGeometry3D";

  INodeGeometry3D() = delete;
  INodeGeometry3D(const INodeGeometry3D&) = default;
  INodeGeometry3D(INodeGeometry3D&&) = default;

  INodeGeometry3D& operator=(const INodeGeometry3D&) = delete;
  INodeGeometry3D& operator=(INodeGeometry3D&&) noexcept = delete;

  ~INodeGeometry3D() noexcept override = default;

  void setPolyhedronListId(const OptionalId& polyListId);

  /**
   * @brief
   * @return
   */
  SharedFaceList* getPolyhedra();

  /**
   * @brief
   * @return
   */
  const SharedFaceList* getPolyhedra() const;

  /**
   * @brief
   * @return
   */
  SharedFaceList& getPolyhedraRef();

  /**
   * @brief
   * @return
   */
  const SharedFaceList& getPolyhedraRef() const;

  /**
   * @brief
   * @param polyhedra
   */
  void setPolyhedraList(const SharedFaceList& polyhedra);

  /**
   * @brief Resizes the polyhedra list to the target size.
   * @param size
   */
  void resizePolyhedraList(usize size);

  OptionalId getPolyhedraDataId() const;

  /**
   * @brief Returns the number of polyhedra in the geometry.
   * @return usize
   */
  usize getNumberOfPolyhedra() const;

  /**
   * @brief Creates and assigns the face list array for the current values.
   * @return StatusCode
   */
  virtual StatusCode findFaces(bool recalculate) = 0;

  /**
   * @brief Deletes the current face list array.
   */
  void deleteFaces();

  /**
   * @brief
   * @return
   */
  const std::optional<IdType>& getUnsharedFacesId() const;

  /**
   * @brief
   * @return
   */
  void setUnsharedFacedId(const OptionalId& id);

  /**
   * @brief Creates and assigns the unshared face list array for the current values.
   */
  virtual StatusCode findUnsharedFaces(bool recalculate) = 0;

  /**
   * @brief Returns a pointer to the unshared face list array.
   * @return
   */
  const SharedFaceList* getUnsharedFaces() const;

  /**
   * @brief Deletes the current unshared face list array.
   */
  void deleteUnsharedFaces();

  /**
   * @brief Returns the number of vertices in the cell.
   * @return
   */
  virtual usize getNumberOfVerticesPerCell() const = 0;

  /**
   * @brief
   * @param tetId
   * @param vertexIds The index into the shared vertex list of each vertex
   */
  void setCellPointIds(usize tetId, nonstd::span<usize> vertexIds);

  /**
   * @brief
   * @param tetId
   * @param vertexIds The index into the shared vertex list of each vertex
   */
  void getCellPointIds(usize tetId, nonstd::span<usize> vertexIds) const;

  /**
   * @brief
   * @param tetId
   * @param coords The coordinates of each vertex
   */
  void getCellCoordinates(usize tetId, nonstd::span<Point3Df> coords) const;

  /****************************************************************************
   * These functions get values related to where the Vertex Coordinates are
   * stored in the DataStructure
   */

  /**
   * @brief
   * @return
   */
  const std::optional<IdType>& getPolyhedronListId() const;

  void setPolyhedraDataId(const OptionalId& polyDataId);

  /**
   * @brief
   * @return
   */
  const std::optional<IdType>& getPolyhedraAttributeMatrixId() const;

  /**
   * @brief
   * @return
   */
  AttributeMatrix* getPolyhedraAttributeMatrix();

  /**
   * @brief
   * @return
   */
  const AttributeMatrix* getPolyhedraAttributeMatrix() const;

  /**
   * @brief
   * @return
   */
  AttributeMatrix& getPolyhedraAttributeMatrixRef();

  /**
   * @brief
   * @return
   */
  const AttributeMatrix& getPolyhedraAttributeMatrixRef() const;

  /**
   * @brief
   * @return
   */
  DataPath getPolyhedronAttributeMatrixDataPath() const;

  /**
   * @brief
   * @param attributeMatrix
   */
  void setPolyhedraAttributeMatrix(const AttributeMatrix& attributeMatrix);

  /**
   * @brief validates that linkages between shared node lists and their associated Attribute Matrix is correct.
   * @return A Result<> object possibly with error code and message.
   */
  Result<> validate() const override;

protected:
  INodeGeometry3D(DataStructure& dataStructure, std::string name);

  INodeGeometry3D(DataStructure& dataStructure, std::string name, IdType importId);

  SharedQuadList* createSharedQuadList(usize numQuads);

  SharedTriList* createSharedTriList(usize numTris);

  /**
   * @brief Updates the array IDs. Should only be called by DataObject::checkUpdatedIds.
   * @param updatedIds
   */
  void checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds) override;

  /* ***************************************************************************
   * These variables are the Ids of the arrays from the DataStructure object.
   */
  std::optional<IdType> m_PolyhedronListId;
  std::optional<IdType> m_PolyhedronAttributeMatrixId;
  std::optional<IdType> m_UnsharedFaceListId;
};
} // namespace nx::core
