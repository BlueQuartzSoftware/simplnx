#pragma once

#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Array.hpp"

#include <optional>

namespace nx::core
{
/**
 * @class RectGridGeom
 * @brief Represents a rectilinear grid geometry with non-uniform spacing along each axis defined by coordinate bounds arrays.
 */
class SIMPLNX_EXPORT RectGridGeom : public IGridGeometry
{
public:
  friend class DataStructure;

  static inline constexpr StringLiteral k_TypeName = "RectGridGeom";

  /**
   * @brief Creates a new RectGridGeom in the specified DataStructure.
   * @param dataStructure The DataStructure to create the RectGridGeom in
   * @param name The name for the new RectGridGeom
   * @param parentId Optional parent object ID to insert the RectGridGeom under
   * @return RectGridGeom* Pointer to the created RectGridGeom, or nullptr if creation failed
   */
  static RectGridGeom* Create(DataStructure& dataStructure, std::string name, const std::optional<IdType>& parentId = {});

  /**
   * @brief Imports a RectGridGeom into the specified DataStructure with a given import ID.
   * @param dataStructure The DataStructure to import the RectGridGeom into
   * @param name The name for the imported RectGridGeom
   * @param importId The ID to use for this imported object
   * @param parentId Optional parent object ID to insert the RectGridGeom under
   * @return RectGridGeom* Pointer to the imported RectGridGeom, or nullptr if import failed
   */
  static RectGridGeom* Import(DataStructure& dataStructure, std::string name, IdType importId, const std::optional<IdType>& parentId = {});

  /**
   * @brief Copy constructor.
   * @param other The RectGridGeom to copy from
   */
  RectGridGeom(const RectGridGeom& other) = default;

  /**
   * @brief Move constructor.
   * @param other The RectGridGeom to move from
   */
  RectGridGeom(RectGridGeom&& other) = default;

  ~RectGridGeom() noexcept override = default;

  RectGridGeom& operator=(const RectGridGeom&) = delete;
  RectGridGeom& operator=(RectGridGeom&&) noexcept = delete;

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
   * @brief Creates a shallow copy of this RectGridGeom.
   * @return DataObject* Pointer to the shallow copy
   */
  DataObject* shallowCopy() override;

  /**
   * @brief Creates a deep copy of this RectGridGeom at the specified path.
   * @param copyPath The path where the deep copy should be created
   * @return std::shared_ptr<DataObject> Shared pointer to the deep copy
   */
  std::shared_ptr<DataObject> deepCopy(const DataPath& copyPath) override;

  /**
   * @brief Sets the coordinate bounds arrays for X, Y, and Z axes.
   * @param xBounds Array of X coordinate bounds
   * @param yBounds Array of Y coordinate bounds
   * @param zBounds Array of Z coordinate bounds
   */
  void setBounds(const Float32Array* xBounds, const Float32Array* yBounds, const Float32Array* zBounds);

  /**
   * @brief Returns a pointer to the X coordinate bounds array.
   * @return Float32Array* Pointer to the X bounds, or nullptr if not available
   */
  Float32Array* getXBounds();

  /**
   * @brief Returns a pointer to the Y coordinate bounds array.
   * @return Float32Array* Pointer to the Y bounds, or nullptr if not available
   */
  Float32Array* getYBounds();

  /**
   * @brief Returns a pointer to the Z coordinate bounds array.
   * @return Float32Array* Pointer to the Z bounds, or nullptr if not available
   */
  Float32Array* getZBounds();

  /**
   * @brief Returns a const pointer to the X coordinate bounds array.
   * @return const Float32Array* Const pointer to the X bounds, or nullptr if not available
   */
  const Float32Array* getXBounds() const;

  /**
   * @brief Returns a const pointer to the Y coordinate bounds array.
   * @return const Float32Array* Const pointer to the Y bounds, or nullptr if not available
   */
  const Float32Array* getYBounds() const;

  /**
   * @brief Returns a const pointer to the Z coordinate bounds array.
   * @return const Float32Array* Const pointer to the Z bounds, or nullptr if not available
   */
  const Float32Array* getZBounds() const;

  /**
   * @brief Returns a reference to the X coordinate bounds array.
   * @return Float32Array& Reference to the X bounds
   */
  Float32Array& getXBoundsRef();

  /**
   * @brief Returns a reference to the Y coordinate bounds array.
   * @return Float32Array& Reference to the Y bounds
   */
  Float32Array& getYBoundsRef();

  /**
   * @brief Returns a reference to the Z coordinate bounds array.
   * @return Float32Array& Reference to the Z bounds
   */
  Float32Array& getZBoundsRef();

