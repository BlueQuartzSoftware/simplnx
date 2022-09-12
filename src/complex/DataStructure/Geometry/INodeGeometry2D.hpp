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

  ~INodeGeometry2D() noexcept override = default;

  const std::optional<IdType>& getFaceListId() const;

  SharedFaceList* getFaces();

  const SharedFaceList* getFaces() const;

  SharedFaceList& getFacesRef();

  const SharedFaceList& getFacesRef() const;

  void setFaces(const SharedFaceList& faces);

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

  /**
   * @brief Sets the vertex IDs making up the specified edge. This method does
   * nothing if the edge list could not be found.
   * @param edgeId
   * @param verts
   */
  void setVertsAtEdge(usize edgeId, const usize verts[2]) override;

  /**
   * @brief Returns the vertices that make up the specified edge by reference.
   * This method does nothing if the edge list could not be found.
   * @param edgeId
   * @param verts
   */
  void getVertsAtEdge(usize edgeId, usize verts[2]) const override;

  /**
   * @brief
   * @return
   */
  const std::optional<IdType>& getFaceDataId() const;

  /**
   * @brief
   * @return
   */
  AttributeMatrix* getFaceData();

  /**
   * @brief
   * @return
   */
  const AttributeMatrix* getFaceData() const;

  /**
   * @brief
   * @return
   */
  AttributeMatrix& getFaceDataRef();

  /**
   * @brief
   * @return
   */
  const AttributeMatrix& getFaceDataRef() const;

  /**
   * @brief
   * @return
   */
  DataPath getFaceDataPath() const;

  /**
   * @brief
   * @param attributeMatrix
   */
  void setFaceData(const AttributeMatrix& attributeMatrix);

  /**
   * @brief Reads values from HDF5
   * @param groupReader
   * @return H5::ErrorType
   */
  H5::ErrorType readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, bool preflight = false) override;

  /**
   * @brief Writes the geometry to HDF5 using the provided parent group ID.
   * @param dataStructureWriter
   * @param parentGroupWriter
   * @param importable
   * @return H5::ErrorType
   */
  H5::ErrorType writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter, bool importable) const override;

protected:
  INodeGeometry2D() = delete;

  INodeGeometry2D(DataStructure& ds, std::string name);

  INodeGeometry2D(DataStructure& ds, std::string name, IdType importId);

  INodeGeometry2D(const INodeGeometry2D&) = default;
  INodeGeometry2D(INodeGeometry2D&&) = default;

  INodeGeometry2D& operator=(const INodeGeometry2D&) = delete;
  INodeGeometry2D& operator=(INodeGeometry2D&&) noexcept = delete;

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

  std::optional<IdType> m_FaceListId;
  std::optional<IdType> m_FaceDataId;
  std::optional<IdType> m_UnsharedEdgeListId;
};
} // namespace complex
