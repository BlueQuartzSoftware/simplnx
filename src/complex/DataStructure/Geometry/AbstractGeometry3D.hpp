
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
  void resizeVertexList(size_t newNumVertices);

  /**
   * @brief setVertices
   * @param vertices
   */
  void setVertices(const SharedVertexList* vertices);

  /**
   * @brief Returns the SharedVertexList array pointer. If the geometry does
   * not include a SharedVertexList, this returns nullptr.
   * @return SharedVertexList*
   */
  SharedVertexList* getVertices();

  /**
   * @brief Returns the SharedVertexList array pointer. If the geometry does
   * not include a SharedVertexList, this returns nullptr.
   * @return const SharedVertexList*
   */
  const SharedVertexList* getVertices() const;

  /**
   * @brief Assigns a new coordinate to the target vertex.
   *
   * If the SharedVertexList has not been assigned or cannot be found, this
   * method does nothing. If the vertex index extends beyond the bounds of the
   * array, this method falls into undefined behavior.
   * @param vertId
   * @param coords
   */
  void setCoords(size_t vertId, const complex::Point3D<float>& coords);

  /**
   * @brief Returns the 3D coordinates of the specified vertex as a Point3D<float>.
   * @param vertId
   * @return complex::Point3D<float>
   */
  complex::Point3D<float> getCoords(size_t vertId) const;

  /**
   * @brief Returns the number of vertices in the SharedVertexList.
   * @return size_t
   */
  size_t getNumberOfVertices() const;

  /**
   * @brief Resizes the SharedEdgeList array.
   *
   * This method does nothing if the geometry does not contain a valid shared
   * edge list.
   * @param newNumEdges
   */
  void resizeEdgeList(size_t newNumEdges);

  /**
   * @brief Returns the geometry's edge list array. Returns nullptr if the
   * array has not been set.
   * @return SharedEdgeList*
   */
  SharedEdgeList* getEdges();

  /**
   * @brief Returns the geometry's edge list array. Returns nullptr if the
   * array has not been set.
   * @return const SharedEdgeList*
   */
  const SharedEdgeList* getEdges() const;

  /**
   * @brief Sets the vertex IDs for the specified edge.
   * @param edgeId
   * @param verts
   */
  void setVertsAtEdge(size_t edgeId, const size_t verts[2]);

  /**
   * @brief Gets the vertex IDs for the specified edge.
   * @param edgeId
   * @param verts
   */
  void getVertsAtEdge(size_t edgeId, size_t verts[2]) const;

  /**
   * @brief Gets the vertex coordinates for the specified edge.
   * @param edgeId
   * @param vert1
   * @param vert2
   */
  void getVertCoordsAtEdge(size_t edgeId, complex::Point3D<float>& vert1, complex::Point3D<float>& vert2) const;

  /**
   * @brief Returns the number of edges that make up the edge list.
   * @return size_t
   */
  size_t getNumberOfEdges() const;

  /**
   * @brief Creates and assigns the edge list array for the current values.
   * @return StatusCode
   */
  virtual StatusCode findEdges() = 0;

  /**
   * @brief Deletes the current edge list array.
   */
  void deleteEdges();

  /**
   * @brief Creates and assigns the face list array for the current values.
   * @return StatusCode
   */
  virtual StatusCode findFaces() = 0;

  /**
   * @brief Deletes the current face list array.
   */
  void deleteFaces();

  /**
   * @brief Creates and assigns the unshared edge list array for the current values.
   */
  virtual StatusCode findUnsharedEdges() = 0;

  /**
   * @brief Returns the current unshared edge list array.
   * @return const SharedEdgeList*
   */
  const SharedEdgeList* getUnsharedEdges() const;

  /**
   * @brief Deletes the current unshared edge list array.
   */
  void deleteUnsharedEdges();

  /**
   * @brief Creates and assigns the unshared face list array for the current values.
   */
  virtual int findUnsharedFaces() = 0;

  /**
   * @brief Returns a pointer to the unshared face list array.
   * @return const SharedEdgeList*
   */
  const SharedEdgeList* getUnsharedFaces() const;

  /**
   * @brief Deletes the current unshared face list array.
   */
  void deleteUnsharedFaces();

  /**
   * @brief
   * @return size_t
   */
  size_t getNumberOfFaces() const;

  /**
   * @brief
   * @return SharedFaceList*
   */
  SharedFaceList* getFaces();

  /**
   * @brief
   * @return const SharedFaceList*
   */
  const SharedFaceList* getFaces() const;

  /**
   * @brief
   * @param faces
   */
  void setFaces(const SharedFaceList* faces);

  /**
   * @brief Sets the geometry's shared edge list array. This does not change the parentage of the provided array.
   * @param edges
   */
  void setEdges(const SharedEdgeList* edges);

  /**
   * @brief Sets the geometry's unshared edge list array.
   * @param bEdgeList
   */
  void setUnsharedEdges(const SharedEdgeList* bEdgeList);

  /**
   * @brief Sets the geometry's unshared face list array.
   * @param bFaceList
   */
  void setUnsharedFaces(const SharedFaceList* bFaceList);

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
  std::optional<DataObject::IdType> m_VertexListId;
  std::optional<DataObject::IdType> m_EdgeListId;
  std::optional<DataObject::IdType> m_UnsharedEdgeListId;
  std::optional<DataObject::IdType> m_FaceListId;
  std::optional<DataObject::IdType> m_UnsharedFaceListId;
};
} // namespace complex
