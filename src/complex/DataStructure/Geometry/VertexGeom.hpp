#pragma once

#include "complex/DataStructure/Geometry/AbstractGeometry.hpp"
#include "complex/Utilities/TooltipGenerator.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class VertexGeom
 * @brief
 */
class COMPLEX_EXPORT VertexGeom : public AbstractGeometry
{
public:
  friend class DataStructure;

  /**
   * @brief
   * @param ds
   * @param name
   * @param parentId = {}
   * @return VertexGeom*
   */
  static VertexGeom* Create(DataStructure& ds, const std::string& name, const std::optional<IdType>& parentId = {});

  /**
   * @brief
   * @param other
   */
  VertexGeom(const VertexGeom& other);

  /**
   * @brief
   * @param other
   */
  VertexGeom(VertexGeom&& other) noexcept;

  virtual ~VertexGeom();

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
   */
  void initializeWithZeros() override;

  /**
   * @brief Resizes the vertex list to the target getSize.
   * @param newNumVertices
   */
  void resizeVertexList(size_t newNumVertices);

  /**
   * @brief Sets the SharedVertexList
   * @param vertices
   */
  void setVertices(const SharedVertexList* vertices);

  /**
   * @brief Returns the SharedVertexList.
   * @return SharedVertexList*
   */
  SharedVertexList* getVertices();

  /**
   * @brief Returns the SharedVertexList.
   * @return const SharedVertexList*
   */
  const SharedVertexList* getVertices() const;

  /**
   * @brief Gets the coordinates at the target vertex ID.
   * @param vertId
   */
  Point3D<float> getCoords(size_t vertId) const;

  /**
   * @brief Sets the coordinates for the specified vertex ID.
   * @param vertId
   * @param coords
   */
  void setCoords(size_t vertId, const Point3D<float>& coords);

  /**
   * @brief
   * @return size_t
   */
  size_t getNumberOfVertices() const;

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
   * @return const FloatArrayType*
   */
  const FloatArrayType* getElementSizes() const override;

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
   * @return const FloatArrayType*
   */
  const FloatArrayType* getElementCentroids() const override;

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
  void findDerivatives(DoubleArrayType* field, DoubleArrayType* derivatives, Observable* observable) const override;

  /**
   * @brief
   * @param format
   * @return std::string
   */
  std::string getInfoString(complex::InfoStringFormat format) const override;

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

  /**
   * @brief Reads values from HDF5
   * @param targetId
   * @param groupId
   * @return H5::ErrorType
   */
  H5::ErrorType readHdf5(H5::IdType targetId, H5::IdType groupId) override;

  /**
   * @brief Writes the geometry to HDF5 using the provided parent group ID.
   * @param groupId
   * @param parentId
   * @return H5::ErrorType
   */
  H5::ErrorType writeHdf5_impl(H5::IdType parentId, H5::IdType groupId) const override;

protected:
  /**
   * @brief
   * @param ds
   * @param name
   */
  VertexGeom(DataStructure& ds, const std::string& name);

  /**
   * @brief
   * @param ds
   * @param name
   * @param numVertices
   * @param allocate
   */
  VertexGeom(DataStructure& ds, const std::string& name, size_t numVertices, bool allocate);

  /**
   * @brief
   * @param ds
   * @param name
   * @param vertices
   */
  VertexGeom(DataStructure& ds, const std::string& name, const SharedVertexList* vertices);

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
  void setElementCentroids(const FloatArrayType* elementCentroids) override;

  /**
   * @brief
   * @param elementSizes
   */
  void setElementSizes(const FloatArrayType* elementSizes) override;

private:
  std::optional<DataObject::IdType> m_VertexListId;
  std::optional<DataObject::IdType> m_VertexSizesId;
};
} // namespace complex