  /**
   * @brief Returns a const reference to the X coordinate bounds array.
   * @return const Float32Array& Const reference to the X bounds
   */
  const Float32Array& getXBoundsRef() const;

  /**
   * @brief Returns a const reference to the Y coordinate bounds array.
   * @return const Float32Array& Const reference to the Y bounds
   */
  const Float32Array& getYBoundsRef() const;

  /**
   * @brief Returns a const reference to the Z coordinate bounds array.
   * @return const Float32Array& Const reference to the Z bounds
   */
  const Float32Array& getZBoundsRef() const;

  /**
   * @brief Returns the optional ID of the X bounds array.
   * @return OptionalId The X bounds ID if it exists
   */
  OptionalId getXBoundsId() const;

  /**
   * @brief Returns the optional ID of the Y bounds array.
   * @return OptionalId The Y bounds ID if it exists
   */
  OptionalId getYBoundsId() const;

  /**
   * @brief Returns the optional ID of the Z bounds array.
   * @return OptionalId The Z bounds ID if it exists
   */
  OptionalId getZBoundsId() const;

  /**
   * @brief Sets the ID of the X bounds array.
   * @param xBoundsId The optional ID of the X bounds array
   */
  void setXBoundsId(const OptionalId& xBoundsId);

  /**
   * @brief Sets the ID of the Y bounds array.
   * @param yBoundsId The optional ID of the Y bounds array
   */
  void setYBoundsId(const OptionalId& yBoundsId);

  /**
   * @brief Sets the ID of the Z bounds array.
   * @param zBoundsId The optional ID of the Z bounds array
   */
  void setZBoundsId(const OptionalId& zBoundsId);

  /**
   * @brief Returns a shared pointer to the X coordinate bounds array.
   * @return std::shared_ptr<Float32Array> Shared pointer to the X bounds
   */
  std::shared_ptr<Float32Array> getSharedXBounds();

  /**
   * @brief Returns a shared pointer to the Y coordinate bounds array.
   * @return std::shared_ptr<Float32Array> Shared pointer to the Y bounds
   */
  std::shared_ptr<Float32Array> getSharedYBounds();

  /**
   * @brief Returns a shared pointer to the Z coordinate bounds array.
   * @return std::shared_ptr<Float32Array> Shared pointer to the Z bounds
   */
  std::shared_ptr<Float32Array> getSharedZBounds();

  /**
   * @brief Returns the total number of cells in the grid.
   * @return usize The number of cells
   */
  usize getNumberOfCells() const override;

  /**
   * @brief This function calculates the sizes of each voxel in the
   * geometry and store it in a new or existing array in the datastructure
   * @param recalculate This will allow for skipping execution when an
   * Element Sizes Array exists and recalculate is `false`
   * @return Result<>
   */
  Result<> findElementSizes(bool recalculate) override;

  /**
   * @brief Returns the parametric center of a grid cell.
   * @return Point3D<float64> The parametric center coordinates
   */
  Point3D<float64> getParametricCenter() const override;

  /**
   * @brief Calculates shape functions at the given parametric coordinates.
   * @param pCoords The parametric coordinates
   * @param shape Output array to store the calculated shape function values
   */
  void getShapeFunctions(const Point3D<float64>& pCoords, double* shape) const override;

  /**
   * @brief Sets the dimensions of the grid geometry.
   * @param dims The new dimensions in X, Y, and Z
   */
  void setDimensions(const SizeVec3& dims) override;

  /**
   * @brief Returns the dimensions of the grid geometry.
   * @return SizeVec3 The dimensions in X, Y, and Z
   */
  SizeVec3 getDimensions() const override;

  /**
   * @brief Returns the origin coordinates (minimum bounds) of the grid.
   * @return Result<FloatVec3> The origin coordinates or an error
   */
  Result<FloatVec3> getOrigin() const;

  /**
   * @brief Returns the number of cells in the X dimension.
   * @return usize The number of cells in X
   */
  usize getNumXCells() const override;

  /**
   * @brief Returns the number of cells in the Y dimension.
   * @return usize The number of cells in Y
   */
  usize getNumYCells() const override;

  /**
   * @brief Returns the number of cells in the Z dimension.
   * @return usize The number of cells in Z
   */
  usize getNumZCells() const override;

  /**
   * @brief Returns the plane coordinates at the specified index array as float32.
   * @param idx Array of indices [x, y, z]
   * @return Point3D<float32> The plane coordinates
   */
  Point3D<float32> getPlaneCoordsf(usize idx[3]) const override;

  /**
   * @brief Returns the plane coordinates at the specified X, Y, Z indices as float32.
   * @param x The X index
   * @param y The Y index
   * @param z The Z index
   * @return Point3D<float32> The plane coordinates
   */
  Point3D<float32> getPlaneCoordsf(usize x, usize y, usize z) const override;

