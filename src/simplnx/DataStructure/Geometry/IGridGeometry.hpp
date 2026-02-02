#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"

#include "simplnx/Common/StringLiteral.hpp"

namespace nx::core
{
class SIMPLNX_EXPORT IGridGeometry : public IGeometry
{
public:
  static constexpr StringLiteral k_CellAttributeMatrixName = "Cell Data";
  static constexpr StringLiteral k_TypeName = "IGridGeometry";

  IGridGeometry() = delete;
  IGridGeometry(const IGridGeometry&) = default;
  IGridGeometry(IGridGeometry&&) = default;

  IGridGeometry& operator=(const IGridGeometry&) = delete;
  IGridGeometry& operator=(IGridGeometry&&) noexcept = delete;

  ~IGridGeometry() noexcept override = default;

  /**
   * @brief Returns the dimensions of the grid geometry.
   * @return SizeVec3 The dimensions in X, Y, and Z
   */
  virtual SizeVec3 getDimensions() const = 0;

  /**
   * @brief Sets the dimensions of the grid geometry.
   * @param dims The new dimensions in X, Y, and Z
   */
  virtual void setDimensions(const SizeVec3& dims) = 0;

  /**
   * @brief Returns the number of cells in the X dimension.
   * @return usize The number of cells in X
   */
  virtual usize getNumXCells() const = 0;

  /**
   * @brief Returns the number of cells in the Y dimension.
   * @return usize The number of cells in Y
   */
  virtual usize getNumYCells() const = 0;

  /**
   * @brief Returns the number of cells in the Z dimension.
   * @return usize The number of cells in Z
   */
  virtual usize getNumZCells() const = 0;

  /**
   * @brief Returns the plane coordinates at the specified index array as float32.
   * @param idx Array of indices [x, y, z]
   * @return Point3D<float32> The plane coordinates
   */
  virtual Point3D<float32> getPlaneCoordsf(usize idx[3]) const = 0;

  /**
   * @brief Returns the plane coordinates at the specified X, Y, Z indices as float32.
   * @param x The X index
   * @param y The Y index
   * @param z The Z index
   * @return Point3D<float32> The plane coordinates
   */
  virtual Point3D<float32> getPlaneCoordsf(usize x, usize y, usize z) const = 0;

  /**
   * @brief Returns the plane coordinates at the specified flat index as float32.
   * @param idx The flat index
   * @return Point3D<float32> The plane coordinates
   */
  virtual Point3D<float32> getPlaneCoordsf(usize idx) const = 0;

  /**
   * @brief Returns the plane coordinates at the specified index array as float64.
   * @param idx Array of indices [x, y, z]
   * @return Point3D<float64> The plane coordinates
   */
  virtual Point3D<float64> getPlaneCoords(usize idx[3]) const = 0;

  /**
   * @brief Returns the plane coordinates at the specified X, Y, Z indices as float64.
   * @param x The X index
   * @param y The Y index
   * @param z The Z index
   * @return Point3D<float64> The plane coordinates
   */
  virtual Point3D<float64> getPlaneCoords(usize x, usize y, usize z) const = 0;

  /**
   * @brief Returns the plane coordinates at the specified flat index as float64.
   * @param idx The flat index
   * @return Point3D<float64> The plane coordinates
   */
  virtual Point3D<float64> getPlaneCoords(usize idx) const = 0;

  /**
   * @brief Returns the cell center coordinates at the specified index array as float32.
   * @param idx Array of indices [x, y, z]
   * @return Point3D<float32> The cell center coordinates
   */
  virtual Point3D<float32> getCoordsf(usize idx[3]) const = 0;

  /**
   * @brief Returns the cell center coordinates at the specified X, Y, Z indices as float32.
   * @param x The X index
   * @param y The Y index
   * @param z The Z index
   * @return Point3D<float32> The cell center coordinates
   */
  virtual Point3D<float32> getCoordsf(usize x, usize y, usize z) const = 0;

  /**
   * @brief Returns the cell center coordinates at the specified flat index as float32.
   * @param idx The flat index
   * @return Point3D<float32> The cell center coordinates
   */
  virtual Point3D<float32> getCoordsf(usize idx) const = 0;

