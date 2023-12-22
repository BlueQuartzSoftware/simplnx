#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/BoundingBox.hpp"
#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/simplnx_export.hpp"

#include <map>
#include <vector>

namespace nx::core
{
/**
 * @class ImageGeom
 * @brief
 */
class SIMPLNX_EXPORT ImageGeom : public IGridGeometry
{
public:
  friend class DataStructure;

  static inline constexpr StringLiteral k_TypeName = "ImageGeom";

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
   * @param dataStructure
   * @param name
   * @param parentId = {}
   * @return ImageGeom*
   */
  static ImageGeom* Create(DataStructure& dataStructure, std::string name, const std::optional<IdType>& parentId = {});

  /**
   * @brief
   * @param dataStructure
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
  ImageGeom(const ImageGeom& other) = default;

  /**
   * @brief
   * @param other
   */
  ImageGeom(ImageGeom&& other) = default;

  ~ImageGeom() noexcept override = default;

  /**
   * @brief Returns the type of geometry.
   * @return
   */
  IGeometry::Type getGeomType() const override;

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return
   */
  DataObject::Type getDataObjectType() const override;

  /**
   * @brief Returns an enumeration of the class or subclass GroupType. Used for quick comparison or type deduction
   * @return
   */
  GroupType getGroupType() const override;

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
  std::shared_ptr<DataObject> deepCopy(const DataPath& copyPath) override;

  /**
   * @brief
   * @return FloatVec3
   */
  FloatVec3 getSpacing() const;

  /**
   * @brief
   * @param spacing
   */
  void setSpacing(const FloatVec3& spacing);

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   */
  void setSpacing(float32 x, float32 y, float32 z);

  /**
   * @brief
   * @return FloatVec3
   */
  FloatVec3 getOrigin() const;

  /**
   * @brief
   * @param origin
   */
  void setOrigin(const FloatVec3& origin);

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
  BoundingBox3D<float32> getBoundingBoxf() const;

  /**
   * @brief
   * @return BoundingBox<float64>
   */
  BoundingBox3D<float64> getBoundingBox() const;

  /**
   * @brief
   * @return usize
   */
  usize getNumberOfCells() const override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findElementSizes() override;

  /**
   * @brief
   * @return Point3D<float64>
   */
  Point3D<float64> getParametricCenter() const override;

  /**
   * @brief
   * @param pCoords
   * @param shape
   */
  void getShapeFunctions(const Point3D<float64>& pCoords, float64* shape) const override;

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
   * @brief Returns the dimensions of the image geometry in the order of X, Y, Z
   * @return SizeVec3
   */
  SizeVec3 getDimensions() const override;

  /**
   * @brief Sets the dimensions of the Image Geometry. Ordering is X (Fastest), then Y, then Z (Slowest). These values
   * become important when calculating things like an index based off of the Dimension values.
   * @param dims
   */
  void setDimensions(const SizeVec3& dims) override;

  /**
   * @brief
   * @return usize
   */
  usize getNumXCells() const override;

  /**
   * @brief
   * @return usize
   */
  usize getNumYCells() const override;

  /**
   * @brief
   * @return usize
   */
  usize getNumZCells() const override;

  /**
   * @brief
   * @param idx
   * @return Point3D<float32>
   */
  Point3D<float32> getPlaneCoordsf(usize idx[3]) const override;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @return Point3D<float32>
   */
  Point3D<float32> getPlaneCoordsf(usize x, usize y, usize z) const override;

  /**
   * @brief
   * @param idx
   * @return Point3D<float32>
   */
  Point3D<float32> getPlaneCoordsf(usize idx) const override;

  /**
   * @brief
   * @param idx
   * @return Point3D<float64>
   */
  Point3D<float64> getPlaneCoords(usize idx[3]) const override;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @return Point3D<float64>
   */
  Point3D<float64> getPlaneCoords(usize x, usize y, usize z) const override;

  /**
   * @brief
   * @param idx
   * @return Point3D<float64>
   */
  Point3D<float64> getPlaneCoords(usize idx) const override;

  /**
   * @brief
   * @param idx
   * @return Point3D<float32>
   */
  Point3D<float32> getCoordsf(usize idx[3]) const override;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @return Point3D<float32>
   */
  Point3D<float32> getCoordsf(usize x, usize y, usize z) const override;

  /**
   * @brief
   * @param idx
   * @return Point3D<float32>
   */
  Point3D<float32> getCoordsf(usize idx) const override;

  /**
   * @brief
   * @param idx
   * @return Point3D<float64>
   */
  Point3D<float64> getCoords(usize idx[3]) const override;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @return Point3D<float64>
   */
  Point3D<float64> getCoords(usize x, usize y, usize z) const override;

  /**
   * @brief
   * @param idx
   * @return Point3D<float64>
   */
  Point3D<float64> getCoords(usize idx) const override;

  /**
   * @brief
   * @param xCoord
   * @param yCoord
   * @param zCoord
   * @return std::optional<usize>
   */
  std::optional<usize> getIndex(float32 xCoord, float32 yCoord, float32 zCoord) const override;

  /**
   * @brief
   * @param xCoord
   * @param yCoord
   * @param zCoord
   * @return std::optional<usize>
   */
  std::optional<usize> getIndex(float64 xCoord, float64 yCoord, float64 zCoord) const override;

  /**
   * @brief
   * @param coords
   * @param index
   * @return ErrorType
   */
  ErrorType computeCellIndex(const Point3D<float32>& coords, SizeVec3& index) const;

protected:
  /**
   * @brief
   * @param dataStructure
   * @param name
   */
  ImageGeom(DataStructure& dataStructure, std::string name);

  /**
   * @brief
   * @param dataStructure
   * @param name
   * @param importId
   */
  ImageGeom(DataStructure& dataStructure, std::string name, IdType importId);

private:
  FloatVec3 m_Spacing;
  FloatVec3 m_Origin;
  SizeVec3 m_Dimensions;
};
} // namespace nx::core
