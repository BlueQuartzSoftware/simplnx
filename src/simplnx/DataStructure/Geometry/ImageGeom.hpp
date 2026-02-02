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
 * @brief Represents a regular 3D grid geometry with uniform spacing in each dimension.
 * Stores origin, spacing, and dimensions to define a structured image-like grid of cells.
 */
class SIMPLNX_EXPORT ImageGeom : public IGridGeometry
{
public:
  friend class DataStructure;

  static constexpr StringLiteral k_TypeName = "ImageGeom";

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
   * @brief Creates a new ImageGeom object and adds it to the provided DataStructure.
   * @param dataStructure The DataStructure to which the ImageGeom will be added
   * @param name The name of the ImageGeom object
   * @param parentId Optional parent ID for hierarchical organization within the DataStructure
   * @return Pointer to the newly created ImageGeom object, or nullptr if creation failed
   */
  static ImageGeom* Create(DataStructure& dataStructure, std::string name, const std::optional<IdType>& parentId = {});

  /**
   * @brief Imports an existing ImageGeom object into the provided DataStructure with a specified ID.
   * @param dataStructure The DataStructure to which the ImageGeom will be imported
   * @param name The name of the ImageGeom object
   * @param importId The ID to assign to the imported ImageGeom
   * @param parentId Optional parent ID for hierarchical organization within the DataStructure
   * @return Pointer to the imported ImageGeom object, or nullptr if import failed
   */
  static ImageGeom* Import(DataStructure& dataStructure, std::string name, IdType importId, const std::optional<IdType>& parentId = {});

  /**
   * @brief Copy constructor creates a new ImageGeom as a copy of another.
   * @param other The ImageGeom object to copy from
   */
  ImageGeom(const ImageGeom& other) = default;

  /**
   * @brief Move constructor transfers ownership from another ImageGeom.
   * @param other The ImageGeom object to move from
   */
  ImageGeom(ImageGeom&& other) = default;

  ~ImageGeom() noexcept override = default;

  /**
   * @brief Returns the type of geometry.
   * @return The geometry type enumeration value for ImageGeom
   */
  IGeometry::Type getGeomType() const override;

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return The DataObject type enumeration value for this object
   */
  DataObject::Type getDataObjectType() const override;

  /**
   * @brief Returns an enumeration of the class or subclass GroupType. Used for quick comparison or type deduction
   * @return The GroupType enumeration value for this geometry
   */
  GroupType getGroupType() const override;

  /**
   * @brief Returns typename of the DataObject as a std::string.
   * @return String representation of the DataObject type name
   */
  std::string getTypeName() const override;

  /**
   * @brief Creates a shallow copy of this ImageGeom object.
   * @return Pointer to the shallow copy DataObject
   */
  DataObject* shallowCopy() override;

  /**
   * @brief Creates a deep copy of this ImageGeom object at the specified path.
   * @param copyPath The DataPath where the deep copy will be placed
   * @return Shared pointer to the deep copy DataObject
   */
  std::shared_ptr<DataObject> deepCopy(const DataPath& copyPath) override;

  /**
   * @brief Gets the spacing (resolution) of the image geometry in X, Y, and Z dimensions.
   * @return FloatVec3 containing the spacing values for each dimension
   */
  FloatVec3 getSpacing() const;

  /**
   * @brief Sets the spacing (resolution) of the image geometry in X, Y, and Z dimensions.
   * @param spacing Vector containing the spacing values for each dimension
   */
  void setSpacing(const FloatVec3& spacing);

  /**
   * @brief Sets the spacing (resolution) of the image geometry using individual components.
   * @param x Spacing in the X dimension
   * @param y Spacing in the Y dimension
   * @param z Spacing in the Z dimension
   */
  void setSpacing(float32 x, float32 y, float32 z);

  /**
   * @brief Gets the origin (lower-left corner) of the image geometry in 3D space.
   * @return FloatVec3 containing the origin coordinates for each dimension
   */
  FloatVec3 getOrigin() const;

  /**
   * @brief Sets the origin (lower-left corner) of the image geometry in 3D space.
   * @param origin Vector containing the origin coordinates for each dimension
   */
  void setOrigin(const FloatVec3& origin);

  /**
   * @brief Sets the origin (lower-left corner) of the image geometry using individual components.
   * @param x Origin coordinate in the X dimension
   * @param y Origin coordinate in the Y dimension
   * @param z Origin coordinate in the Z dimension
   */
  void setOrigin(float32 x, float32 y, float32 z);

