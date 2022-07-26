#pragma once

#include "complex/DataStructure/Geometry/INodeGeometry2D.hpp"

namespace complex
{
class COMPLEX_EXPORT INodeGeometry3D : public INodeGeometry2D
{
public:
  static inline constexpr StringLiteral k_PolyhedronDataName = "Polyhedron Data";

  ~INodeGeometry3D() noexcept override = default;

  const std::optional<IdType>& getPolyhedronListId() const
  {
    return m_PolyhedronListId;
  }

  SharedFaceList* getPolyhedra()
  {
    return getDataStructureRef().getDataAs<SharedFaceList>(m_PolyhedronListId);
  }

  const SharedFaceList* getPolyhedra() const
  {
    return getDataStructureRef().getDataAs<SharedFaceList>(m_PolyhedronListId);
  }

  SharedFaceList& getPolyhedraRef()
  {
    return getDataStructureRef().getDataRefAs<SharedFaceList>(m_PolyhedronListId.value());
  }

  const SharedFaceList& getPolyhedraRef() const
  {
    return getDataStructureRef().getDataRefAs<SharedFaceList>(m_PolyhedronListId.value());
  }

  void setPolyhedra(const SharedFaceList& polyhedra)
  {
    m_PolyhedronListId = polyhedra.getId();
  }

  /**
   * @brief Resizes the polyhedra list to the target size.
   * @param size
   */
  void resizePolyhedronList(usize size)
  {
    getPolyhedraRef().getIDataStoreRef().reshapeTuples({size});
  }

  /**
   * @brief Returns the number of polyhedra in the geometry.
   * @return usize
   */
  usize getNumberOfPolyhedra() const
  {
    const auto& polyhedra = getPolyhedraRef();
    return polyhedra.getNumberOfTuples();
  }

  /**
   * @brief Creates and assigns the face list array for the current values.
   * @return StatusCode
   */
  virtual StatusCode findFaces() = 0;

  /**
   * @brief Deletes the current face list array.
   */
  void deleteFaces()
  {
    getDataStructureRef().removeData(m_FaceListId);
    m_FaceListId.reset();
  }

  /**
   * @brief
   * @return
   */
  const std::optional<IdType>& getUnsharedFacesId() const
  {
    return m_UnsharedFaceListId;
  }

  /**
   * @brief Creates and assigns the unshared face list array for the current values.
   */
  virtual StatusCode findUnsharedFaces() = 0;

  /**
   * @brief Returns a pointer to the unshared face list array.
   * @return
   */
  const SharedFaceList* getUnsharedFaces() const
  {
    return getDataStructureRef().getDataAs<SharedFaceList>(m_UnsharedFaceListId);
  }

  /**
   * @brief Deletes the current unshared face list array.
   */
  void deleteUnsharedFaces()
  {
    getDataStructureRef().removeData(m_UnsharedFaceListId);
    m_UnsharedFaceListId.reset();
  }

  /**
   * @brief Assigns a new coordinate to the target vertex.
   *
   * If the SharedVertexList has not been assigned or cannot be found, this
   * method does nothing. If the vertex index extends beyond the bounds of the
   * array, this method falls into undefined behavior.
   * @param vertId
   * @param coords
   */
  void setCoords(usize vertId, const Point3D<float32>& coords) override
  {
    auto& vertices = getVerticesRef();

    usize index = vertId * 3;
    for(usize i = 0; i < 3; i++)
    {
      vertices[index + i] = coords[i];
    }
  }

  /**
   * @brief Returns the 3D coordinates of the specified vertex as a Point3D<float32>.
   * @param vertId
   * @return Point3D<float32>
   */
  Point3D<float32> getCoords(usize vertId) const override
  {
    auto& vertices = getVerticesRef();

    usize index = vertId * 3;
    auto x = vertices[index];
    auto y = vertices[index + 1];
    auto z = vertices[index + 2];
    return Point3D<float32>(x, y, z);
  }

  /**
   * @brief Gets the vertex coordinates for the specified edge.
   * @param edgeId
   * @param vert1
   * @param vert2
   */
  void getVertCoordsAtEdge(usize edgeId, Point3D<float32>& vert1, Point3D<float32>& vert2) const override
  {
    auto& vertices = getVerticesRef();

    usize verts[2];
    getVertsAtEdge(edgeId, verts);

    for(usize i = 0; i < 3; i++)
    {
      vert1[i] = vertices[verts[0] * 3 + i];
      vert2[i] = vertices[verts[1] * 3 + i];
    }
  }

  /**
   * @brief
   * @return
   */
  const std::optional<IdType>& getPolyhedraDataId() const
  {
    return m_PolyhedronDataId;
  }

  /**
   * @brief
   * @return
   */
  AttributeMatrix* getPolyhedronData()
  {
    return getDataStructureRef().getDataAs<AttributeMatrix>(m_PolyhedronDataId);
  }

