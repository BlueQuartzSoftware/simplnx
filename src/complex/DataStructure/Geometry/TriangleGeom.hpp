#pragma once

#include "complex/DataStructure/Geometry/AbstractGeometry2D.hpp"
#include "complex/Utilities/TooltipGenerator.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class TriangleGeom
 * @brief
 */
class COMPLEX_EXPORT TriangleGeom : public AbstractGeometry2D
{
public:
  friend class DataStructure;

  /**
   * @brief
   * @param other
   */
  TriangleGeom(const TriangleGeom& other);

  /**
   * @brief
   * @param other
   */
  TriangleGeom(TriangleGeom&& other) noexcept;

  virtual ~TriangleGeom();

  /**
   * @brief
   * @return DataObject*
   */
  DataObject* shallowCopy() override;

  /**
   * @brief
   * @return DataObject*
   */
  DataObject* deepCopy() override;

  /**
   * @brief
   * @return std::string
   */
  std::string getGeometryTypeAsString() const override;

  /**
   * @brief
   * @param newNumTris
   */
  void resizeTriList(size_t newNumTris);

  /**
   * @brief
   * @param triangles
   */
  void setTriangles(const SharedTriList* triangles);

  /**
   * @brief
   * @return SharedTriList*
   */
  SharedTriList* getTriangles();

  /**
   * @brief
   * @return const SharedTriList*
   */
  const SharedTriList* getTriangles() const;

  /**
   * @brief
   * @param triId
   * @param verts
   */
  void setVertsAtTri(size_t triId, size_t verts[3]);

  /**
   * @brief
   * @param triId
   * @param verts
   */
  void getVertsAtTri(size_t triId, size_t verts[3]) const;

  /**
   * @brief
   * @param triId
   * @param vert1
   * @param vert2
   * @param vert3
   */
  void getVertCoordsAtTri(size_t triId, Point3D<float>& vert1, Point3D<float>& vert2, Point3D<float>& vert3) const;

  /**
   * @brief
   * @return size_t
   */
  size_t getNumberOfTris() const;

  /**
   * @brief
   */
  void initializeWithZeros() override;

  /**
   * @brief
   * @return size_t
   */
  size_t getNumberOfElements() const override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findElementSizes() override;

  /**
   * @brief
   * @return const FloatArray*
   */
  const FloatArray* getElementSizes() const override;

  /**
   * @brief
   */
  void deleteElementSizes() override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findElementsContainingVert() override;

  /**
   * @brief
   * @return const ElementDynamicList*
   */
  const ElementDynamicList* getElementsContainingVert() const override;

  /**
   * @brief
   */
  void deleteElementsContainingVert() override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findElementNeighbors() override;

  /**
   * @brief
   * @return const ElementDynamicList*
   */
  const ElementDynamicList* getElementNeighbors() const override;

  /**
   * @brief
   */
  void deleteElementNeighbors() override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findElementCentroids() override;

  /**
   * @brief
   * @return const FloatArray*
   */
  const FloatArray* getElementCentroids() const override;

  /**
   * @brief
   */
  void deleteElementCentroids() override;

  /**
   * @brief
   * @param pCoords
   * @return complex::Point3D<double>
   */
  complex::Point3D<double> getParametricCenter() const override;

  /**
   * @brief
   * @param pCoords
   * @param shape
   */
  void getShapeFunctions(const complex::Point3D<double>& pCoords, double* shape) const override;

  /**
   * @brief
   * @param field
   * @param derivatives
   * @param observable
   */
  void findDerivatives(DoubleArray* field, DoubleArray* derivatives, Observable* observable) const override;

  /**
   * @brief
   * @return complex::TooltipGenerator
   */
  complex::TooltipGenerator getTooltipGenerator() const override;

  /**
   * @brief
   * @param newNumVertices
   */
  void resizeVertexList(size_t newNumVertices) override;

  /**
   * @brief
   * @param vertices
   */
  void setVertices(const SharedVertexList* vertices) override;

  /**
   * @brief
   * @return SharedVertexList*
   */
  SharedVertexList* getVertices() override;

  /**
   * @brief
   * @return const SharedVertexList*
   */
  const SharedVertexList* getVertices() const override;