  /**
   * @brief Returns the plane coordinates at the specified flat index as float32.
   * @param idx The flat index
   * @return Point3D<float32> The plane coordinates
   */
  Point3D<float32> getPlaneCoordsf(usize idx) const override;

  /**
   * @brief Returns the plane coordinates at the specified index array as float64.
   * @param idx Array of indices [x, y, z]
   * @return Point3D<float64> The plane coordinates
   */
  Point3D<float64> getPlaneCoords(usize idx[3]) const override;

  /**
   * @brief Returns the plane coordinates at the specified X, Y, Z indices as float64.
   * @param x The X index
   * @param y The Y index
   * @param z The Z index
   * @return Point3D<float64> The plane coordinates
   */
  Point3D<float64> getPlaneCoords(usize x, usize y, usize z) const override;

  /**
   * @brief Returns the plane coordinates at the specified flat index as float64.
   * @param idx The flat index
   * @return Point3D<float64> The plane coordinates
   */
  Point3D<float64> getPlaneCoords(usize idx) const override;

  /**
   * @brief Returns the cell center coordinates at the specified index array as float32.
   * @param idx Array of indices [x, y, z]
   * @return Point3D<float32> The cell center coordinates
   */
  Point3D<float32> getCoordsf(usize idx[3]) const override;

  /**
   * @brief Returns the cell center coordinates at the specified X, Y, Z indices as float32.
   * @param x The X index
   * @param y The Y index
   * @param z The Z index
   * @return Point3D<float32> The cell center coordinates
   */
  Point3D<float32> getCoordsf(usize x, usize y, usize z) const override;

  /**
   * @brief Returns the cell center coordinates at the specified flat index as float32.
   * @param idx The flat index
   * @return Point3D<float32> The cell center coordinates
   */
  Point3D<float32> getCoordsf(usize idx) const override;

  /**
   * @brief Returns the cell center coordinates at the specified index array as float64.
   * @param idx Array of indices [x, y, z]
   * @return Point3D<float64> The cell center coordinates
   */
  Point3D<float64> getCoords(usize idx[3]) const override;

  /**
   * @brief Returns the cell center coordinates at the specified X, Y, Z indices as float64.
   * @param x The X index
   * @param y The Y index
   * @param z The Z index
   * @return Point3D<float64> The cell center coordinates
   */
  Point3D<float64> getCoords(usize x, usize y, usize z) const override;

  /**
   * @brief Returns the cell center coordinates at the specified flat index as float64.
   * @param idx The flat index
   * @return Point3D<float64> The cell center coordinates
   */
  Point3D<float64> getCoords(usize idx) const override;

  /**
   * @brief Returns the flat index for the cell containing the specified coordinates (float32).
   * @param xCoord The X coordinate
   * @param yCoord The Y coordinate
   * @param zCoord The Z coordinate
   * @return std::optional<usize> The flat index if coordinates are within bounds, nullopt otherwise
   */
  std::optional<usize> getIndex(float32 xCoord, float32 yCoord, float32 zCoord) const override;

  /**
   * @brief Returns the flat index for the cell containing the specified coordinates (float64).
   * @param xCoord The X coordinate
   * @param yCoord The Y coordinate
   * @param zCoord The Z coordinate
   * @return std::optional<usize> The flat index if coordinates are within bounds, nullopt otherwise
   */
  std::optional<usize> getIndex(float64 xCoord, float64 yCoord, float64 zCoord) const override;

protected:
  /**
   * @brief Constructs a RectGridGeom with the specified name.
   * @param dataStructure The DataStructure this geometry belongs to
   * @param name The name for this geometry
   */
  RectGridGeom(DataStructure& dataStructure, std::string name);

  /**
   * @brief Constructs a RectGridGeom with the specified name and import ID.
   * @param dataStructure The DataStructure this geometry belongs to
   * @param name The name for this geometry
   * @param importId The ID to use for this imported object
   */
  RectGridGeom(DataStructure& dataStructure, std::string name, IdType importId);

  /**
   * @brief Updates the array IDs. Should only be called by DataObject::checkUpdatedIds.
   * @param updatedIdsMap
   */
  void checkUpdatedIdsImpl(const std::unordered_map<DataObject::IdType, DataObject::IdType>& updatedIdsMap) override;

private:
  std::optional<IdType> m_xBoundsId;
  std::optional<IdType> m_yBoundsId;
  std::optional<IdType> m_zBoundsId;
  SizeVec3 m_Dimensions;
};
} // namespace nx::core
