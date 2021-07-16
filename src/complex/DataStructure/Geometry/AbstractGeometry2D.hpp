#pragma once

#include "complex/DataStructure/Geometry/AbstractGeometry.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class AbstractGeometry2D
 * @brief
 */
class COMPLEX_EXPORT AbstractGeometry2D : public AbstractGeometry
{
public:
  /**
   * Empty Destructor
   */
  virtual ~AbstractGeometry2D();

  /**
   * @brief
   * @param newNumVertices
   */
  void resizeVertexList(size_t newNumVertices);

  /**
   * @brief
   * @param vertices
   */
  void setVertices(const SharedVertexList* vertices);

  /**
   * @brief
   * @return SharedVertexList*
   */
  SharedVertexList* getVertices();

  /**
   * @brief
   * @return const SharedVertexList*
   */
  const SharedVertexList* getVertices() const;

  /**
   * @brief
   * @param vertId
   * @return
   */
  virtual Point3D<float> getCoords(size_t vertId) const = 0;

  /**
   * @brief
   * @param vertId
   * @param coords
   */
  virtual void setCoords(size_t vertId, const Point3D<float>& coords) = 0;

  /**
   * @brief
   * @return size_t
   */
  virtual size_t getNumberOfVertices() const = 0;

  /**
   * @brief
   * @param newNumEdges
   */
  virtual void resizeEdgeList(size_t newNumEdges) = 0;

  /**
   * @brief
   * @return SharedEdgeList*
   */
  SharedEdgeList* getEdges();

  /**
   * @brief
   * @return const SharedEdgeList*
   */
  const SharedEdgeList* getEdges() const;

  /**
   * @brief
   * @param edgeId
   * @param verts
   */
  void setVertsAtEdge(size_t edgeId, const size_t verts[2]);

  /**
   * @brief
   * @param edgeId
   * @param verts
   */
  void getVertsAtEdge(size_t edgeId, size_t verts[2]) const;

  /**
   * @brief
   * @param edgeId
   * @param vert1
   * @param vert2
   */
  virtual void getVertCoordsAtEdge(size_t edgeId, Point3D<float>& vert1, Point3D<float>& vert2) const = 0;

  /**
   * @brief
   * @return size_t
   */
  size_t getNumberOfEdges() const;

  /**
   * @brief
   * @return StatusCode
   */
  virtual StatusCode findEdges() = 0;

  /**
   * @brief
   */
  void deleteEdges();

  /**
   * @brief
   * @return StatusCode
   */
  virtual StatusCode findUnsharedEdges() = 0;

  /**
   * @brief
   * @return const SharedEdgeList*
   */
  const SharedEdgeList* getUnsharedEdges() const;

  /**
   * @brief
   */
  void deleteUnsharedEdges();

  /**
   * @brief
   * @param edges
   */
  void setEdges(const SharedEdgeList* edges);

  /**
   * @brief
   * @param bEdgeList
   */
  void setUnsharedEdges(const SharedEdgeList* bEdgeList);

protected:
  /**
   * @brief
   * @param ds
   * @param name
   */
  AbstractGeometry2D(DataStructure* ds, const std::string& name);

  /**
   * @brief
   * @param other
   */
  AbstractGeometry2D(const AbstractGeometry2D& other);

  /**
   * @brief
   * @param other
   */
  AbstractGeometry2D(AbstractGeometry2D&& other) noexcept;

private:
  std::optional<DataObject::IdType> m_VertexListId;
  std::optional<DataObject::IdType> m_EdgeListId;
  std::optional<DataObject::IdType> m_UnsharedEdgeListId;
};
} // namespace complex
