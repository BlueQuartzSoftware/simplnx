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
  void setBounds(const Float32Array* xBounds, const Float32Array* yBounds, const Float32Array* zBounds);

  /**
   * @brief
   * @return const Float32Array*
   */
  const Float32Array* getXBounds() const;

  /**
   * @brief
   * @return const Float32Array*
   */
  const Float32Array* getYBounds() const;

  /**
   * @brief
   * @return const Float32Array*
   */
  const Float32Array* getZBounds() const;

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
   * @param pCoords
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
  void findDerivatives(Float64Array* field, Float64Array* derivatives, Observable* observable = nullptr) const override;

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
   * @return usize
   */
  usize getNumXPoints() const override;

  /**
   * @brief
   * @return usize
   */
  usize getNumYPoints() const override;

  /**
   * @brief
   * @return usize
   */
  usize getNumZPoints() const override;

  /**
   * @brief
   * @param idx
   * @return complex::Point3D<float32>
   */
  complex::Point3D<float32> getPlaneCoordsf(usize idx[3]) const override;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @return complex::Point3D<float32>
   */
  complex::Point3D<float32> getPlaneCoordsf(usize x, usize y, usize z) const override;

  /**
   * @brief
   * @param idx
   * @return complex::Point3D<float32>
   */
  complex::Point3D<float32> getPlaneCoordsf(usize idx) const override;

  /**
   * @brief
   * @param idx
   * @return complex::Point3D<float64>
   */
  complex::Point3D<float64> getPlaneCoords(usize idx[3]) const override;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @return complex::Point3D<float64>
   */
  complex::Point3D<float64> getPlaneCoords(usize x, usize y, usize z) const override;

  /**
   * @brief
   * @param idx
   * @return complex::Point3D<float64>
   */
  complex::Point3D<float64> getPlaneCoords(usize idx) const override;

  /**
   * @brief
   * @param idx
   * @return complex::Point3D<float32>
   */
  complex::Point3D<float32> getCoordsf(usize idx[3]) const override;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @return complex::Point3D<float32>
   */
  complex::Point3D<float32> getCoordsf(usize x, usize y, usize z) const override;

  /**
   * @brief
   * @param idx
   * @return complex::Point3D<float32>
   */
  complex::Point3D<float32> getCoordsf(usize idx) const override;

  /**
   * @brief
   * @param idx
   * @return complex::Point3D<float64>
   */
  complex::Point3D<float64> getCoords(usize idx[3]) const override;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @return complex::Point3D<float64>
   */
  complex::Point3D<float64> getCoords(usize x, usize y, usize z) const override;

  /**
   * @brief
   * @param idx
   * @return complex::Point3D<float64>
   */
  complex::Point3D<float64> getCoords(usize idx) const override;

  /**
   * @brief
   * @param xCoord
   * @param yCoord
   * @param zCoord
   */
  usize getIndex(float32 xCoord, float32 yCoord, float32 zCoord) const override;

  /**
   * @brief
   * @param xCoord
   * @param yCoord
   * @param zCoord
   */
  usize getIndex(float64 xCoord, float64 yCoord, float64 zCoord) const override;

  /**
   * @brief
   * @return uint32
   */
  uint32 getXdmfGridType() const override;

  /**
   * @brief Reads values from HDF5
   * @param groupReader
   * @return H5::ErrorType
   */
  H5::ErrorType readHdf5(const H5::GroupReader& groupReader) override;

  /**
   * @brief Writes the geometry to HDF5 using the provided parent group ID.
   * @param parentGroupWriter
   * @return H5::ErrorType
   */
  H5::ErrorType writeHdf5(H5::GroupWriter& parentGroupWriter) const override;

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
  // StatusCode gatherMetaData(hid_t parentId, usize volDims, bool preflight);

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
  void setElementCentroids(const Float32Array* elementCentroids) override;

  /**
   * @brief
   * @param elementSizes
   */
  void setElementSizes(const Float32Array* elementSizes) override;

private:
  std::optional<DataObject::IdType> m_xBoundsId;
  std::optional<DataObject::IdType> m_yBoundsId;
  std::optional<DataObject::IdType> m_zBoundsId;
  std::optional<DataObject::IdType> m_VoxelSizesId;
  SizeVec3 m_Dimensions;
};
} // namespace complex
