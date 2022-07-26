#pragma once

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Geometry/IGeometry.hpp"

namespace complex
{
class COMPLEX_EXPORT INodeGeometry0D : public IGeometry
{
public:
  static inline constexpr StringLiteral k_VertexDataName = "Vertex Data";

  ~INodeGeometry0D() noexcept override = default;

  const std::optional<IdType>& getVertexListId() const
  {
    return m_VertexListId;
  }

  SharedVertexList* getVertices()
  {
    return getDataStructureRef().getDataAs<SharedVertexList>(m_VertexListId);
  }

  const SharedVertexList* getVertices() const
  {
    return getDataStructureRef().getDataAs<SharedVertexList>(m_VertexListId);
  }

  SharedVertexList& getVerticesRef()
  {
    return getDataStructureRef().getDataRefAs<SharedVertexList>(m_VertexListId.value());
  }

  const SharedVertexList& getVerticesRef() const
  {
    return getDataStructureRef().getDataRefAs<SharedVertexList>(m_VertexListId.value());
  }

  void setVertices(const SharedVertexList& vertices)
  {
    m_VertexListId = vertices.getId();
  }

  /**
   * @brief Resizes the vertex list to the target size.
   * @param size
   */
  void resizeVertexList(usize size)
  {
    getVerticesRef().getIDataStoreRef().reshapeTuples({size});
  }

  /**
   * @brief Returns the number of vertices in the geometry.
   * @return usize
   */
  usize getNumberOfVertices() const
  {
    const auto& vertices = getVerticesRef();
    return vertices.getNumberOfTuples();
  }

  /**
   * @brief Gets the coordinates at the target vertex ID.
   * @param vertId
   * @return
   */
  virtual Point3D<float32> getCoords(usize vertId) const = 0;

  /**
   * @brief Sets the coordinates for the specified vertex ID.
   * @param vertId
   * @param coords
   */
  virtual void setCoords(usize vertId, const Point3D<float32>& coords) = 0;

  /**
   * @brief
   * @return
   */
  const std::optional<IdType>& getVertexDataId() const
  {
    return m_VertexDataId;
  }

  /**
   * @brief
   * @return
   */
  AttributeMatrix* getVertexData()
  {
    return getDataStructureRef().getDataAs<AttributeMatrix>(m_VertexDataId);
  }

  /**
   * @brief
   * @return
   */
  const AttributeMatrix* getVertexData() const
  {
    return getDataStructureRef().getDataAs<AttributeMatrix>(m_VertexDataId);
  }

  /**
   * @brief
   * @return
   */
  AttributeMatrix& getVertexDataRef()
  {
    return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_VertexDataId.value());
  }

  /**
   * @brief
   * @return
   */
  const AttributeMatrix& getVertexDataRef() const
  {
    return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_VertexDataId.value());
  }

  /**
   * @brief
   * @return
   */
  DataPath getVertexDataPath() const
  {
    return getVertexDataRef().getDataPaths().at(0);
  }

  /**
   * @brief
   * @param attributeMatrix
   */
  void setVertexData(const AttributeMatrix& attributeMatrix)
  {
    m_VertexDataId = attributeMatrix.getId();
  }

  /**
   * @brief Reads values from HDF5
   * @param groupReader
   * @return H5::ErrorType
   */
  H5::ErrorType readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, bool preflight = false) override
  {
    H5::ErrorType error = IGeometry::readHdf5(dataStructureReader, groupReader, preflight);
    if(error < 0)
    {
      return error;
    }

    m_VertexListId = ReadH5DataId(groupReader, H5Constants::k_VertexListTag);
    m_VertexDataId = ReadH5DataId(groupReader, H5Constants::k_VertexDataTag);

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
    H5::ErrorType error = IGeometry::writeHdf5(dataStructureWriter, parentGroupWriter, importable);
    if(error < 0)
    {
      return error;
    }

    H5::GroupWriter groupWriter = parentGroupWriter.createGroupWriter(getName());

    error = WriteH5DataId(groupWriter, m_VertexListId, H5Constants::k_VertexListTag);
    if(error < 0)
    {
      return error;
    }

    error = WriteH5DataId(groupWriter, m_VertexDataId, H5Constants::k_VertexDataTag);
    if(error < 0)
    {
      return error;
    }

    return error;
  }

protected:
  INodeGeometry0D() = delete;

  INodeGeometry0D(DataStructure& ds, std::string name)
  : IGeometry(ds, std::move(name))
  {
  }

  INodeGeometry0D(DataStructure& ds, std::string name, IdType importId)
  : IGeometry(ds, std::move(name), importId)
  {
  }

  INodeGeometry0D(const INodeGeometry0D&) = default;
  INodeGeometry0D(INodeGeometry0D&&) noexcept = default;

  INodeGeometry0D& operator=(const INodeGeometry0D&) = delete;
  INodeGeometry0D& operator=(INodeGeometry0D&&) noexcept = delete;

  /**
   * @brief Updates the array IDs. Should only be called by DataObject::checkUpdatedIds.
   * @param updatedIds
   */
  void checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds) override
  {
    IGeometry::checkUpdatedIdsImpl(updatedIds);

    for(const auto& updatedId : updatedIds)
    {
      if(m_VertexListId == updatedId.first)
      {
        m_VertexListId = updatedId.second;
      }
      if(m_VertexDataId == updatedId.first)
      {
        m_VertexDataId = updatedId.second;
      }
    }
  }

  std::optional<IdType> m_VertexListId;
  std::optional<IdType> m_VertexDataId;
};
} // namespace complex
