#pragma once

#include "simplnx/DataStructure/Geometry/INodeGeometry1D.hpp"

namespace nx::core
{
namespace NodeType
{
inline constexpr int8_t Unused = 0;
inline constexpr int8_t Default = 2;
inline constexpr int8_t TriplePoint = 3;
inline constexpr int8_t QuadPoint = 4;
inline constexpr int8_t SurfaceDefault = 12;
inline constexpr int8_t SurfaceTriplePoint = 13;
inline constexpr int8_t SurfaceQuadPoint = 14;
} // namespace NodeType

class SIMPLNX_EXPORT INodeGeometry2D : public INodeGeometry1D
{
public:
  static inline constexpr StringLiteral k_FaceAttributeMatrixName = "Face Data";
  static inline constexpr StringLiteral k_FaceFeatureAttributeMatrixName = "Face Feature Data";
  static inline constexpr StringLiteral k_SharedFacesListName = "Shared Faces List";
  static inline constexpr StringLiteral k_TypeName = "INodeGeometry2D";

  INodeGeometry2D() = delete;
  INodeGeometry2D(const INodeGeometry2D&) = default;
  INodeGeometry2D(INodeGeometry2D&&) = default;

  INodeGeometry2D& operator=(const INodeGeometry2D&) = delete;
  INodeGeometry2D& operator=(INodeGeometry2D&&) noexcept = delete;

  ~INodeGeometry2D() noexcept override = default;

  /**
   * @brief Returns a pointer to the Face List
   * @return
   */
  SharedFaceList* getFaces();

  /**
   * @brief Returns a pointer to the Face List
   * @return
   */
  const SharedFaceList* getFaces() const;

  /**
   * @brief Returns a reference to the Face List
   * @return
   */
  SharedFaceList& getFacesRef();

  /**
   * @brief Returns a reference to the Face List
   * @return
   */
  const SharedFaceList& getFacesRef() const;

  /**
   * @brief Sets the list of Faces for this geometry
   * @param faces The new list of faces
   */
  void setFaceList(const SharedFaceList& faces);

  /**
   * @brief Returns the optional ID of the face list array.
   * @return OptionalId The face list ID if it exists
   */
  OptionalId getFaceListId() const;

  /**
   * @brief Sets the ID of the face list array.
   * @param facesId The optional ID of the face list array
   */
  void setFaceListId(const OptionalId& facesId);

  /**
   * @brief Resizes the face list to the target size.
   * @param size
   */
  void resizeFaceList(usize size);

  /**
   * @brief Returns the number of faces in the geometry.
   * @return usize
   */
  usize getNumberOfFaces() const;

  /**
   * @brief Returns the number of vertices that make up each face.
   * @return usize The number of vertices per face
   */
  virtual usize getNumberOfVerticesPerFace() const = 0;

  /**
   * @brief Sets the vertex IDs making up the specified face.
   * @param triId The index of the face to modify
   * @param vertexIds The index into the shared vertex list of each vertex
   */
  void setFacePointIds(usize triId, nonstd::span<usize> vertexIds);

  /**
   * @brief Returns the vertices that make up the specified face.
   * @param faceId The index of the face to query
   * @param vertexIds The index into the shared vertex list of each vertex
   */
  void getFacePointIds(usize faceId, nonstd::span<usize> vertexIds) const;

  /**
   * @brief Returns the vertex coordinates for a specified face.
   * @param faceId The index of the face to query
   * @param coords The coordinates of each vertex
   */
  void getFaceCoordinates(usize faceId, nonstd::span<Point3Df> coords) const;

  /**
   * @brief Pure-Virtual intended to find the shared edges of each element
   * in the geometry and store it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when an Edge
   * Array exists and recalculate is `false`
   * @return Result<>
   */
  virtual Result<> findEdges(bool recalculate) = 0;

  /**
   * @brief Deletes the shared edge list and removes it from the DataStructure.
   */
  void deleteEdges();

  /**
   * @brief Returns the optional ID of the unshared edges array.
   * @return const std::optional<IdType>& The unshared edges ID if it exists
   */
  const std::optional<IdType>& getUnsharedEdgesId() const;

  /**
   * @brief Sets the ID of the unshared edges array.
   * @param unsharedEdgesId The optional ID of the unshared edges array
   */
  void setUnsharedEdgesId(const OptionalId& unsharedEdgesId);

