#pragma once

#include "complex/Common/Point3D.hpp"
#include "complex/DataStructure/Geometry/AbstractGeometry.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class EdgeGeom
 * @brief
 */
class COMPLEX_EXPORT EdgeGeom : public AbstractGeometry
{
public:
  friend class DataStructure;

  /**
   * @brief
   * @param other
   */
  EdgeGeom(const EdgeGeom& other);

  /**
   * @brief
   * @param other
   */
  EdgeGeom(EdgeGeom&& other) noexcept;

  virtual ~EdgeGeom();

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
   * @param coords
   */
  void setCoords(size_t vertId, const complex::Point3D<float>& coords);

  /**
   * @brief
   * @param vertId
   * @return Point3D<float>
   */
  complex::Point3D<float> getCoords(size_t vertId) const;

  /**
   * @brief
   * @return size_t
   */
  size_t getNumberOfVertices() const;

  /**
   * @brief
   * @param newNumEdges
   */
  void resizeEdgeList(size_t newNumEdges);

  /**
   * @brief
   * @param edges
   */
  void setEdges(const SharedEdgeList* edges);

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
  void setVertsAtEdge(size_t edgeId, size_t verts[2]);

  /**
   * @brief
   * @param edgeId
   * @param verts
   */
  void getVertsAtEdge(size_t edgeId, size_t verts[2]);

  /**
   * @brief
   * @param edgeId
   * @param vert1
   * @param vert2
   */
  void getVertCoordsAtEdge(size_t edgeId, complex::Point3D<float>& vert1, complex::Point3D<float>& vert2) const;

  /**
   * @brief
   * @return size_t
   */
  size_t getNumberOfEdges() const;

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
   * @return uint32_t
   */
  uint32_t getXdmfGridType() const override;

protected:
  /**
   * @brief
   * @param ds
   * @param name
   * @param edges
   * @param vertices
   */
  EdgeGeom(DataStructure* ds, const std::string& name, const SharedEdgeList* edges, const SharedVertexList* vertices);

  /**
   * @brief
   * @param ds
   * @param name
   */
  EdgeGeom(DataStructure* ds, const std::string& name);

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

private:
  std::optional<DataObject::IdType> m_VertexListId;
  std::optional<DataObject::IdType> m_EdgeListId;
  std::optional<DataObject::IdType> m_EdgesContainingVertId;
  std::optional<DataObject::IdType> m_EdgeNeighborsId;
  std::optional<DataObject::IdType> m_EdgeCentroidsId;
  std::optional<DataObject::IdType> m_EdgeSizesId;
};
} // namespace complex