  /**
   * @brief Returns the cell center coordinates at the specified index array as float64.
   * @param idx Array of indices [x, y, z]
   * @return Point3D<float64> The cell center coordinates
   */
  virtual Point3D<float64> getCoords(usize idx[3]) const = 0;

  /**
   * @brief Returns the cell center coordinates at the specified X, Y, Z indices as float64.
   * @param x The X index
   * @param y The Y index
   * @param z The Z index
   * @return Point3D<float64> The cell center coordinates
   */
  virtual Point3D<float64> getCoords(usize x, usize y, usize z) const = 0;

  /**
   * @brief Returns the cell center coordinates at the specified flat index as float64.
   * @param idx The flat index
   * @return Point3D<float64> The cell center coordinates
   */
  virtual Point3D<float64> getCoords(usize idx) const = 0;

  /**
   * @brief Returns the flat index for the cell containing the specified coordinates (float32).
   * @param xCoord The X coordinate
   * @param yCoord The Y coordinate
   * @param zCoord The Z coordinate
   * @return std::optional<usize> The flat index if coordinates are within bounds, nullopt otherwise
   */
  virtual std::optional<usize> getIndex(float32 xCoord, float32 yCoord, float32 zCoord) const = 0;

  /**
   * @brief Returns the flat index for the cell containing the specified coordinates (float64).
   * @param xCoord The X coordinate
   * @param yCoord The Y coordinate
   * @param zCoord The Z coordinate
   * @return std::optional<usize> The flat index if coordinates are within bounds, nullopt otherwise
   */
  virtual std::optional<usize> getIndex(float64 xCoord, float64 yCoord, float64 zCoord) const = 0;

  /**
   * @brief Returns the optional ID of the cell data AttributeMatrix.
   * @return const std::optional<IdType>& The cell data ID if it exists
   */
  const std::optional<IdType>& getCellDataId() const;

  /**
   * @brief Returns a pointer to the cell data AttributeMatrix.
   * @return AttributeMatrix* Pointer to the cell data, or nullptr if not available
   */
  AttributeMatrix* getCellData();

  /**
   * @brief Returns a const pointer to the cell data AttributeMatrix.
   * @return const AttributeMatrix* Const pointer to the cell data, or nullptr if not available
   */
  const AttributeMatrix* getCellData() const;

  /**
   * @brief Returns a reference to the cell data AttributeMatrix.
   * @return AttributeMatrix& Reference to the cell data
   */
  AttributeMatrix& getCellDataRef();

  /**
   * @brief Returns a const reference to the cell data AttributeMatrix.
   * @return const AttributeMatrix& Const reference to the cell data
   */
  const AttributeMatrix& getCellDataRef() const;

  /**
   * @brief Returns the DataPath to the cell data AttributeMatrix.
   * @return DataPath The path to the cell data
   */
  DataPath getCellDataPath() const;

  /**
   * @brief Sets the cell data AttributeMatrix by copying from another AttributeMatrix.
   * @param attributeMatrix The AttributeMatrix to copy as cell data
   */
  void setCellData(const AttributeMatrix& attributeMatrix);

  /**
   * @brief Sets the cell data AttributeMatrix by ID.
   * @param id The optional ID of the AttributeMatrix to use as cell data
   */
  void setCellData(OptionalId id);

  /**
   * @brief validates that linkages between shared node lists and their associated Attribute Matrix is correct.
   * @return A Result<> object possibly with error code and message.
   */
  Result<> validate() const override;

protected:
  /**
   * @brief Constructs an IGridGeometry with the specified name.
   * @param dataStructure The DataStructure this geometry belongs to
   * @param name The name for this geometry
   */
  IGridGeometry(DataStructure& dataStructure, std::string name);

  /**
   * @brief Constructs an IGridGeometry with the specified name and import ID.
   * @param dataStructure The DataStructure this geometry belongs to
   * @param name The name for this geometry
   * @param importId The ID to use for this imported object
   */
  IGridGeometry(DataStructure& dataStructure, std::string name, IdType importId);

  /**
   * @brief Updates the array IDs. Should only be called by DataObject::checkUpdatedIds.
   * @param updatedIdsMap
   */
  void checkUpdatedIdsImpl(const std::unordered_map<DataObject::IdType, DataObject::IdType>& updatedIdsMap) override;

  std::optional<IdType> m_CellDataId;
};
} // namespace nx::core
