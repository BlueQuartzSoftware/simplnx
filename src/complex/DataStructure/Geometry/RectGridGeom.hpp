#pragma once

#include "complex/Common/Point3D.hpp"
#include "complex/DataStructure/Geometry/AbstractGeometryGrid.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class RectGridGeom
 * @brief
 */
class COMPLEX_EXPORT RectGridGeom : public AbstractGeometryGrid
{
public:
  friend class DataStructure;

  /**
   * @brief
   * @param other
   */
  RectGridGeom(const RectGridGeom& other);

  /**
   * @brief
   * @param other
   */
  RectGridGeom(RectGridGeom&& other) noexcept;

  virtual ~RectGridGeom();

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
   * @param xBounds
   * @param yBounds
   * @param zBounds
   */
  void setBounds(const FloatArray* xBounds, const FloatArray* yBounds, const FloatArray* zBounds);

  /**
   * @brief
   * @return const FloatArray*
   */
  const FloatArray* getXBounds() const;

  /**
   * @brief
   * @return const FloatArray*
   */
  const FloatArray* getYBounds() const;

  /**
   * @brief
   * @return const FloatArray*
   */
  const FloatArray* getZBounds() const;

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
   * @param dims
   */
  void setDimensions(const complex::SizeVec3& dims) override;

  /**
   * @brief
   * @return SizeVec3
   */
  complex::SizeVec3 getDimensions() const override;

  /**
   * @brief
   * @return size_t
   */
  size_t getNumXPoints() const override;

  /**
   * @brief
   * @return size_t
   */
  size_t getNumYPoints() const override;

  /**
   * @brief
   * @return size_t
   */
  size_t getNumZPoints() const override;

  /**
   * @brief
   * @param idx
   * @return complex::Point3D<float>
   */
  complex::Point3D<float> getPlaneCoordsf(size_t idx[3]) const override;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @return complex::Point3D<float>
   */
  complex::Point3D<float> getPlaneCoordsf(size_t x, size_t y, size_t z) const override;

  /**
   * @brief
   * @param idx
   * @return complex::Point3D<float>
   */
  complex::Point3D<float> getPlaneCoordsf(size_t idx) const override;

  /**
   * @brief
   * @param idx
   * @return complex::Point3D<double>
   */
  complex::Point3D<double> getPlaneCoords(size_t idx[3]) const override;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @return complex::Point3D<double>
   */
  complex::Point3D<double> getPlaneCoords(size_t x, size_t y, size_t z) const override;

  /**
   * @brief
   * @param idx
   * @return complex::Point3D<double>
   */
  complex::Point3D<double> getPlaneCoords(size_t idx) const override;

  /**
   * @brief
   * @param idx
   * @return complex::Point3D<float>
   */
  complex::Point3D<float> getCoordsf(size_t idx[3]) const override;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @return complex::Point3D<float>
   */
  complex::Point3D<float> getCoordsf(size_t x, size_t y, size_t z) const override;

  /**
   * @brief
   * @param idx
   * @return complex::Point3D<float>
   */
  complex::Point3D<float> getCoordsf(size_t idx) const override;

  /**
   * @brief
   * @param idx
   * @return complex::Point3D<double>
   */
  complex::Point3D<double> getCoords(size_t idx[3]) const override;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @return complex::Point3D<double>
   */
  complex::Point3D<double> getCoords(size_t x, size_t y, size_t z) const override;

  /**
   * @brief
   * @param idx
   * @return complex::Point3D<double>
   */
  complex::Point3D<double> getCoords(size_t idx) const override;

  /**
   * @brief
   * @param xCoord
   * @param yCoord
   * @param zCoord
   */
  size_t getIndex(float xCoord, float yCoord, float zCoord) const override;

  /**
   * @brief
   * @param xCoord
   * @param yCoord
   * @param zCoord
   */
  size_t getIndex(double xCoord, double yCoord, double zCoord) const override;

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
   */
  RectGridGeom(DataStructure* ds, const std::string& name);

  /**
   * @brief
   * @param parentId
   * @param volDims
   * @param preflight
   * @return StatusCode
   */
  // StatusCode gatherMetaData(hid_t parentId, size_t volDims, bool preflight);

  /**
   * @brief
   * @param elementsContainingVert
   */
  void setElementsContainingVert(const ElementDynamicList* elementsContainingVert) override;

  /**
   * @brief
   * @param elementsNeighbors
   */
  void setElementNeighbors(const ElementDynamicList* elementsNeighbors) override;

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
  std::optional<DataObject::IdType> m_xBoundsId;
  std::optional<DataObject::IdType> m_yBoundsId;
  std::optional<DataObject::IdType> m_zBoundsId;
  std::optional<DataObject::IdType> m_VoxelSizesId;
  SizeVec3 m_Dimensions;
};
} // namespace complex