  /**
   * @brief
   * @param vertId
   * @param coords
   */
  void setCoords(size_t vertId, const Point3D<float>& coords) override;

  /**
   * @brief
   * @param vertId
   */
  Point3D<float> getCoords(size_t vertId) const override;

  /**
   * @brief
   * @return size_t
   */
  size_t getNumberOfVertices() const override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findEdges() override;

  /**
   * @brief
   * @param newNumEdges
   */
  void resizeEdgeList(size_t newNumEdges) override;

  /**
   * @brief
   * @return SharedEdgeList*
   */
  SharedEdgeList* getEdges() override;

  /**
   * @brief
   * @return const SharedEdgeList*
   */
  const SharedEdgeList* getEdges() const override;

  /**
   * @brief
   */
  void deleteEdges() override;

  /**
   * @brief
   * @param edgeId
   * @param verts
   */
  void setVertsAtEdge(size_t edgeId, size_t verts[2]) override;

  /**
   * @brief
   * @param edgeId
   * @param verts
   */
  void getVertsAtEdge(size_t edgeId, size_t verts[2]) const override;

  /**
   * @brief
   * @param edgeId
   * @param vert1
   * @param vert2
   */
  void getVertCoordsAtEdge(size_t edgeId, Point3D<float>& vert1, Point3D<float>& vert2) const override;

  /**
   * @brief
   * @return size_t
   */
  size_t getNumberOfEdges() const override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findUnsharedEdges() override;

  /**
   * @brief
   * @return std::const SharedEdgeList*
   */
  const SharedEdgeList* getUnsharedEdges() const override;

  /**
   * @brief
   */
  void deleteUnsharedEdges() override;

  /**
   * @brief
   * @return uint32_t
   */
  uint32_t getXdmfGridType() const override;

  /**
   * @brief
   * @param out
   * @param hdfFileName
   * @return H5::ErrorType
   */
  H5::ErrorType generateXdmfText(std::ostream& out, const std::string& hdfFileName) const override;

  /**
   * @brief
   * @param in
   * @param hdfFileName
   * @return H5::ErrorType
   */
  H5::ErrorType readFromXdmfText(std::istream& in, const std::string& hdfFileName) override;

protected:
  /**
   * @brief
   * @param ds
   * @param name
   */
  TriangleGeom(DataStructure* ds, const std::string& name);

  /**
   * @brief
   * @param ds
   * @param name
   * @param numTriangles
   * @param vertices
   * @param allocate
   */
  TriangleGeom(DataStructure* ds, const std::string& name, size_t numTriangles, const SharedVertexList* vertices, bool allocate);

  /**
   * @brief
   * @param ds
   * @param name
   * @param triangles
   * @param vertices
   */
  TriangleGeom(DataStructure* ds, const std::string& name, const SharedTriList* triangles, const SharedVertexList* vertices);

  /**
   * @brief
   * @param elementsContainingVert
   */
  void setElementsContainingVert(const ElementDynamicList* elementsContainingVert) override;

  /**
   * @brief
   * @param elementNeighbors
   */
  void setElementNeighbors(const ElementDynamicList* elementNeighbors) override;

  /**
   * @brief
   * @param elementCentroids
   */
  void setElementCentroids(const FloatArray* elementCentroids) override;

  /**
   * @brief
   * @param elementSizes
   */
  void setElementSizes(const FloatArray* elementSizes) override;

  /**
   * @brief
   * @param edges
   */
  void setEdges(const SharedEdgeList* edges) override;

  /**
   * @brief
   * @param bEdgeList
   */
  void setUnsharedEdges(const SharedEdgeList* bEdgeList) override;

private:
  std::optional<DataObject::IdType> m_VertexListId;
  std::optional<DataObject::IdType> m_EdgeListId;
  std::optional<DataObject::IdType> m_UnsharedEdgeListId;
  std::optional<DataObject::IdType> m_TriListId;
  std::optional<DataObject::IdType> m_TrianglesContainingVertId;
  std::optional<DataObject::IdType> m_TriangleNeighborsId;
  std::optional<DataObject::IdType> m_TriangleCentroidsId;
  std::optional<DataObject::IdType> m_TriangleSizesId;
};
} // namespace complex