  /**
   * @brief Calculates and returns the 3D bounding box of the image geometry using 32-bit floats.
   * @return BoundingBox3D<float32> representing the spatial extent of the geometry
   */
  BoundingBox3D<float32> getBoundingBoxf() const;

  /**
   * @brief Calculates and returns the 3D bounding box of the image geometry using 64-bit floats.
   * @return BoundingBox3D<float64> representing the spatial extent of the geometry
   */
  BoundingBox3D<float64> getBoundingBox() const;

  /**
   * @brief Calculates the total number of cells in the image geometry.
   * @return Total number of cells (product of X, Y, and Z dimensions)
   */
  usize getNumberOfCells() const override;

  /**
   * @brief !!! NOTE: This function will return areas for "2D" and volume for "3D" !!!
   * This function calculates the sizes of each voxel in the geometry and
   * store it in a new or existing array in the DataStructure. This function
   * will error out if more than two dimensions have a size of `1` aka are empty (think about
   * a geometry XYZ 1x1x5, do you use X or Y spacing for area).
   * @param recalculate This will allow for skipping execution when an
   * Element Sizes Array exists and recalculate is `false`
   * @return Result<>
   */
  Result<> findElementSizes(bool recalculate) override;

  /**
   * @brief Returns the parametric center of the image geometry.
   * @return Point3D<float64> representing the parametric center coordinates
   */
  Point3D<float64> getParametricCenter() const override;

  /**
   * @brief Calculates shape functions at the given parametric coordinates.
   * @param pCoords The parametric coordinates where shape functions are evaluated
   * @param shape Output array to store the calculated shape function values
   */
  void getShapeFunctions(const Point3D<float64>& pCoords, float64* shape) const override;

  /**
   * @brief Determines the dimensionality of the geometry and returns either 1, 2, or 3.
   * Example 1: If this image geometry has dimensions 100 x 100 x 100, this method would return a dimensionality of 3.
   * Example 2: If this image geometry has dimensions 100 x 100 x 1, this method would return a dimensionality of 2.
   * Example 3: If this image geometry has dimensions 1 x 1 x 100, this method would return a dimensionality of 1.
   * Example 4: If this image geometry has dimensions 1 x 1 x 1, this method would return a dimensionality of 1.
   * @return The effective dimensionality (1, 2, or 3) based on non-singular dimensions
   */
  usize getDimensionality() const;

  /**
   * @brief Returns the dimensions of the image geometry in the order of X, Y, Z.
   * @return SizeVec3 containing the number of cells in each dimension [X, Y, Z]
   */
  SizeVec3 getDimensions() const override;

  /**
   * @brief Sets the dimensions of the Image Geometry. Ordering is X (Fastest), then Y, then Z (Slowest). These values
   * become important when calculating things like an index based off of the Dimension values.
   * @param dims Vector containing the number of cells in each dimension [X, Y, Z]
   */
  void setDimensions(const SizeVec3& dims) override;

  /**
   * @brief Returns the number of cells in the X dimension.
   * @return Number of cells along the X axis
   */
  usize getNumXCells() const override;

  /**
   * @brief Returns the number of cells in the Y dimension.
   * @return Number of cells along the Y axis
   */
  usize getNumYCells() const override;

  /**
   * @brief Returns the number of cells in the Z dimension.
   * @return Number of cells along the Z axis
   */
  usize getNumZCells() const override;

  /**
   * @brief Gets the planar coordinates at the specified cell indices using 32-bit floats.
   * @param idx Array of 3 indices [x, y, z] specifying the cell location
   * @return Point3D<float32> representing the planar coordinates
   */
  Point3D<float32> getPlaneCoordsf(usize idx[3]) const override;

  /**
   * @brief Gets the planar coordinates at the specified cell indices using 32-bit floats.
   * @param x Cell index in the X dimension
   * @param y Cell index in the Y dimension
   * @param z Cell index in the Z dimension
   * @return Point3D<float32> representing the planar coordinates
   */
  Point3D<float32> getPlaneCoordsf(usize x, usize y, usize z) const override;

  /**
   * @brief Gets the planar coordinates for a flat cell index using 32-bit floats.
   * @param idx Flat (linearized) cell index
   * @return Point3D<float32> representing the planar coordinates
   */
  Point3D<float32> getPlaneCoordsf(usize idx) const override;

  /**
   * @brief Gets the planar coordinates at the specified cell indices using 64-bit floats.
   * @param idx Array of 3 indices [x, y, z] specifying the cell location
   * @return Point3D<float64> representing the planar coordinates
   */
  Point3D<float64> getPlaneCoords(usize idx[3]) const override;

