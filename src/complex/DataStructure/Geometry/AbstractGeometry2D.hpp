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
  virtual void resizeVertexList(size_t newNumVertices) = 0;

  /**
   * @brief
   * @param vertices
   */
  virtual void setVertices(const SharedVertexList* vertices) = 0;

  /**
   * @brief
   * @return SharedVertexList*
   */
  virtual SharedVertexList* getVertices() = 0;

  /**
   * @brief
   * @return const SharedVertexList*
   */
  virtual const SharedVertexList* getVertices() const = 0;

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
  virtual SharedEdgeList* getEdges() = 0;

  /**
   * @brief
   * @return const SharedEdgeList*
   */
  virtual const SharedEdgeList* getEdges() const = 0;

  /**
   * @brief
   * @param edgeId
   * @param verts
   */
  virtual void setVertsAtEdge(size_t edgeId, size_t verts[2]) = 0;

  /**
   * @brief
   * @param edgeId
   * @param verts
   */
  virtual void getVertsAtEdge(size_t edgeId, size_t verts[2]) const = 0;

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
  virtual size_t getNumberOfEdges() const = 0;

  /**
   * @brief
   * @return StatusCode
   */
  virtual StatusCode findEdges() = 0;

  /**
   * @brief
   */
  virtual void deleteEdges() = 0;

  /**
   * @brief
   * @return StatusCode
   */
  virtual StatusCode findUnsharedEdges() = 0;

  /**
   * @brief
   * @return const SharedEdgeList*
   */
  virtual const SharedEdgeList* getUnsharedEdges() const = 0;

  /**
   * @brief
   */
  virtual void deleteUnsharedEdges() = 0;

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

  /**
   * @brief
   * @param edges
   */
  virtual void setEdges(const SharedEdgeList* edges) = 0;

  /**
   * @brief
   * @param bEdgeList
   */
  virtual void setUnsharedEdges(const SharedEdgeList* bEdgeList) = 0;

private:
};
} // namespace complex
