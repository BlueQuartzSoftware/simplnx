#pragma once

#include "simplnx/DataStructure/Geometry/AbstractNodeGeometry2D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry3D.hpp"

namespace nx::core
{
class SIMPLNX_EXPORT AbstractNodeGeometry3D : public AbstractNodeGeometry2D, public INodeGeometry3D
{
public:
  static constexpr StringLiteral k_TypeName = "INodeGeometry3D";

  AbstractNodeGeometry3D() = delete;
  AbstractNodeGeometry3D(const AbstractNodeGeometry3D&) = default;
  AbstractNodeGeometry3D(AbstractNodeGeometry3D&&) = default;

  AbstractNodeGeometry3D& operator=(const AbstractNodeGeometry3D&) = delete;
  AbstractNodeGeometry3D& operator=(AbstractNodeGeometry3D&&) noexcept = delete;

  ~AbstractNodeGeometry3D() noexcept override = default;

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
   * @return Result<>
   */
  Result<> findFaces(bool recalculate) override = 0;

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
  Result<> findUnsharedFaces(bool recalculate) override = 0;

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
  usize getNumberOfVerticesPerCell() const override = 0;

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
  AbstractNodeGeometry3D(DataStructure& dataStructure, std::string name);

  AbstractNodeGeometry3D(DataStructure& dataStructure, std::string name, IdType importId);

  SharedQuadList* createSharedQuadList(usize numQuads);

  SharedTriList* createSharedTriList(usize numTris);

  /**
   * @brief Updates the array IDs. Should only be called by AbstractDataObject::checkUpdatedIds.
   * @param updatedIdsMap
   */
  void checkUpdatedIdsImpl(const std::unordered_map<AbstractDataObject::IdType, AbstractDataObject::IdType>& updatedIdsMap) override;

  /* ***************************************************************************
   * These variables are the Ids of the arrays from the DataStructure object.
   */
  std::optional<IdType> m_PolyhedronListId;
  std::optional<IdType> m_PolyhedronAttributeMatrixId;
  std::optional<IdType> m_UnsharedFaceListId;
};
} // namespace nx::core
