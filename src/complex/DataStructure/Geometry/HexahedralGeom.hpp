#pragma once

#include "complex/DataStructure/Geometry/AbstractGeometry3D.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class HexahedralGeom
 * @brief
 */
class COMPLEX_EXPORT HexahedralGeom : public AbstractGeometry3D
{
public:
  friend class DataStructure;

  /**
   * @brief
   * @param other
   */
  HexahedralGeom(const HexahedralGeom& other);

  /**
   * @brief
   * @param other
   */
  HexahedralGeom(HexahedralGeom&& other) noexcept;

  virtual ~HexahedralGeom();

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
   * @param numQuads
   */
  void resizeQuadList(size_t numQuads);

  /**
   * @brief
   * @param quads
   */
  void setQuads(const SharedQuadList* quads);

  /**
   * @brief
   * @return SharedQuadList*
   */
  SharedQuadList* getQuads();

  /**
   * @brief
   * @return const SharedQuadList*
   */
  const SharedQuadList* getQuads() const;

  /**
   * @brief
   * @param quadId
   * @param verts
   */
  void setVertsAtQuad(size_t quadId, size_t verts[4]);

  /**
   * @brief
   * @param quadId
   * @param verts
   */
  void getVertsAtQuad(size_t quadId, size_t verts[4]) const;

  /**
   * @brief
   * @param quadId
   * @param vert1
   * @param vert2
   * @param vert3
   * @param vert4
   */
  void getVertCoordsAtQuad(size_t quadId, complex::Point3D<float>& vert1, complex::Point3D<float>& vert2, complex::Point3D<float>& vert3, complex::Point3D<float>& vert4) const;

  /**
   * @brief
   * @return size_t
   */
  size_t getNumberOfQuads() const;

  /**
   * @brief
   * @param numHexas
   */
  void resizeHexList(size_t numHexas);

  /**
   * @brief
   * @param hexas
   */
  void setHexahedra(const SharedHexList* hexas);

  /**
   * @brief
   * @return SharedHexList*
   */
  SharedHexList* getHexahedra();

  /**
   * @brief
   * @return const SharedHexList*
   */
  const SharedHexList* getHexahedra() const;

  /**
   * @brief
   * @param hexId
   * @param verts
   */
  void setVertsAtHex(size_t hexId, size_t verts[8]);

  /**
   * @brief
   * @param hexId
   * @param verts
   */
  void getVertsAtHex(size_t hexId, size_t verts[8]) const;

  /**
   * @brief
   * @param hexId
   * @param vert1
   * @param vert2
   * @param vert3
   * @param vert4
   * @param vert5
   * @param vert6
   * @param vert7
   * @param vert8
   */
  void getVertCoordsAtHex(size_t hexId, complex::Point3D<float>& vert1, complex::Point3D<float>& vert2, complex::Point3D<float>& vert3, complex::Point3D<float>& vert4, complex::Point3D<float>& vert5,
                          complex::Point3D<float>& vert6, complex::Point3D<float>& vert7, complex::Point3D<float>& vert8) const;

  /**
   * @brief
   * @return size_t
   */
  size_t getNumberOfHexas() const;

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
   * @return StatusCode
   */
  StatusCode findElementCentroids() override;

  /**
   * @brief
   * @return
   */
  const FloatArray* getElementCentroids() const override;

  /**
   * @brief
   */
  void deleteElementCentroids() override;

  /**
   * @brief
   * @return
   */
  complex::Point3D<double> getParametricCenter() const override;

  /**
   * @brief
   * @param  pCoords
   * @param  shape
   */
  void getShapeFunctions(const complex::Point3D<double>& pCoords, double* shape) const override;

  /**
   * @brief
   * @param field
   * @param derivatives
   * @param observable
   */
  void findDerivatives(DoubleArray* field, DoubleArray* derivatives, Observable* observable = nullptr) const override;

  /**
   * @brief
   * @return complex::TooltipGenerator
   */
  complex::TooltipGenerator getTooltipGenerator() const override;

  /**
   * @brief
   * @param numVertices
   */
  void resizeVertexList(size_t numVertices) override;

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
  void setCoords(size_t vertId, const complex::Point3D<float>& coords) override;

  /**
   * @brief
   * @param vertId
   * @return
   */
  complex::Point3D<float> getCoords(size_t vertId) const override;

  /**
   * @brief
   * @return size_t
   */
  size_t getNumberOfVertices() const override;

  /**
   * @brief
   * @param numEdges
   */
  void resizeEdgeList(size_t numEdges) override;

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
   * @param edgeId
   * @param verts
   */
  void setVertsAtEdge(size_t edgeId, const size_t verts[2]) override;

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
  void getVertCoordsAtEdge(size_t edgeId, complex::Point3D<float>& vert1, complex::Point3D<float>& vert2) const override;

  /**
   * @brief
   * @return size_t
   */
  size_t getNumberOfEdges() const override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findEdges() override;

  /**
   * @brief
   */
  void deleteEdges() override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findFaces() override;

  /**
   * @brief
   */
  void deleteFaces() override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findUnsharedEdges() override;

  /**
   * @brief
   * @return const SharedEdgeList*
   */
  const SharedEdgeList* getUnsharedEdges() const override;

  /**
   * @brief
   */
  void deleteUnsharedEdges() override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findUnsharedFaces() override;

  /**
   * @brief
   * @return const SharedEdgeList*
   */
  const SharedEdgeList* getUnsharedFaces() const override;

  /**
   * @brief
   */
  void deleteUnsharedFaces() override;

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
  HexahedralGeom(DataStructure* ds, const std::string& name);

  /**
   * @brief
   * @param ds
   * @param name
   * @param numHexas
   * @param vertices
   * @param allocate
   */
  HexahedralGeom(DataStructure* ds, const std::string& name, size_t numHexas, const std::shared_ptr<SharedVertexList>& vertices, bool allocate = true);

  /**
   * @brief
   * @param ds
   * @param name
   * @param hexas
   * @param vertices
   */
  HexahedralGeom(DataStructure* ds, const std::string& name, const std::shared_ptr<SharedHexList>& hexas, const std::shared_ptr<SharedVertexList>& vertices);

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

  /**
   * @brief
   * @param bFaceList
   */
  void setUnsharedFaces(const SharedFaceList* bFaceList) override;

private:
  std::optional<DataObject::IdType> m_VertexListId;
  std::optional<DataObject::IdType> m_EdgeListId;
  std::optional<DataObject::IdType> m_UnsharedEdgeListId;
  std::optional<DataObject::IdType> m_QuadListId;
  std::optional<DataObject::IdType> m_UnsharedQuadListId;
  std::optional<DataObject::IdType> m_HexListId;
  std::optional<DataObject::IdType> m_HexasContainingVertId;
  std::optional<DataObject::IdType> m_HexNeighborsId;
  std::optional<DataObject::IdType> m_HexCentroidsId;
  std::optional<DataObject::IdType> m_HexSizesId;
};
} // namespace complex
