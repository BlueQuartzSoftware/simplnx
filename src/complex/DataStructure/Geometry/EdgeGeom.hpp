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
   * @param ds
   * @param name
   * @param parentId = {}
   * @return EdgeGeom*
   */
  static EdgeGeom* Create(DataStructure& ds, std::string name, const std::optional<IdType>& parentId = {});

  /**
   * @brief
   * @param ds
   * @param name
   * @param importId
   * @param parentId = {}
   * @return EdgeGeom*
   */
  static EdgeGeom* Import(DataStructure& ds, std::string name, IdType importId, const std::optional<IdType>& parentId = {});

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

  EdgeGeom& operator=(const EdgeGeom&) = delete;
  EdgeGeom& operator=(EdgeGeom&&) noexcept = delete;

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return
   */
  DataObject::DataObjectType getType() const override;

  /**
   * @brief Returns typename of the DataObject as a std::string.
   * @return std::string
   */
  std::string getTypeName() const override;

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
  void resizeVertexList(usize newNumVertices);

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
  void setCoords(usize vertId, const complex::Point3D<float32>& coords);

  /**
   * @brief
   * @param vertId
   * @return Point3D<float32>
   */
  complex::Point3D<float32> getCoords(usize vertId) const;

  /**
   * @brief
   * @return usize
   */
  usize getNumberOfVertices() const;

  /**
   * @brief
   * @param newNumEdges
   */
  void resizeEdgeList(usize newNumEdges);

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
  void setVertsAtEdge(usize edgeId, usize verts[2]);

  /**
   * @brief
   * @param edgeId
   * @param verts
   */
  void getVertsAtEdge(usize edgeId, usize verts[2]);

  /**
   * @brief
   * @param edgeId
   * @param vert1
   * @param vert2
   */
  void getVertCoordsAtEdge(usize edgeId, complex::Point3D<float32>& vert1, complex::Point3D<float32>& vert2) const;

  /**
   * @brief
   * @return usize
   */
  usize getNumberOfEdges() const;

  /**
   * @brief
   */
  void initializeWithZeros() override;

  /**
   * @brief
   * @return usize
   */
  usize getNumberOfElements() const override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findElementSizes() override;

  /**
   * @brief
   * @return const Float32Array*
   */
  const Float32Array* getElementSizes() const override;

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
   * @return const Float32Array*
   */
  const Float32Array* getElementCentroids() const override;

  /**
   * @brief
   */
  void deleteElementCentroids() override;

  /**
   * @brief
   * @return complex::Point3D<float64>
   */
  complex::Point3D<float64> getParametricCenter() const override;

  /**
   * @brief
   * @param pCoords
   * @param shape
   */
  void getShapeFunctions(const complex::Point3D<float64>& pCoords, double* shape) const override;

  /**
   * @brief
   * @param field
   * @param derivatives
   * @param observable
   */
  void findDerivatives(Float64Array* field, Float64Array* derivatives, Observable* observable) const override;

  /**
   * @brief
   * @return complex::TooltipGenerator
   */
  complex::TooltipGenerator getTooltipGenerator() const override;

  /**
   * @brief
   * @return uint32
   */
  uint32 getXdmfGridType() const override;

  /**
   * @brief Reads values from HDF5
   * @param dataStructureReader
   * @param groupReader
   * @return H5::ErrorType
   */
  H5::ErrorType readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader) override;

  /**
   * @brief Writes the geometry to HDF5 using the provided parent group ID.
   * @param dataStructureWriter
   * @param parentGroupWriter
   * @return H5::ErrorType
   */
  H5::ErrorType writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter) const override;

protected:
  /**
   * @brief
   * @param ds
   * @param name
   * @param edges
   * @param vertices
   */
  EdgeGeom(DataStructure& ds, std::string name, const SharedEdgeList* edges, const SharedVertexList* vertices);

  /**
   * @brief
   * @param ds
   * @param name
   */
  EdgeGeom(DataStructure& ds, std::string name);

  /**
   * @brief
   * @param ds
   * @param name
   * @param importId
   */
  EdgeGeom(DataStructure& ds, std::string name, IdType importId);

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
  void setElementCentroids(const Float32Array* elementCentroids) override;

  /**
   * @brief
   * @param elementSizes
   */
  void setElementSizes(const Float32Array* elementSizes) override;

private:
  std::optional<IdType> m_VertexListId;
  std::optional<IdType> m_EdgeListId;
  std::optional<IdType> m_EdgesContainingVertId;
  std::optional<IdType> m_EdgeNeighborsId;
  std::optional<IdType> m_EdgeCentroidsId;
  std::optional<IdType> m_EdgeSizesId;
};
} // namespace complex
