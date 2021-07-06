
#pragma once

#include "complex/DataStructure/Geometry/AbstractGeometry.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class AbstractGeometry3D
 * @brief
 */
class COMPLEX_EXPORT AbstractGeometry3D : public AbstractGeometry
{
public:
  virtual ~AbstractGeometry3D();

  /**
   * @brief resizeVertexList
   * @param newNumVertices
   */
  virtual void resizeVertexList(size_t newNumVertices) = 0;

  /**
   * @brief setVertices
   * @param vertices
   */
  virtual void setVertices(const SharedVertexList* vertices) = 0;

  /**
   * @brief getVertices
   * @return SharedVertexList*
   */
  virtual SharedVertexList* getVertices() = 0;

  /**
   * @brief getVertices
   * @return const SharedVertexList*
   */
  virtual const SharedVertexList* getVertices() const = 0;

  /**
   * @brief setCoords
   * @param vertId
   * @param coords
   */
  virtual void setCoords(size_t vertId, const complex::Point3D<float>& coords) = 0;

  /**
   * @brief getCoords
   * @param vertId
   * @return complex::Point3D<float>
   */
  virtual complex::Point3D<float> getCoords(size_t vertId) const = 0;

  /**
   * @brief getNumberOfVertices
   * @return
   */
  virtual size_t getNumberOfVertices() const = 0;

  // -----------------------------------------------------------------------------
  // Inherited from SharedEdgeOps
  // -----------------------------------------------------------------------------

  /**
   * @brief resizeEdgeList
   * @param newNumEdges
   */
  virtual void resizeEdgeList(size_t newNumEdges) = 0;

  /**
   * @brief getEdges
   * @return SharedEdgeList*
   */
  virtual SharedEdgeList* getEdges() = 0;

  /**
   * @brief getEdges
   * @return const SharedEdgeList*
   */
  virtual const SharedEdgeList* getEdges() const = 0;

  /**
   * @brief setVerts
   * @param edgeId
   * @param verts
   */
  virtual void setVertsAtEdge(size_t edgeId, size_t verts[2]) = 0;

  /**
   * @brief getVerts
   * @param edgeId
   * @param verts
   */
  virtual void getVertsAtEdge(size_t edgeId, size_t verts[2]) const = 0;

  /**
   * @brief getVertCoordsAtEdge
   * @param edgeId
   * @param vert1
   * @param vert2
   */
  virtual void getVertCoordsAtEdge(size_t edgeId, complex::Point3D<float>& vert1, complex::Point3D<float>& vert2) const = 0;

  /**
   * @brief getNumberOfEdges
   * @return
   */
  virtual size_t getNumberOfEdges() const = 0;

  // -----------------------------------------------------------------------------
  // Connectivity
  // -----------------------------------------------------------------------------

  /**
   * @brief findElementEdges
   * @return
   */
  virtual int findEdges() = 0;

  /**
   * @brief deleteElementEdges
   */
  virtual void deleteEdges() = 0;

  /**
   * @brief findFaces
   * @return
   */
  virtual int findFaces() = 0;

  /**
   * @brief deleteFaces
   */
  virtual void deleteFaces() = 0;

  // -----------------------------------------------------------------------------
  // Topology
  // -----------------------------------------------------------------------------

  /**
   * @brief findUnsharedEdges
   */
  virtual int findUnsharedEdges() = 0;

  /**
   * @brief getUnsharedEdges
   * @return
   */
  virtual const SharedEdgeList* getUnsharedEdges() const = 0;

  /**
   * @brief deleteUnsharedEdges
   */
  virtual void deleteUnsharedEdges() = 0;

  /**
   * @brief findUnsharedFaces
   */
  virtual int findUnsharedFaces() = 0;

  /**
   * @brief getUnsharedFaces
   * @return
   */
  virtual const SharedEdgeList* getUnsharedFaces() const = 0;

  /**
   * @brief deleteUnsharedFaces
   */
  virtual void deleteUnsharedFaces() = 0;

protected:
  /**
   * @brief
   * @param ds
   * @param name
   */
  AbstractGeometry3D(DataStructure* ds, const std::string& name);

  /**
   * @brief
   * @param other
   */
  AbstractGeometry3D(const AbstractGeometry3D& other);

  /**
   * @brief
   * @param other
   */
  AbstractGeometry3D(AbstractGeometry3D&& other) noexcept;

  /**
   * @brief setEdges
   * @param edges
   */
  virtual void setEdges(const SharedEdgeList* edges) = 0;

  /**
   * @brief setUnsharedEdges
   * @param bEdgeList
   */
  virtual void setUnsharedEdges(const SharedEdgeList* bEdgeList) = 0;

  /**
   * @brief setUnsharedFaces
   * @param bFaceList
   */
  virtual void setUnsharedFaces(const SharedFaceList* bFaceList) = 0;

  /**
   * @brief
   * @param numQuads
   * @return SharedQuadList*
   */
  SharedQuadList* createSharedQuadList(size_t numQuads);

  /**
   * @brief
   * @param numTris
   * @return SharedTriList*
   */
  SharedTriList* createSharedTriList(size_t numTris);

private:
};
} // namespace complex
