#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/AbstractGeometry.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"

namespace nx::core
{
class SIMPLNX_EXPORT AbstractGridGeometry : public AbstractGeometry, public IGridGeometry
{
public:
  static constexpr StringLiteral k_TypeName = "IGridGeometry";

  AbstractGridGeometry() = delete;
  AbstractGridGeometry(const AbstractGridGeometry&) = default;
  AbstractGridGeometry(AbstractGridGeometry&&) = default;

  AbstractGridGeometry& operator=(const AbstractGridGeometry&) = delete;
  AbstractGridGeometry& operator=(AbstractGridGeometry&&) noexcept = delete;

  ~AbstractGridGeometry() noexcept override = default;

  /**
   * @brief
   * @return SizeVec3
   */
  SizeVec3 getDimensions() const override = 0;

  /**
   * @brief
   * @param dims
   */
  void setDimensions(const SizeVec3& dims) override = 0;

  /**
   * @brief
   * @return usize
   */
  usize getNumXCells() const override = 0;

  /**
   * @brief
   * @return usize
   */
  usize getNumYCells() const override = 0;

  /**
   * @brief
   * @return usize
   */
  usize getNumZCells() const override = 0;

  /**
   * @brief
   * @param idx
   * @return Point3D<float32>
   */
  Point3D<float32> getPlaneCoordsf(usize idx[3]) const override = 0;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @return Point3D<float32>
   */
  Point3D<float32> getPlaneCoordsf(usize x, usize y, usize z) const override = 0;

  /**
   * @brief
   * @param idx
   * @return Point3D<float32>
   */
  Point3D<float32> getPlaneCoordsf(usize idx) const override = 0;

  /**
   * @brief
   * @param idx
   * @return Point3D<float64>
   */
  Point3D<float64> getPlaneCoords(usize idx[3]) const override = 0;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @return Point3D<float64>
   */
  Point3D<float64> getPlaneCoords(usize x, usize y, usize z) const override = 0;

  /**
   * @brief
   * @param idx
   * @return Point3D<float64>
   */
  Point3D<float64> getPlaneCoords(usize idx) const override = 0;

  /**
   * @brief
   * @param idx
   * @return Point3D<float32>
   */
  Point3D<float32> getCoordsf(usize idx[3]) const override = 0;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @return Point3D<float32>
   */
  Point3D<float32> getCoordsf(usize x, usize y, usize z) const override = 0;

  /**
   * @brief
   * @param idx
   * @return
   */
  Point3D<float32> getCoordsf(usize idx) const override = 0;

  /**
   * @brief
   * @param idx
   * @return Point3D<float64>
   */
  Point3D<float64> getCoords(usize idx[3]) const override = 0;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @return Point3D<float64>
   */
  Point3D<float64> getCoords(usize x, usize y, usize z) const override = 0;

  /**
   * @brief
   * @param idx
   * @return Point3D<float64>
   */
  Point3D<float64> getCoords(usize idx) const override = 0;

  /**
   * @brief
   * @param xCoord
   * @param yCoord
   * @param zCoord
   * @return std::optional<usize>
   */
  std::optional<usize> getIndex(float32 xCoord, float32 yCoord, float32 zCoord) const override = 0;

  /**
   * @brief
   * @param xCoord
   * @param yCoord
   * @param zCoord
   * @return std::optional<usize>
   */
  std::optional<usize> getIndex(float64 xCoord, float64 yCoord, float64 zCoord) const override = 0;

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
   * @brief
   * @param id
   */
  void setCellData(OptionalId id);

  /**
   * @brief validates that linkages between shared node lists and their associated Attribute Matrix is correct.
   * @return A Result<> object possibly with error code and message.
   */
  Result<> validate() const override;

protected:
  AbstractGridGeometry(DataStructure& dataStructure, std::string name);

  AbstractGridGeometry(DataStructure& dataStructure, std::string name, IdType importId);

  /**
   * @brief Updates the array IDs. Should only be called by AbstractDataObject::checkUpdatedIds.
   * @param updatedIdsMap
   */
  void checkUpdatedIdsImpl(const std::unordered_map<AbstractDataObject::IdType, AbstractDataObject::IdType>& updatedIdsMap) override;

  std::optional<IdType> m_CellDataId;
};
} // namespace nx::core
