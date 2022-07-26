#pragma once

#include "complex/DataStructure/Geometry/INodeGeometry0D.hpp"

namespace complex
{
class COMPLEX_EXPORT INodeGeometry1D : public INodeGeometry0D
{
public:
  static inline constexpr StringLiteral k_EdgeDataName = "Edge Data";

  ~INodeGeometry1D() noexcept override = default;

  const std::optional<IdType>& getEdgeListId() const
  {
    return m_EdgeListId;
  }

  SharedEdgeList* getEdges()
  {
    return getDataStructureRef().getDataAs<SharedEdgeList>(m_EdgeListId);
  }

  const SharedEdgeList* getEdges() const
  {
    return getDataStructureRef().getDataAs<SharedEdgeList>(m_EdgeListId);
  }

  SharedEdgeList& getEdgesRef()
  {
    return getDataStructureRef().getDataRefAs<SharedEdgeList>(m_EdgeListId.value());
  }

  const SharedEdgeList& getEdgesRef() const
  {
    return getDataStructureRef().getDataRefAs<SharedEdgeList>(m_EdgeListId.value());
  }

  void setEdges(const SharedEdgeList& edges)
  {
    m_EdgeListId = edges.getId();
  }

  /**
   * @brief Resizes the edge list to the target size.
   * @param size
   */
  void resizeEdgeList(usize size)
  {
    getEdgesRef().getIDataStoreRef().reshapeTuples({size});
  }

  /**
   * @brief Returns the number of edges in the geometry.
   * @return usize
   */
  usize getNumberOfEdges() const
  {
    const auto& edges = getEdgesRef();
    return edges.getNumberOfTuples();
  }

  /**
   * @brief Returns the vertex coordinates for a specified edge by reference.
   * This method does nothing if the edge list could not be found.
   * @param edgeId
   * @param vert1
   * @param vert2
   */
  virtual void getVertCoordsAtEdge(usize edgeId, Point3D<float32>& vert1, Point3D<float32>& vert2) const = 0;

  /**
   * @brief
   * @return StatusCode
   */
  virtual StatusCode findElementsContainingVert() = 0;

  /**
   * @brief
   * @return const ElementDynamicList*
   */
  const ElementDynamicList* getElementsContainingVert() const
  {
    return getDataStructureRef().getDataAs<ElementDynamicList>(m_ElementContainingVertId);
  }

  /**
   * @brief
   */
  void deleteElementsContainingVert()
  {
    getDataStructureRef().removeData(m_ElementContainingVertId);
    m_ElementContainingVertId.reset();
  }

  /**
   * @brief
   * @return StatusCode
   */
  virtual StatusCode findElementNeighbors() = 0;

  /**
   * @brief
   * @return const ElementDynamicList*
   */
  const ElementDynamicList* getElementNeighbors() const
  {
    return getDataStructureRef().getDataAs<ElementDynamicList>(m_ElementNeighborsId);
  }

  /**
   * @brief
   */
  void deleteElementNeighbors()
  {
    getDataStructureRef().removeData(m_ElementNeighborsId);
    m_ElementNeighborsId.reset();
  }

  /**
   * @brief
   * @return StatusCode
   */
  virtual StatusCode findElementCentroids() = 0;

  /**
   * @brief
   * @return const Float32Array*
   */
  const Float32Array* getElementCentroids() const
  {
    return getDataStructureRef().getDataAs<Float32Array>(m_ElementCentroidsId);
  }

  /**
   * @brief
   */
  void deleteElementCentroids()
  {
    getDataStructureRef().removeData(m_ElementCentroidsId);
    m_ElementCentroidsId.reset();
  }

  /**
   * @brief Sets the vertex IDs making up the specified edge. This method does
   * nothing if the edge list could not be found.
   * @param edgeId
   * @param verts
   */
  virtual void setVertsAtEdge(usize edgeId, const usize verts[2]) = 0;

  /**
   * @brief Returns the vertices that make up the specified edge by reference.
   * This method does nothing if the edge list could not be found.
   * @param edgeId
   * @param verts
   */
  virtual void getVertsAtEdge(usize edgeId, usize verts[2]) const = 0;

  /**
   * @brief
   * @return
   */
  const std::optional<IdType>& getEdgeDataId() const
  {
    return m_EdgeDataId;
  }

  /**
   * @brief
   * @return
   */
  AttributeMatrix* getEdgeData()
  {
    return getDataStructureRef().getDataAs<AttributeMatrix>(m_EdgeDataId);
  }

  /**
   * @brief
   * @return
   */
  const AttributeMatrix* getEdgeData() const
  {
    return getDataStructureRef().getDataAs<AttributeMatrix>(m_EdgeDataId);
  }

