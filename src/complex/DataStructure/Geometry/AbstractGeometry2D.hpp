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
   * @brief Resizes the vertex list with the new number of vertices.
   * @param newNumVertices
   */
  void resizeVertexList(size_t newNumVertices);

  /**
   * @brief Sets a new SharedVertexList, replacing any previous list.
   * @param vertices
   */
  void setVertices(const SharedVertexList* vertices);

  /**
   * @brief Returns a pointer to the SharedVertexList. Returns nullptr if none
   * are available.
   * @return SharedVertexList*
   */
  SharedVertexList* getVertices();

  /**
   * @brief Returns a const pointer to the SharedVertexList. Returns nullptr
   * if none are available.
   * @return const SharedVertexList*
   */
  const SharedVertexList* getVertices() const;

  /**
   * @brief Returns the Point3D<float> coordinate for the specified vertex ID.
   * @param vertId
   * @return Point3D<float>
   */
  virtual Point3D<float> getCoords(size_t vertId) const = 0;

  /**
   * @brief Sets the coordinates for the specified vertex ID.
   * @param vertId
   * @param coords
   */
  virtual void setCoords(size_t vertId, const Point3D<float>& coords) = 0;

  /**
   * @brief Returns the number of vertices in the geometry.
   * @return size_t
   */
  virtual size_t getNumberOfVertices() const = 0;

  /**
   * @brief Resizes the edge list to match the specified number of edges.
   * @param newNumEdges
   */
  virtual void resizeEdgeList(size_t newNumEdges) = 0;

  /**
   * @brief Returns a pointer to the SharedEdgeList. Returns nullptr if no
   * edge list could be found.
   * @return SharedEdgeList*
   */
  SharedEdgeList* getEdges();

  /**
   * @brief Returns a const pointer to the SharedEdgeList. Returns nullptr if
   * no edge list could be found.
   * @return const SharedEdgeList*
   */
  const SharedEdgeList* getEdges() const;

  /**
   * @brief Sets the vertex IDs making up the specified edge. This method does
   * nothing if the edge list could not be found.
   * @param edgeId
   * @param verts
   */
  void setVertsAtEdge(size_t edgeId, const size_t verts[2]);

  /**
   * @brief Returns the vertices that make up the specified edge by reference.
   * This method does nothing if the edge list could not be found.
   * @param edgeId
   * @param verts
   */
  void getVertsAtEdge(size_t edgeId, size_t verts[2]) const;

  /**
   * @brief Returns the vertex coordinates for a specified edge by reference.
   * This method does nothing if the edge list could not be found.
   * @param edgeId
   * @param vert1
   * @param vert2
   */
  virtual void getVertCoordsAtEdge(size_t edgeId, Point3D<float>& vert1, Point3D<float>& vert2) const = 0;

  /**
   * @brief Returns the number of edges in the geometry. Returns 0 if no edge
   * list could be found.
   * @return size_t
   */
  size_t getNumberOfEdges() const;

  /**
   * @brief
   * @return StatusCode
   */
  virtual StatusCode findEdges() = 0;

  /**
   * @brief Deletes the shared edge list and removes it from the DataStructure.
   */
  void deleteEdges();

  /**
   * @brief
   * @return StatusCode
   */
  virtual StatusCode findUnsharedEdges() = 0;

  /**
   * @brief Returns a const pointer to the unshared edge list. Returns nullptr
   * if no unshared edge list could be found.
   * @return const SharedEdgeList*
   */
  const SharedEdgeList* getUnsharedEdges() const;

  /**
   * @brief Deletes the unshared edge list and removes it from the DataStructure.
   */
  void deleteUnsharedEdges();

  /**
   * @brief Sets the shared edge list, replacing any existing shared edge list.
   * @param edges
   */
  void setEdges(const SharedEdgeList* edges);

  /**
   * @brief Sets the unshared edge list, replacing any existing unshared edge
   * list.
   * @param bEdgeList
   */
  void setUnsharedEdges(const SharedEdgeList* bEdgeList);

protected:
  /**
   * @brief
   * @param ds
   * @param name
   */
  AbstractGeometry2D(DataStructure& ds, const std::string& name);

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
