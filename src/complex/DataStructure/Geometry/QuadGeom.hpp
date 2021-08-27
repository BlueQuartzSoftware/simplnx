#pragma once

#include "complex/Common/Point3D.hpp"
#include "complex/DataStructure/Geometry/AbstractGeometry2D.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class QuadGeom
 * @brief
 */
class COMPLEX_EXPORT QuadGeom : public AbstractGeometry2D
{
public:
  friend class DataStructure;

  /**
   * @brief
   * @param ds
   * @param name
   * @param parentId = {}
   * @return QuadGeom*
   */
  static QuadGeom* Create(DataStructure& ds, const std::string& name, const std::optional<IdType>& parentId = {});

  /**
   * @brief
   * @param other
   */
  QuadGeom(const QuadGeom& other);

  /**
   * @brief
   * @param other
   */
  QuadGeom(QuadGeom&& other) noexcept;

  virtual ~QuadGeom();

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
   * @brief Returns the number of quads in the geometry. If the quad list has
   * not been set, this method returns 0.
   * @return size_t
   */
  size_t getNumberOfQuads() const;

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
   * @return std::shared_ptr<FloatArray>
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
   * @return std::shared_ptr<ElementDynamicList>
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
   * @return std::shared_ptr<ElementDynamicList>
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
   * @return std::shared_ptr<FloatArray>
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
  void findDerivatives(DoubleArray* field, DoubleArray* derivatives, Observable* observable = nullptr) const override;

  /**
   * @brief
   * @return complex::TooltipGenerator
   */
  complex::TooltipGenerator getTooltipGenerator() const override;

  /**
   * @brief
   * @param vertId
   * @param Point3D<float>
   */
  void setCoords(size_t vertId, const Point3D<float>& coord) override;

  /**
   * @brief
   * @param vertId
   * @return Point3D<float>
   */
  Point3D<float> getCoords(size_t vertId) const override;

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
   * @param edgeId
   * @param vert1
   * @param vert2
   */
  void getVertCoordsAtEdge(size_t edgeId, complex::Point3D<float>& vert1, complex::Point3D<float>& vert2) const override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findEdges() override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findUnsharedEdges() override;

  /**
   * @brief
   * @return uint32_t
   */
  uint32_t getXdmfGridType() const override;

  /**
   * @brief Reads values from HDF5
   * @param targetId
   * @param groupId
   * @return H5::ErrorType
   */
  H5::ErrorType readHdf5(H5::IdType targetId, H5::IdType groupId) override;

  /**
   * @brief Writes the geometry to HDF5 using the provided parent group ID.
   * @param parentId
   * @param groupId
   * @return H5::ErrorType
   */
  H5::ErrorType writeHdf5_impl(H5::IdType parentId, H5::IdType groupId) const override;

protected:
  /**
   * @brief
   * @param ds
   * @param name
   */
  QuadGeom(DataStructure& ds, const std::string& name);

  /**
   * @brief
   * @param ds
   * @param name
   * @param numQuads
   * @param vertices
   * @param allocate
   */
  QuadGeom(DataStructure& ds, const std::string& name, size_t numQuads, const std::shared_ptr<SharedVertexList>& vertices, bool allocate = true);

  /**
   * @brief
   * @param ds
   * @param name
   * @param quads
   * @param vertices
   */
  QuadGeom(DataStructure& ds, const std::string& name, const std::shared_ptr<SharedQuadList>& quads, const std::shared_ptr<SharedVertexList>& vertices);

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
  std::optional<DataObject::IdType> m_QuadListId;
  std::optional<DataObject::IdType> m_QuadsContainingVertId;
  std::optional<DataObject::IdType> m_QuadNeighborsId;
  std::optional<DataObject::IdType> m_QuadCentroidsId;
  std::optional<DataObject::IdType> m_QuadSizesId;
};
} // namespace complex
