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
   * @param ds
   * @param name
   * @param parentId = {}
   * @return RectGridGeom*
   */
  static RectGridGeom* Create(DataStructure& ds, const std::string& name, const std::optional<IdType>& parentId = {});

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
   * @param xBounds
   * @param yBounds
   * @param zBounds
   */
  void setBounds(const FloatArrayType* xBounds, const FloatArrayType* yBounds, const FloatArrayType* zBounds);

  /**
   * @brief
   * @return const FloatArrayType*
   */
  const FloatArrayType* getXBounds() const;

  /**
   * @brief
   * @return const FloatArrayType*
   */
  const FloatArrayType* getYBounds() const;

  /**
   * @brief
   * @return const FloatArrayType*
   */
  const FloatArrayType* getZBounds() const;

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
  void findDerivatives(DoubleArrayType* field, DoubleArrayType* derivatives, Observable* observable = nullptr) const override;

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
  RectGridGeom(DataStructure& ds, const std::string& name);

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
  void setElementCentroids(const FloatArrayType* elementCentroids) override;

  /**
   * @brief
   * @param elementSizes
   */
  void setElementSizes(const FloatArrayType* elementSizes) override;

private:
  std::optional<DataObject::IdType> m_xBoundsId;
  std::optional<DataObject::IdType> m_yBoundsId;
  std::optional<DataObject::IdType> m_zBoundsId;
  std::optional<DataObject::IdType> m_VoxelSizesId;
  SizeVec3 m_Dimensions;
};
} // namespace complex