  /**
   * @brief Gets the planar coordinates at the specified cell indices using 64-bit floats.
   * @param x Cell index in the X dimension
   * @param y Cell index in the Y dimension
   * @param z Cell index in the Z dimension
   * @return Point3D<float64> representing the planar coordinates
   */
  Point3D<float64> getPlaneCoords(usize x, usize y, usize z) const override;

  /**
   * @brief Gets the planar coordinates for a flat cell index using 64-bit floats.
   * @param idx Flat (linearized) cell index
   * @return Point3D<float64> representing the planar coordinates
   */
  Point3D<float64> getPlaneCoords(usize idx) const override;

  /**
   * @brief Gets the cell center coordinates at the specified cell indices using 32-bit floats.
   * @param idx Array of 3 indices [x, y, z] specifying the cell location
   * @return Point3D<float32> representing the cell center coordinates
   */
  Point3D<float32> getCoordsf(usize idx[3]) const override;

  /**
   * @brief Gets the cell center coordinates at the specified cell indices using 32-bit floats.
   * @param x Cell index in the X dimension
   * @param y Cell index in the Y dimension
   * @param z Cell index in the Z dimension
   * @return Point3D<float32> representing the cell center coordinates
   */
  Point3D<float32> getCoordsf(usize x, usize y, usize z) const override;

  /**
   * @brief Gets the cell center coordinates for a flat cell index using 32-bit floats.
   * @param idx Flat (linearized) cell index
   * @return Point3D<float32> representing the cell center coordinates
   */
  Point3D<float32> getCoordsf(usize idx) const override;

  /**
   * @brief Gets the cell center coordinates at the specified cell indices using 64-bit floats.
   * @param idx Array of 3 indices [x, y, z] specifying the cell location
   * @return Point3D<float64> representing the cell center coordinates
   */
  Point3D<float64> getCoords(usize idx[3]) const override;

  /**
   * @brief Gets the cell center coordinates at the specified cell indices using 64-bit floats.
   * @param x Cell index in the X dimension
   * @param y Cell index in the Y dimension
   * @param z Cell index in the Z dimension
   * @return Point3D<float64> representing the cell center coordinates
   */
  Point3D<float64> getCoords(usize x, usize y, usize z) const override;

  /**
   * @brief Gets the cell center coordinates for a flat cell index using 64-bit floats.
   * @param idx Flat (linearized) cell index
   * @return Point3D<float64> representing the cell center coordinates
   */
  Point3D<float64> getCoords(usize idx) const override;

  /**
   * @brief Computes the flat cell index from 3D coordinates using 32-bit floats.
   * @param xCoord X coordinate in physical space
   * @param yCoord Y coordinate in physical space
   * @param zCoord Z coordinate in physical space
   * @return Optional flat cell index if coordinates are within bounds, std::nullopt otherwise
   */
  std::optional<usize> getIndex(float32 xCoord, float32 yCoord, float32 zCoord) const override;

  /**
   * @brief Computes the flat cell index from 3D coordinates using 64-bit floats.
   * @param xCoord X coordinate in physical space
   * @param yCoord Y coordinate in physical space
   * @param zCoord Z coordinate in physical space
   * @return Optional flat cell index if coordinates are within bounds, std::nullopt otherwise
   */
  std::optional<usize> getIndex(float64 xCoord, float64 yCoord, float64 zCoord) const override;

  /**
   * @brief Computes the 3D cell index from physical coordinates.
   * @param coords Physical coordinates as a Point3D
   * @param index Output parameter to store the computed 3D cell indices [x, y, z]
   * @return ErrorType indicating success or the type of out-of-bounds error if any
   */
  ErrorType computeCellIndex(const Point3D<float32>& coords, SizeVec3& index) const;

protected:
  /**
   * @brief Protected constructor for creating an ImageGeom with a generated ID.
   * @param dataStructure The DataStructure that will own this ImageGeom
   * @param name The name of the ImageGeom object
   */
  ImageGeom(DataStructure& dataStructure, std::string name);

  /**
   * @brief Protected constructor for creating an ImageGeom with a specified import ID.
   * @param dataStructure The DataStructure that will own this ImageGeom
   * @param name The name of the ImageGeom object
   * @param importId The ID to assign to this ImageGeom
   */
  ImageGeom(DataStructure& dataStructure, std::string name, IdType importId);

private:
  FloatVec3 m_Spacing;
  FloatVec3 m_Origin;
  SizeVec3 m_Dimensions;
};
} // namespace nx::core
