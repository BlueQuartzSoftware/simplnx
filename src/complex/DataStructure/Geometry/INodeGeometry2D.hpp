#pragma once

#include "complex/DataStructure/Geometry/INodeGeometry1D.hpp"

namespace complex
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

class COMPLEX_EXPORT INodeGeometry2D : public INodeGeometry1D
{
public:
  static inline constexpr StringLiteral k_FaceDataName = "Face Data";
  static inline constexpr StringLiteral k_Edges = "Shared Edge List";
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
   * @brief
   * @return
   */
  virtual usize getNumberOfVerticesPerFace() const = 0;

  /**
   * @brief
   * @param triId
   * @param vertexIds The index into the shared vertex list of each vertex
   */
  void setFacePointIds(usize triId, nonstd::span<usize> vertexIds);

  /**
   * @brief
   * @param faceId
   * @param vertexIds The index into the shared vertex list of each vertex
   */
  void getFacePointIds(usize faceId, nonstd::span<usize> vertexIds) const;

  /**
   * @brief
   * @param faceId
   * @param coords The coordinates of each vertex
   */
  void getFaceCoordinates(usize faceId, nonstd::span<Point3Df> coords) const;

  /**
   * @brief
   * @return StatusCode
   */
  virtual StatusCode findEdges() = 0;

  /**
   * @brief Deletes the shared edge list and removes it from the DataStructure.
   */
  void deleteEdges();

  /**
   * @brief
   * @return
   */
  const std::optional<IdType>& getUnsharedEdgesId() const;

  /**
   * @brief
   * @return StatusCode
   */
  virtual StatusCode findUnsharedEdges() = 0;

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

  const std::optional<IdType>& getFaceListDataArrayId() const;

  /**
   * @brief
   * @return
   */
  const std::optional<IdType>& getFaceAttributeMatrixId() const;

  /**
   * @brief
   * @return
   */
  AttributeMatrix* getFaceAttributeMatrix();

  /**
   * @brief
   * @return
   */
  const AttributeMatrix* getFaceAttributeMatrix() const;

  /**
   * @brief
   * @return
   */
  AttributeMatrix& getFaceAttributeMatrixRef();

  /**
   * @brief
   * @return
   */
  const AttributeMatrix& getFaceAttributeMatrixRef() const;

  /**
   * @brief
   * @return
   */
  DataPath getFaceAttributeMatrixDataPath() const;

  /**
   * @brief
   * @param attributeMatrix
   */
  void setFaceAttributeMatrix(const AttributeMatrix& attributeMatrix);

  /**
   * @brief Reads values from HDF5
   * @param groupReader
   * @return H5::ErrorType
   */
  H5::ErrorType readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, bool preflight) override;

  /**
   * @brief Writes the geometry to HDF5 using the provided parent group ID.
   * @param dataStructureWriter
   * @param parentGroupWriter
   * @param importable
   * @return H5::ErrorType
   */
  H5::ErrorType writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter, bool importable) const override;

protected:
  INodeGeometry2D(DataStructure& ds, std::string name);

  INodeGeometry2D(DataStructure& ds, std::string name, IdType importId);

  /**
   * @brief
   * @param numEdges
   * @return SharedEdgeList*
   */
  SharedEdgeList* createSharedEdgeList(usize numEdges);

  /**
   * @brief Updates the array IDs. Should only be called by DataObject::checkUpdatedIds.
   * @param updatedIds
   */
  void checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds) override;

  /* ***************************************************************************
   * These variables are the Ids of the arrays from the complex::DataStructure object.
   */
  std::optional<IdType> m_FaceListId;
  std::optional<IdType> m_FaceAttributeMatrixId;
  std::optional<IdType> m_UnsharedEdgeListId;
};
} // namespace complex
