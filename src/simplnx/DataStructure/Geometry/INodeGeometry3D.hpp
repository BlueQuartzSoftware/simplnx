#pragma once

#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"

namespace nx::core
{
class SIMPLNX_EXPORT INodeGeometry3D : public INodeGeometry2D
{
public:
  static inline constexpr StringLiteral k_PolyhedronDataName = "Polyhedron Data";
  static inline constexpr StringLiteral k_SharedPolyhedronListName = "Shared Polyhedron List";
  static inline constexpr StringLiteral k_TypeName = "INodeGeometry3D";

  INodeGeometry3D() = delete;
  INodeGeometry3D(const INodeGeometry3D&) = default;
  INodeGeometry3D(INodeGeometry3D&&) = default;

  INodeGeometry3D& operator=(const INodeGeometry3D&) = delete;
  INodeGeometry3D& operator=(INodeGeometry3D&&) noexcept = delete;

  ~INodeGeometry3D() noexcept override = default;

  /**
   * @brief Sets the ID of the polyhedron list array.
   * @param polyListId The optional ID of the polyhedron list array
   */
  void setPolyhedronListId(const OptionalId& polyListId);

  /**
   * @brief Returns a pointer to the polyhedron list array.
   * @return SharedFaceList* Pointer to the polyhedron list, or nullptr if not available
   */
  SharedFaceList* getPolyhedra();

  /**
   * @brief Returns a const pointer to the polyhedron list array.
   * @return const SharedFaceList* Const pointer to the polyhedron list, or nullptr if not available
   */
  const SharedFaceList* getPolyhedra() const;

  /**
   * @brief Returns a reference to the polyhedron list array.
   * @return SharedFaceList& Reference to the polyhedron list
   */
  SharedFaceList& getPolyhedraRef();

  /**
   * @brief Returns a const reference to the polyhedron list array.
   * @return const SharedFaceList& Const reference to the polyhedron list
   */
  const SharedFaceList& getPolyhedraRef() const;

  /**
   * @brief Sets the internal reference to the polyhedron list.
   * @param polyhedra The polyhedron list array to use
   */
  void setPolyhedraList(const SharedFaceList& polyhedra);

  /**
   * @brief Resizes the polyhedra list to the target size.
   * @param size
   */
  void resizePolyhedraList(usize size);

  /**
   * @brief Returns the optional ID of the polyhedra data AttributeMatrix.
   * @return OptionalId The polyhedra data ID if it exists
   */
  OptionalId getPolyhedraDataId() const;

  /**
   * @brief Returns the number of polyhedra in the geometry.
   * @return usize
   */
  usize getNumberOfPolyhedra() const;

  /**
   * @brief Pure-Virtual intended to find the shared faces of each element
   * in the geometry and store it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when a Shared Face
   * Array exists and recalculate is `false`
   * @return Result<>
   */
  virtual Result<> findFaces(bool recalculate) = 0;

  /**
   * @brief Deletes the current face list array.
   */
  void deleteFaces();

  /**
   * @brief Returns the optional ID of the unshared faces array.
   * @return const std::optional<IdType>& The unshared faces ID if it exists
   */
  const std::optional<IdType>& getUnsharedFacesId() const;

  /**
   * @brief Sets the ID of the unshared faces array.
   * @param id The optional ID of the unshared faces array
   */
  void setUnsharedFacedId(const OptionalId& id);

  /**
   * @brief Pure-Virtual intended to find the unshared faces of each element
   * in the geometry and store it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when an Unshared Faces
   * Array exists and recalculate is `false`
   * @return Result<>
   */
  virtual Result<> findUnsharedFaces(bool recalculate) = 0;

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
   * @return usize The number of vertices per cell
   */
  virtual usize getNumberOfVerticesPerCell() const = 0;

  /**
   * @brief Sets the vertex IDs making up the specified cell.
   * @param tetId The index of the cell to modify
   * @param vertexIds The index into the shared vertex list of each vertex
   */
  void setCellPointIds(usize tetId, nonstd::span<usize> vertexIds);

  /**
   * @brief Returns the vertices that make up the specified cell.
   * @param tetId The index of the cell to query
   * @param vertexIds The index into the shared vertex list of each vertex
   */
  void getCellPointIds(usize tetId, nonstd::span<usize> vertexIds) const;

  /**
   * @brief Returns the vertex coordinates for a specified cell.
   * @param tetId The index of the cell to query
   * @param coords The coordinates of each vertex
   */
  void getCellCoordinates(usize tetId, nonstd::span<Point3Df> coords) const;

