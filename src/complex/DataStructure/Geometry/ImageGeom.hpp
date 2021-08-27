#pragma once

#include "complex/Common/Array.hpp"
#include "complex/Common/BoundingBox.hpp"
#include "complex/DataStructure/Geometry/AbstractGeometryGrid.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class ImageGeom
 * @brief
 */
class COMPLEX_EXPORT ImageGeom : virtual public AbstractGeometryGrid
{
public:
  friend class DataStructure;

  enum class ErrorType : EnumType
  {
    XOutOfBoundsLow = 0,
    XOutOfBoundsHigh = 1,
    YOutOfBoundsLow = 2,
    YOutOfBoundsHigh = 3,
    ZOutOfBoundsLow = 4,
    ZOutOfBoundsHigh = 5,
    IndexOutOfBounds = 6,
    NoError = 7
  };

  /**
   * @brief
   * @param ds
   * @param name
   * @param parentId = {}
   * @return ImageGeom*
   */
  static ImageGeom* Create(DataStructure& ds, const std::string& name, const std::optional<IdType>& parentId = {});

  /**
   * @brief
   * @param other
   */
  ImageGeom(const ImageGeom& other);

  /**
   * @brief
   * @param other
   */
  ImageGeom(ImageGeom&& other) noexcept;

  virtual ~ImageGeom();

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
   * @return complex::FloatVec3
   */
  complex::FloatVec3 getSpacing() const;

  /**
   * @brief
   * @param spacing
   */
  void setSpacing(const complex::FloatVec3& spacing);

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   */
  void setSpacing(float x, float y, float z);

  /**
   * @brief
   * @return complex::FloatVec3
   */
  complex::FloatVec3 getOrigin() const;

  /**
   * @brief
   * @param origin
   */
  void setOrigin(const complex::FloatVec3& origin);

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   */
  void setOrigin(float x, float y, float z);

  /**
   * @brief
   * @return BoundingBox<float>
   */
  BoundingBox<float> getBoundingBoxf() const;

  /**
   * @brief
   * @return BoundingBox<double>
   */
  BoundingBox<double> getBoundingBox() const;

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
   * @return complex::SizeVec3
   */
  complex::SizeVec3 getDimensions() const override;

  /**
   * @brief
   * @param dims
   */
  void setDimensions(const complex::SizeVec3& dims) override;

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
   * @return Point3D<float>
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
   * @return size_t
   */
  size_t getIndex(float xCoord, float yCoord, float zCoord) const override;

  /**
   * @brief
   * @param xCoord
   * @param yCoord
   * @param zCoord
   * @return size_t
   */
  size_t getIndex(double xCoord, double yCoord, double zCoord) const override;

  /**
   * @brief
   * @param coords
   * @param index
   * @return ErrorType
   */
  ErrorType computeCellIndex(const complex::Point3D<float>& coords, SizeVec3& index) const;

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
  ImageGeom(DataStructure& ds, const std::string& name);

  /**
   * @brief
   * @param parentId
   * @param volDims
   * @param spacing
   * @param origin
   * @param spatialDims
   * @param geomName
   * @param preflight
   * @return StatusCode
   */
  // StatusCode gatherMetaData(hid_t parentId, size_t volDims, float spacing, float origin, uint32_t spatialDims, const std::string& geomName, bool preflight);

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
  std::optional<DataObject::IdType> m_VoxelSizesId;
  FloatVec3 m_Spacing;
  FloatVec3 m_Origin;
  SizeVec3 m_Dimensions;
};
} // namespace complex