  /**
   * @brief Pure-Virtual intended to find the unshared edges of each element
   * in the geometry and store it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when an Unshared Edge
   * Array exists and recalculate is `false`
   * @return Result<>
   */
  virtual Result<> findUnsharedEdges(bool recalculate) = 0;

  /**
   * @brief Returns a const pointer to the unshared edge list. Returns nullptr
   * if no unshared edge list could be found.
   * @return const SharedEdgeList*
   */
  const SharedEdgeList* getUnsharedEdges() const;

  /**
   * @brief Deletes the unshared edge list and removes it from the DataStructure.
   */
  void deleteUnsharedEdges();

  /****************************************************************************
   * These functions get values related to where the Vertex Coordinates are
   * stored in the DataStructure
   */

  /**
   * @brief Returns the DataStructure unique ID of the face list array.
   * @return const std::optional<IdType>& The face list array ID if it exists
   */
  const std::optional<IdType>& getFaceListDataArrayId() const;

  /**
   * @brief Returns the DataStructure unique ID of the face AttributeMatrix.
   * @return const std::optional<IdType>& The face AttributeMatrix ID if it exists
   */
  const std::optional<IdType>& getFaceAttributeMatrixId() const;

  /**
   * @brief Sets the ID of the face data AttributeMatrix.
   * @param faceDataId The optional ID of the face data AttributeMatrix
   */
  void setFaceDataId(const OptionalId& faceDataId);

  /**
   * @brief Returns pointer to the Attribute Matrix that holds data assigned to each face.
   * @return AttributeMatrix* Pointer to the face AttributeMatrix, or nullptr if not available
   */
  AttributeMatrix* getFaceAttributeMatrix();

  /**
   * @brief Returns pointer to the Attribute Matrix that holds data assigned to each face.
   * @return const AttributeMatrix* Const pointer to the face AttributeMatrix, or nullptr if not available
   */
  const AttributeMatrix* getFaceAttributeMatrix() const;

  /**
   * @brief Returns reference to the Attribute Matrix that holds data assigned to each face.
   * @return AttributeMatrix& Reference to the face AttributeMatrix
   */
  AttributeMatrix& getFaceAttributeMatrixRef();

  /**
   * @brief Returns reference to the Attribute Matrix that holds data assigned to each face.
   * @return const AttributeMatrix& Const reference to the face AttributeMatrix
   */
  const AttributeMatrix& getFaceAttributeMatrixRef() const;

  /**
   * @brief Returns the DataPath to the AttributeMatrix for the face data.
   * @return DataPath The path to the face AttributeMatrix
   */
  DataPath getFaceAttributeMatrixDataPath() const;

  /**
   * @brief Sets the Attribute Matrix for the data assigned to the faces.
   * @param attributeMatrix The AttributeMatrix to use for face data
   */
  void setFaceAttributeMatrix(const AttributeMatrix& attributeMatrix);

  /**
   * @brief validates that linkages between shared node lists and their associated Attribute Matrix is correct.
   * @return A Result<> object possibly with error code and message.
   */
  Result<> validate() const override;

protected:
  /**
   * @brief Constructs an INodeGeometry2D with the specified name.
   * @param dataStructure The DataStructure this geometry belongs to
   * @param name The name for this geometry
   */
  INodeGeometry2D(DataStructure& dataStructure, std::string name);

  /**
   * @brief Constructs an INodeGeometry2D with the specified name and import ID.
   * @param dataStructure The DataStructure this geometry belongs to
   * @param name The name for this geometry
   * @param importId The ID to use for this imported object
   */
  INodeGeometry2D(DataStructure& dataStructure, std::string name, IdType importId);

  /**
   * @brief Creates a new shared edge list with the specified number of edges.
   * @param numEdges The number of edges to allocate space for
   * @return SharedEdgeList* Pointer to the created shared edge list
   */
  SharedEdgeList* createSharedEdgeList(usize numEdges);

  /**
   * @brief Updates the array IDs. Should only be called by DataObject::checkUpdatedIds.
   * @param updatedIdsMap
   */
  void checkUpdatedIdsImpl(const std::unordered_map<DataObject::IdType, DataObject::IdType>& updatedIdsMap) override;

  /* ***************************************************************************
   * These variables are the Ids of the arrays from the DataStructure object.
   */
  std::optional<IdType> m_FaceListId;
  std::optional<IdType> m_FaceAttributeMatrixId;
  std::optional<IdType> m_UnsharedEdgeListId;
};
} // namespace nx::core