  /****************************************************************************
   * These functions get values related to where the Vertex Coordinates are
   * stored in the DataStructure
   */

  /**
   * @brief Returns the DataStructure unique ID of the polyhedron list array.
   * @return const std::optional<IdType>& The polyhedron list array ID if it exists
   */
  const std::optional<IdType>& getPolyhedronListId() const;

  /**
   * @brief Sets the ID of the polyhedra data AttributeMatrix.
   * @param polyDataId The optional ID of the polyhedra data AttributeMatrix
   */
  void setPolyhedraDataId(const OptionalId& polyDataId);

  /**
   * @brief Returns the DataStructure unique ID of the polyhedra AttributeMatrix.
   * @return const std::optional<IdType>& The polyhedra AttributeMatrix ID if it exists
   */
  const std::optional<IdType>& getPolyhedraAttributeMatrixId() const;

  /**
   * @brief Returns pointer to the Attribute Matrix that holds data assigned to each polyhedron.
   * @return AttributeMatrix* Pointer to the polyhedra AttributeMatrix, or nullptr if not available
   */
  AttributeMatrix* getPolyhedraAttributeMatrix();

  /**
   * @brief Returns pointer to the Attribute Matrix that holds data assigned to each polyhedron.
   * @return const AttributeMatrix* Const pointer to the polyhedra AttributeMatrix, or nullptr if not available
   */
  const AttributeMatrix* getPolyhedraAttributeMatrix() const;

  /**
   * @brief Returns reference to the Attribute Matrix that holds data assigned to each polyhedron.
   * @return AttributeMatrix& Reference to the polyhedra AttributeMatrix
   */
  AttributeMatrix& getPolyhedraAttributeMatrixRef();

  /**
   * @brief Returns reference to the Attribute Matrix that holds data assigned to each polyhedron.
   * @return const AttributeMatrix& Const reference to the polyhedra AttributeMatrix
   */
  const AttributeMatrix& getPolyhedraAttributeMatrixRef() const;

  /**
   * @brief Returns the DataPath to the AttributeMatrix for the polyhedron data.
   * @return DataPath The path to the polyhedra AttributeMatrix
   */
  DataPath getPolyhedronAttributeMatrixDataPath() const;

  /**
   * @brief Sets the Attribute Matrix for the data assigned to the polyhedra.
   * @param attributeMatrix The AttributeMatrix to use for polyhedra data
   */
  void setPolyhedraAttributeMatrix(const AttributeMatrix& attributeMatrix);

  /**
   * @brief validates that linkages between shared node lists and their associated Attribute Matrix is correct.
   * @return A Result<> object possibly with error code and message.
   */
  Result<> validate() const override;

protected:
  /**
   * @brief Constructs an INodeGeometry3D with the specified name.
   * @param dataStructure The DataStructure this geometry belongs to
   * @param name The name for this geometry
   */
  INodeGeometry3D(DataStructure& dataStructure, std::string name);

  /**
   * @brief Constructs an INodeGeometry3D with the specified name and import ID.
   * @param dataStructure The DataStructure this geometry belongs to
   * @param name The name for this geometry
   * @param importId The ID to use for this imported object
   */
  INodeGeometry3D(DataStructure& dataStructure, std::string name, IdType importId);

  /**
   * @brief Creates a new shared quad list with the specified number of quads.
   * @param numQuads The number of quads to allocate space for
   * @return SharedQuadList* Pointer to the created shared quad list
   */
  SharedQuadList* createSharedQuadList(usize numQuads);

  /**
   * @brief Creates a new shared triangle list with the specified number of triangles.
   * @param numTris The number of triangles to allocate space for
   * @return SharedTriList* Pointer to the created shared triangle list
   */
  SharedTriList* createSharedTriList(usize numTris);

  /**
   * @brief Updates the array IDs. Should only be called by DataObject::checkUpdatedIds.
   * @param updatedIdsMap
   */
  void checkUpdatedIdsImpl(const std::unordered_map<DataObject::IdType, DataObject::IdType>& updatedIdsMap) override;

  /* ***************************************************************************
   * These variables are the Ids of the arrays from the DataStructure object.
   */
  std::optional<IdType> m_PolyhedronListId;
  std::optional<IdType> m_PolyhedronAttributeMatrixId;
  std::optional<IdType> m_UnsharedFaceListId;
};
} // namespace nx::core
