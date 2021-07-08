
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
  /**
   * @brief Copy constructor
   * @param other
   */
  AbstractGeometry3D(const AbstractGeometry3D& other);

  /**
   * @brief Move constructor
   * @param other
   */
  AbstractGeometry3D(AbstractGeometry3D&& other) noexcept;

  virtual ~AbstractGeometry3D();

  /**
   * @brief Resizes the vertex array to the match the target number of vertices.
   * @param newNumVertices
   */
  virtual void resizeVertexList(size_t newNumVertices) = 0;

  /**
   * @brief setVertices
   * @param vertices
   */
  virtual void setVertices(const SharedVertexList* vertices) = 0;

  /**
   * @brief Returns the SharedVertexList array pointer. If the geometry does
   * not include a SharedVertexList, this returns nullptr.
   * @return SharedVertexList*
   */
  virtual SharedVertexList* getVertices() = 0;

  /**
   * @brief Returns the SharedVertexList array pointer. If the geometry does
   * not include a SharedVertexList, this returns nullptr.
   * @return const SharedVertexList*
   */
  virtual const SharedVertexList* getVertices() const = 0;

  /**
   * @brief Assigns a new coordinate to the target vertex.
   *
   * If the SharedVertexList has not been assigned or cannot be found, this
   * method does nothing. If the vertex index extends beyond the bounds of the
   * array, this method falls into undefined behavior.
   * @param vertId
   * @param coords
   */
  virtual void setCoords(size_t vertId, const complex::Point3D<float>& coords) = 0;

  /**
   * @brief Returns the 3D coordinates of the specified vertex as a Point3D<float>.
   * @param vertId
   * @return complex::Point3D<float>
   */
  virtual complex::Point3D<float> getCoords(size_t vertId) const = 0;

  /**
   * @brief Returns the number of vertices in the SharedVertexList.
   * @return size_t
   */
  virtual size_t getNumberOfVertices() const = 0;

  /**
   * @brief Resizes the SharedEdgeList array.
   *
   * This method does nothing if the geometry does not contain a valid shared
   * edge list.
   * @param newNumEdges
   */
  virtual void resizeEdgeList(size_t newNumEdges) = 0;

  /**
   * @brief Returns the geometry's edge list array. Returns nullptr if the
   * array has not been set.
   * @return SharedEdgeList*
   */
  virtual SharedEdgeList* getEdges() = 0;

  /**
   * @brief Returns the geometry's edge list array. Returns nullptr if the
   * array has not been set.
   * @return const SharedEdgeList*
   */
  virtual const SharedEdgeList* getEdges() const = 0;

  /**
   * @brief Sets the vertex IDs for the specified edge.
   * @param edgeId
   * @param verts
   */
  virtual void setVertsAtEdge(size_t edgeId, const size_t verts[2]) = 0;

  /**
   * @brief Gets the vertex IDs for the specified edge.
   * @param edgeId
   * @param verts
   */
  virtual void getVertsAtEdge(size_t edgeId, size_t verts[2]) const = 0;

  /**
   * @brief Gets the vertex coordinates for the specified edge.
   * @param edgeId
   * @param vert1
   * @param vert2
   */
  virtual void getVertCoordsAtEdge(size_t edgeId, complex::Point3D<float>& vert1, complex::Point3D<float>& vert2) const = 0;

  /**
   * @brief Returns the number of edges that make up the edge list.
   * @return size_t
   */
  virtual size_t getNumberOfEdges() const = 0;

  /**
   * @brief Creates and assigns the edge list array for the current values.
   * @return StatusCode
   */
  virtual StatusCode findEdges() = 0;

  /**
   * @brief Deletes the current edge list array.
   */
  virtual void deleteEdges() = 0;

  /**
   * @brief Creates and assigns the face list array for the current values.
   * @return StatusCode
   */
  virtual StatusCode findFaces() = 0;

  /**
   * @brief Deletes the current face list array.
   */
  virtual void deleteFaces() = 0;

  /**
   * @brief Creates and assigns the unshared edge list array for the current values.
   */
  virtual StatusCode findUnsharedEdges() = 0;

  /**
   * @brief Returns the current unshared edge list array.
   * @return SharedEdgeList*
   */
  virtual const SharedEdgeList* getUnsharedEdges() const = 0;

  /**
   * @brief Deletes the current unshared edge list array.
   */
  virtual void deleteUnsharedEdges() = 0;

  /**
   * @brief Creates and assigns the unshared face list array for the current values.
   */
  virtual int findUnsharedFaces() = 0;

  /**
   * @brief Returns a pointer to the unshared face list array.
   * @return
   */
  virtual const SharedEdgeList* getUnsharedFaces() const = 0;

  /**
   * @brief Deletes the current unshared face list array.
   */
  virtual void deleteUnsharedFaces() = 0;

  /**
   * @brief Sets the geometry's shared edge list array. This does not change the parentage of the provided array.
   * @param edges
   */
  virtual void setEdges(const SharedEdgeList* edges) = 0;

  /**
   * @brief Sets the geometry's unshared edge list array.
   * @param bEdgeList
   */
  virtual void setUnsharedEdges(const SharedEdgeList* bEdgeList) = 0;

  /**
   * @brief Sets the geometry's unshared face list array.
   * @param bFaceList
   */
  virtual void setUnsharedFaces(const SharedFaceList* bFaceList) = 0;

protected:
  /**
   * @brief
   * @param ds
   * @param name
   */
  AbstractGeometry3D(DataStructure* ds, const std::string& name);

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
