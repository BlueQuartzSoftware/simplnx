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
  ~INodeGeometry2D() noexcept override = default;

  const std::optional<IdType>& getFaceListId() const
  {
    return m_FaceListId;
  }

  SharedFaceList* getFaces()
  {
    return getDataStructureRef().getDataAs<SharedFaceList>(m_FaceListId);
  }

  const SharedFaceList* getFaces() const
  {
    return getDataStructureRef().getDataAs<SharedFaceList>(m_FaceListId);
  }

  SharedFaceList& getFacesRef()
  {
    return getDataStructureRef().getDataRefAs<SharedFaceList>(m_FaceListId.value());
  }

  const SharedFaceList& getFacesRef() const
  {
    return getDataStructureRef().getDataRefAs<SharedFaceList>(m_FaceListId.value());
  }

  void setFaces(const SharedFaceList& faces)
  {
    m_FaceListId = faces.getId();
  }

  /**
   * @brief Resizes the face list to the target size.
   * @param size
   */
  void resizeFaceList(usize size)
  {
    getFacesRef().getIDataStoreRef().reshapeTuples({size});
  }

  /**
   * @brief Returns the number of faces in the geometry.
   * @return usize
   */
  usize getNumberOfFaces() const
  {
    const auto& faces = getFacesRef();
    return faces.getNumberOfTuples();
  }

  /**
   * @brief
   * @return StatusCode
   */
  virtual StatusCode findEdges() = 0;

  /**
   * @brief Deletes the shared edge list and removes it from the DataStructure.
   */
  void deleteEdges()
  {
    getDataStructureRef().removeData(m_EdgeListId);
    m_EdgeListId.reset();
  }

  /**
   * @brief
   * @return
   */
  const std::optional<IdType>& getUnsharedEdgesId() const
  {
    return m_UnsharedEdgeListId;
  }

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
  const SharedEdgeList* getUnsharedEdges() const
  {
    return getDataStructureRef().getDataAs<SharedEdgeList>(m_UnsharedEdgeListId);
  }

  /**
   * @brief Deletes the unshared edge list and removes it from the DataStructure.
   */
  void deleteUnsharedEdges()
  {
    getDataStructureRef().removeData(m_UnsharedEdgeListId);
    m_UnsharedEdgeListId.reset();
  }

  /**
   * @brief Sets the vertex IDs making up the specified edge. This method does
   * nothing if the edge list could not be found.
   * @param edgeId
   * @param verts
   */
  void setVertsAtEdge(usize edgeId, const usize verts[2]) override
  {
    auto& edges = getEdgesRef();
    edges[edgeId * 2] = verts[0];
    edges[edgeId * 2 + 1] = verts[1];
  }

  /**
   * @brief Returns the vertices that make up the specified edge by reference.
   * This method does nothing if the edge list could not be found.
   * @param edgeId
   * @param verts
   */
  void getVertsAtEdge(usize edgeId, usize verts[2]) const override
  {
    auto& edges = getEdgesRef();
    verts[0] = edges.at(edgeId * 2);
    verts[1] = edges.at(edgeId * 2 + 1);
  }

protected:
  INodeGeometry2D() = delete;

  INodeGeometry2D(DataStructure& ds, std::string name)
  : INodeGeometry1D(ds, std::move(name))
  {
  }

  INodeGeometry2D(DataStructure& ds, std::string name, IdType importId)
  : INodeGeometry1D(ds, std::move(name), importId)
  {
  }

  INodeGeometry2D(const INodeGeometry2D&) = default;
  INodeGeometry2D(INodeGeometry2D&&) noexcept = default;

  INodeGeometry2D& operator=(const INodeGeometry2D&) = delete;
  INodeGeometry2D& operator=(INodeGeometry2D&&) noexcept = delete;

  /**
   * @brief
   * @param numEdges
   * @return SharedEdgeList*
   */
  SharedEdgeList* createSharedEdgeList(usize numEdges)
  {
    auto dataStore = std::make_unique<DataStore<MeshIndexType>>(std::vector<usize>{numEdges}, std::vector<usize>{2}, 0);
    SharedEdgeList* edges = DataArray<MeshIndexType>::Create(*getDataStructure(), "Shared Edge List", std::move(dataStore), getId());
    return edges;
  }

  /**
   * @brief Updates the array IDs. Should only be called by DataObject::checkUpdatedIds.
   * @param updatedIds
   */
  void checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds) override
  {
    INodeGeometry1D::checkUpdatedIdsImpl(updatedIds);

    for(const auto& updatedId : updatedIds)
    {
      if(m_FaceListId == updatedId.first)
      {
        m_FaceListId = updatedId.second;
      }

      if(m_UnsharedEdgeListId == updatedId.first)
      {
        m_UnsharedEdgeListId = updatedId.second;
      }
    }
  }

  /**
   * @brief Reads the DataStructure group from a target HDF5 group.
   * @param dataStructureReader
   * @param groupReader
   * @return H5::Error
   */
  //H5::ErrorType readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, bool preflight) override
  //{
  //}

  /**
   * @brief Writes the contained DataObjects to the target HDF5 group.
   * @param parentGroupWriter
   * @param importable
   * @return H5::ErrorType
   */
  //H5::ErrorType writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter, bool importable) const override
  //{
  //  auto errorCode = INodeGeometry1D::writeHdf5(dataStructureWriter, parentGroupWriter, importable);
  //  if(errorCode < 0)
  //  {
  //    return errorCode;
  //  }

  //  auto groupWriter = parentGroupWriter.createGroupWriter(getName());

  //  errorCode = WriteH5DataId(groupWriter, m_FaceListId, H5Constants::k_TriangleListTag);
  //  if(errorCode < 0)
  //  {
  //    return errorCode;
  //  }

  //  errorCode = WriteH5DataId(groupWriter, m_UnsharedEdgeListId, H5Constants::k_TriangleListTag);
  //  if(errorCode < 0)
  //  {
  //    return errorCode;
  //  }

  //  return errorCode;
  //}

  std::optional<IdType> m_FaceListId;
  std::optional<IdType> m_UnsharedEdgeListId;
};
} // namespace complex