  /**
   * @brief
   * @return
   */
  const AttributeMatrix* getPolyhedronData() const
  {
    return getDataStructureRef().getDataAs<AttributeMatrix>(m_PolyhedronDataId);
  }

  /**
   * @brief
   * @return
   */
  AttributeMatrix& getPolyhedronDataRef()
  {
    return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_PolyhedronDataId.value());
  }

  /**
   * @brief
   * @return
   */
  const AttributeMatrix& getPolyhedronDataRef() const
  {
    return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_PolyhedronDataId.value());
  }

  /**
   * @brief
   * @return
   */
  DataPath getPolyhedronDataPath() const
  {
    return getPolyhedronDataRef().getDataPaths().at(0);
  }

  /**
   * @brief
   * @param attributeMatrix
   */
  void setPolyhedronData(const AttributeMatrix& attributeMatrix)
  {
    m_PolyhedronDataId = attributeMatrix.getId();
  }

  /**
   * @brief Reads values from HDF5
   * @param groupReader
   * @return H5::ErrorType
   */
  H5::ErrorType readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, bool preflight = false) override
  {
    H5::ErrorType error = INodeGeometry2D::readHdf5(dataStructureReader, groupReader, preflight);
    if(error < 0)
    {
      return error;
    }

    m_PolyhedronListId = ReadH5DataId(groupReader, H5Constants::k_PolyhedronListTag);
    m_PolyhedronDataId = ReadH5DataId(groupReader, H5Constants::k_PolyhedronDataTag);
    m_UnsharedFaceListId = ReadH5DataId(groupReader, H5Constants::k_UnsharedFaceListTag);

    return error;
  }

  /**
   * @brief Writes the geometry to HDF5 using the provided parent group ID.
   * @param dataStructureWriter
   * @param parentGroupWriter
   * @param importable
   * @return H5::ErrorType
   */
  H5::ErrorType writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter, bool importable) const override
  {
    H5::ErrorType error = INodeGeometry2D::writeHdf5(dataStructureWriter, parentGroupWriter, importable);
    if(error < 0)
    {
      return error;
    }

    H5::GroupWriter groupWriter = parentGroupWriter.createGroupWriter(getName());
    error = WriteH5DataId(groupWriter, m_PolyhedronListId, H5Constants::k_PolyhedronListTag);
    if(error < 0)
    {
      return error;
    }

    error = WriteH5DataId(groupWriter, m_PolyhedronDataId, H5Constants::k_PolyhedronDataTag);
    if(error < 0)
    {
      return error;
    }

    error = WriteH5DataId(groupWriter, m_UnsharedFaceListId, H5Constants::k_UnsharedFaceListTag);
    if(error < 0)
    {
      return error;
    }

    return error;
  }

protected:
  INodeGeometry3D() = delete;

  INodeGeometry3D(DataStructure& ds, std::string name)
  : INodeGeometry2D(ds, std::move(name))
  {
  }

  INodeGeometry3D(DataStructure& ds, std::string name, IdType importId)
  : INodeGeometry2D(ds, std::move(name), importId)
  {
  }

  INodeGeometry3D(const INodeGeometry3D&) = default;
  INodeGeometry3D(INodeGeometry3D&&) noexcept = default;

  INodeGeometry3D& operator=(const INodeGeometry3D&) = delete;
  INodeGeometry3D& operator=(INodeGeometry3D&&) noexcept = delete;

  SharedQuadList* createSharedQuadList(usize numQuads)
  {
    auto dataStore = std::make_unique<DataStore<MeshIndexType>>(std::vector<usize>{numQuads}, std::vector<usize>{4}, 0);
    SharedQuadList* quads = DataArray<MeshIndexType>::Create(*getDataStructure(), "Shared Quad List", std::move(dataStore), getId());
    return quads;
  }

  SharedTriList* createSharedTriList(usize numTris)
  {
    auto dataStore = std::make_unique<DataStore<MeshIndexType>>(std::vector<usize>{numTris}, std::vector<usize>{3}, 0);
    SharedTriList* triangles = DataArray<MeshIndexType>::Create(*getDataStructure(), "Shared Tri List", std::move(dataStore), getId());
    return triangles;
  }

  /**
   * @brief Updates the array IDs. Should only be called by DataObject::checkUpdatedIds.
   * @param updatedIds
   */
  void checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds) override
  {
    INodeGeometry2D::checkUpdatedIdsImpl(updatedIds);

    for(const auto& updatedId : updatedIds)
    {
      if(m_PolyhedronListId == updatedId.first)
      {
        m_PolyhedronListId = updatedId.second;
      }

      if(m_PolyhedronDataId == updatedId.first)
      {
        m_PolyhedronDataId = updatedId.second;
      }

      if(m_UnsharedFaceListId == updatedId.first)
      {
        m_UnsharedFaceListId = updatedId.second;
      }
    }
  }

  std::optional<IdType> m_PolyhedronListId;
  std::optional<IdType> m_PolyhedronDataId;
  std::optional<IdType> m_UnsharedFaceListId;
};
} // namespace complex