  /**
   * @brief
   * @return
   */
  AttributeMatrix& getEdgeDataRef()
  {
    return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_EdgeDataId.value());
  }

  /**
   * @brief
   * @return
   */
  const AttributeMatrix& getEdgeDataRef() const
  {
    return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_EdgeDataId.value());
  }

  /**
   * @brief
   * @return
   */
  DataPath getEdgeDataPath() const
  {
    return getEdgeDataRef().getDataPaths().at(0);
  }

  /**
   * @brief
   * @param attributeMatrix
   */
  void setEdgeData(const AttributeMatrix& attributeMatrix)
  {
    m_EdgeDataId = attributeMatrix.getId();
  }

  /**
   * @brief Reads values from HDF5
   * @param groupReader
   * @return H5::ErrorType
   */
  H5::ErrorType readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, bool preflight = false) override
  {
    H5::ErrorType error = INodeGeometry0D::readHdf5(dataStructureReader, groupReader, preflight);
    if(error < 0)
    {
      return error;
    }

    m_EdgeListId = ReadH5DataId(groupReader, H5Constants::k_EdgeListTag);
    m_EdgeDataId = ReadH5DataId(groupReader, H5Constants::k_EdgeDataTag);
    m_ElementContainingVertId = ReadH5DataId(groupReader, H5Constants::k_ElementContainingVertTag);
    m_ElementNeighborsId = ReadH5DataId(groupReader, H5Constants::k_ElementNeighborsTag);
    m_ElementCentroidsId = ReadH5DataId(groupReader, H5Constants::k_ElementCentroidTag);

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
    H5::ErrorType error = INodeGeometry0D::writeHdf5(dataStructureWriter, parentGroupWriter, importable);
    if(error < 0)
    {
      return error;
    }

    H5::GroupWriter groupWriter = parentGroupWriter.createGroupWriter(getName());
    error = WriteH5DataId(groupWriter, m_EdgeListId, H5Constants::k_EdgeListTag);
    if(error < 0)
    {
      return error;
    }

    error = WriteH5DataId(groupWriter, m_EdgeDataId, H5Constants::k_EdgeDataTag);
    if(error < 0)
    {
      return error;
    }

    error = WriteH5DataId(groupWriter, m_ElementContainingVertId, H5Constants::k_ElementContainingVertTag);
    if(error < 0)
    {
      return error;
    }

    error = WriteH5DataId(groupWriter, m_ElementNeighborsId, H5Constants::k_ElementNeighborsTag);
    if(error < 0)
    {
      return error;
    }

    error = WriteH5DataId(groupWriter, m_ElementCentroidsId, H5Constants::k_ElementCentroidTag);
    if(error < 0)
    {
      return error;
    }

    return error;
  }

protected:
  INodeGeometry1D() = delete;

  INodeGeometry1D(DataStructure& ds, std::string name)
  : INodeGeometry0D(ds, std::move(name))
  {
  }

  INodeGeometry1D(DataStructure& ds, std::string name, IdType importId)
  : INodeGeometry0D(ds, std::move(name), importId)
  {
  }

  INodeGeometry1D(const INodeGeometry1D&) = default;
  INodeGeometry1D(INodeGeometry1D&&) noexcept = default;

  INodeGeometry1D& operator=(const INodeGeometry1D&) = delete;
  INodeGeometry1D& operator=(INodeGeometry1D&&) noexcept = delete;

  /**
   * @brief Updates the array IDs. Should only be called by DataObject::checkUpdatedIds.
   * @param updatedIds
   */
  void checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds) override
  {
    INodeGeometry0D::checkUpdatedIdsImpl(updatedIds);

    for(const auto& updatedId : updatedIds)
    {
      if(m_VertexListId == updatedId.first)
      {
        m_VertexListId = updatedId.second;
      }

      if(m_EdgeDataId == updatedId.first)
      {
        m_EdgeDataId = updatedId.second;
      }

      if(m_EdgeListId == updatedId.first)
      {
        m_EdgeListId = updatedId.second;
      }

      if(m_ElementContainingVertId == updatedId.first)
      {
        m_ElementContainingVertId = updatedId.second;
      }

      if(m_ElementNeighborsId == updatedId.first)
      {
        m_ElementNeighborsId = updatedId.second;
      }

      if(m_ElementCentroidsId == updatedId.first)
      {
        m_ElementCentroidsId = updatedId.second;
      }

      if(m_ElementSizesId == updatedId.first)
      {
        m_ElementSizesId = updatedId.second;
      }
    }
  }

  std::optional<IdType> m_EdgeListId;
  std::optional<IdType> m_EdgeDataId;
  std::optional<IdType> m_ElementContainingVertId;
  std::optional<IdType> m_ElementNeighborsId;
  std::optional<IdType> m_ElementCentroidsId;
};
} // namespace complex
