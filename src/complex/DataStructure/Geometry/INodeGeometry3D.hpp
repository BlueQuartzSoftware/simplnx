#pragma once

#include "complex/DataStructure/Geometry/INodeGeometry2D.hpp"

namespace complex
{
class COMPLEX_EXPORT INodeGeometry3D : public INodeGeometry2D
{
public:
  ~INodeGeometry3D() noexcept override = default;

  const std::optional<IdType>& getPolyhedraListId() const
  {
    return m_PolyhedraListId;
  }

  SharedFaceList* getPolyhedra()
  {
    return getDataStructureRef().getDataAs<SharedFaceList>(m_PolyhedraListId);
  }

  const SharedFaceList* getPolyhedra() const
  {
    return getDataStructureRef().getDataAs<SharedFaceList>(m_PolyhedraListId);
  }

  SharedFaceList& getPolyhedraRef()
  {
    return getDataStructureRef().getDataRefAs<SharedFaceList>(m_PolyhedraListId.value());
  }

  const SharedFaceList& getPolyhedraRef() const
  {
    return getDataStructureRef().getDataRefAs<SharedFaceList>(m_PolyhedraListId.value());
  }

  void setPolyhedra(const SharedFaceList& polyhedra)
  {
    m_PolyhedraListId = polyhedra.getId();
  }

  /**
   * @brief Resizes the polyhedra list to the target size.
   * @param size
   */
  void resizePolyhedraList(usize size)
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
      if(m_PolyhedraListId == updatedId.first)
      {
        m_PolyhedraListId = updatedId.second;
      }

      if(m_UnsharedFaceListId == updatedId.first)
      {
        m_UnsharedFaceListId = updatedId.second;
      }
    }
  }

  std::optional<IdType> m_PolyhedraListId;
  std::optional<IdType> m_UnsharedFaceListId;
};
} // namespace complex
