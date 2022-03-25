#pragma once

#include "complex/Common/Array.hpp"
#include "complex/Common/BoundingBox.hpp"
#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Geometry/AbstractGeometryGrid.hpp"

#include "complex/complex_export.hpp"

#include <map>
#include <vector>

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
  static ImageGeom* Create(DataStructure& dataStructure, std::string name, const std::optional<IdType>& parentId = {});

  /**
   * @brief
   * @param ds
   * @param name
   * @param importId
   * @param parentId = {}
   * @return ImageGeom*
   */
  static ImageGeom* Import(DataStructure& dataStructure, std::string name, IdType importId, const std::optional<IdType>& parentId = {});

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
   * @brief Returns the type of geometry.
   * @return
   */
  AbstractGeometry::Type getGeomType() const override;

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return
   */
  DataObject::Type getDataObjectType() const override;

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
  void setSpacing(float32 x, float32 y, float32 z);

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
  void setOrigin(float32 x, float32 y, float32 z);

  /**
   * @brief
   * @return BoundingBox<float32>
   */
  BoundingBox<float32> getBoundingBoxf() const;

  /**
   * @brief
   * @return BoundingBox<float64>
   */
  BoundingBox<float64> getBoundingBox() const;

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
   * @return std::shared_ptr<Float32Array>
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
   * @brief getDimensionality Determines the dimensionality of the geometry and returns either 1, 2, or 3
   * Example 1: If this image geometry has dimensions 100 x 100 x 100, this method would return a dimensionality of 3.
   * Example 2: If this image geometry has dimensions 100 x 100 x 1, this method would return a dimensionality of 2.
   * Example 3: If this image geometry has dimensions 1 x 1 x 100, this method would return a dimensionality of 1.
   * Example 4: If this image geometry has dimensions 1 x 1 x 1, this method would return a dimensionality of 1.
   * @return usize
   */
  usize getDimensionality() const;

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
   * @return Point3D<float32>
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
   * @return usize
   */
  usize getIndex(float32 xCoord, float32 yCoord, float32 zCoord) const override;

  /**
   * @brief
   * @param xCoord
   * @param yCoord
   * @param zCoord
   * @return usize
   */
  usize getIndex(float64 xCoord, float64 yCoord, float64 zCoord) const override;

  /**
   * @brief
   * @param coords
   * @param index
   * @return ErrorType
   */
  ErrorType computeCellIndex(const complex::Point3D<float32>& coords, SizeVec3& index) const;

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
  H5::ErrorType readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupId) override;

  /**
   * @brief Writes the geometry to HDF5 using the provided parent group ID.
   * @param dataStructureWriter
   * @param parentGroupWriter
   * @param importable
   * @return H5::ErrorType
   */
  H5::ErrorType writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter, bool importable) const override;

protected:
  /**
   * @brief
   * @param ds
   * @param name
   */
  ImageGeom(DataStructure& ds, std::string name);

  /**
   * @brief
   * @param ds
   * @param name
   * @param importId
   */
  ImageGeom(DataStructure& ds, std::string name, IdType importId);

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
  // StatusCode gatherMetaData(hid_t parentId, usize volDims, float32 spacing, float32 origin, uint32 spatialDims, const std::string& geomName, bool preflight);

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

  /**
   * @brief Updates the array IDs. Should only be called by DataObject::checkUpdatedIds.
   * @param updatedIds
   */
  void checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds) override;

private:
  std::optional<IdType> m_VoxelSizesId;
  FloatVec3 m_Spacing;
  FloatVec3 m_Origin;
  SizeVec3 m_Dimensions;
};
} // namespace complex
