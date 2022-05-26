#pragma once

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Geometry/IGeometry.hpp"

namespace complex
{
class COMPLEX_EXPORT INodeGeometry0D : public IGeometry
{
public:
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

protected:
  INodeGeometry0D() = default;

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

  INodeGeometry0D& operator=(const INodeGeometry0D&) = default;
  INodeGeometry0D& operator=(INodeGeometry0D&&) noexcept = default;

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
    }
  }

  std::optional<IdType> m_VertexListId;
};
} // namespace complex
