#pragma once

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/Geometry/IGeometry.hpp"

namespace complex
{
class COMPLEX_EXPORT IGridGeometry : public IGeometry
{
public:
  static inline constexpr StringLiteral k_CellDataName = "Cell Data";

  ~IGridGeometry() noexcept override = default;

  /**
   * @brief
   * @return SizeVec3
   */
  virtual SizeVec3 getDimensions() const = 0;

  /**
   * @brief
   * @param dims
   */
  virtual void setDimensions(const SizeVec3& dims) = 0;

  /**
   * @brief
   * @return usize
   */
  virtual usize getNumXPoints() const = 0;

  /**
   * @brief
   * @return usize
   */
  virtual usize getNumYPoints() const = 0;

  /**
   * @brief
   * @return usize
   */
  virtual usize getNumZPoints() const = 0;

  /**
   * @brief
   * @param idx
   * @return Point3D<float32>
   */
  virtual Point3D<float32> getPlaneCoordsf(usize idx[3]) const = 0;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @return Point3D<float32>
   */
  virtual Point3D<float32> getPlaneCoordsf(usize x, usize y, usize z) const = 0;

  /**
   * @brief
   * @param idx
   * @return Point3D<float32>
   */
  virtual Point3D<float32> getPlaneCoordsf(usize idx) const = 0;

  /**
   * @brief
   * @param idx
   * @return Point3D<float64>
   */
  virtual Point3D<float64> getPlaneCoords(usize idx[3]) const = 0;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @return Point3D<float64>
   */
  virtual Point3D<float64> getPlaneCoords(usize x, usize y, usize z) const = 0;

  /**
   * @brief
   * @param idx
   * @return Point3D<float64>
   */
  virtual Point3D<float64> getPlaneCoords(usize idx) const = 0;

  /**
   * @brief
   * @param idx
   * @return Point3D<float32>
   */
  virtual Point3D<float32> getCoordsf(usize idx[3]) const = 0;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @preturn Point3D<float32>
   */
  virtual Point3D<float32> getCoordsf(usize x, usize y, usize z) const = 0;

  /**
   * @brief
   * @param idx
   * @return
   */
  virtual Point3D<float32> getCoordsf(usize idx) const = 0;

  /**
   * @brief
   * @param idx
   * @return Point3D<float64>
   */
  virtual Point3D<float64> getCoords(usize idx[3]) const = 0;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @return Point3D<float64>
   */
  virtual Point3D<float64> getCoords(usize x, usize y, usize z) const = 0;

  /**
   * @brief
   * @param idx
   * @return Point3D<float64>
   */
  virtual Point3D<float64> getCoords(usize idx) const = 0;

  /**
   * @brief
   * @param xCoord
   * @param yCoord
   * @param zCoord
   * @return usize
   */
  virtual usize getIndex(float32 xCoord, float32 yCoord, float32 zCoord) const = 0;

  /**
   * @brief
   * @param xCoord
   * @param yCoord
   * @param zCoord
   * @return usize
   */
  virtual usize getIndex(float64 xCoord, float64 yCoord, float64 zCoord) const = 0;

  /**
   * @brief
   * @return
   */
  const std::optional<IdType>& getCellDataId() const;

  /**
   * @brief
   * @return
   */
  AttributeMatrix* getCellData();

  /**
   * @brief
   * @return
   */
  const AttributeMatrix* getCellData() const;

  /**
   * @brief
   * @return
   */
  AttributeMatrix& getCellDataRef();

  /**
   * @brief
   * @return
   */
  const AttributeMatrix& getCellDataRef() const;

  /**
   * @brief
   * @return
   */
  DataPath getCellDataPath() const;

  /**
   * @brief
   * @param attributeMatrix
   */
  void setCellData(const AttributeMatrix& attributeMatrix);

  /**
   * @brief Reads values from HDF5
   * @param groupReader
   * @return H5::ErrorType
   */
  H5::ErrorType readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, bool preflight = false) override;

  /**
   * @brief Writes the geometry to HDF5 using the provided parent group ID.
   * @param dataStructureWriter
   * @param parentGroupWriter
   * @param importable
   * @return H5::ErrorType
   */
  H5::ErrorType writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter, bool importable) const override;

protected:
  IGridGeometry() = delete;

  IGridGeometry(DataStructure& ds, std::string name);

  IGridGeometry(DataStructure& ds, std::string name, IdType importId);

  IGridGeometry(const IGridGeometry&) = default;
  IGridGeometry(IGridGeometry&&) noexcept = default;

  IGridGeometry& operator=(const IGridGeometry&) = delete;
  IGridGeometry& operator=(IGridGeometry&&) noexcept = delete;

  /**
   * @brief Updates the array IDs. Should only be called by DataObject::checkUpdatedIds.
   * @param updatedIds
   */
  void checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds) override;

  std::optional<IdType> m_CellDataId;
};
} // namespace complex
