#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/DynamicListArray.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"

namespace nx::core
{
class SIMPLNX_EXPORT AbstractGeometry : public BaseGroup, public IGeometry
{
public:
  friend class DataStructure;

  // Bring IGeometry nested types into scope to resolve ambiguity with AbstractDataObject::Type
  using IGeometry::ElementDynamicList;
  using IGeometry::LengthUnit;
  using IGeometry::MeshIndexArrayType;
  using IGeometry::MeshIndexType;
  using IGeometry::SharedEdgeList;
  using IGeometry::SharedFaceList;
  using IGeometry::SharedHexList;
  using IGeometry::SharedQuadList;
  using IGeometry::SharedTetList;
  using IGeometry::SharedTriList;
  using IGeometry::SharedVertexList;
  using IGeometry::Type;

  static constexpr StringLiteral k_TypeName = "IGeometry";

  AbstractGeometry() = delete;

  AbstractGeometry(const AbstractGeometry&) = default;
  AbstractGeometry(AbstractGeometry&&) = default;

  AbstractGeometry& operator=(const AbstractGeometry&) = delete;
  AbstractGeometry& operator=(AbstractGeometry&&) noexcept = delete;

  ~AbstractGeometry() noexcept override = default;

  /**
   * @brief Returns the type of geometry.
   * @return
   */
  IGeometry::Type getGeomType() const override = 0;

  /**
   * @brief Returns the number of Cells (NOT POINTS) of a Geometry
   * @return usize
   */
  usize getNumberOfCells() const override = 0;

  /**
   * @brief
   * @return Result<>
   */
  Result<> findElementSizes(bool recalculate) override = 0;

  /**
   * @brief
   * @return const Float32Array*
   */
  const Float32Array* getElementSizes() const;

  OptionalId getElementSizesId() const;

  void setElementSizesId(const OptionalId& sizesId);

  /**
   * @brief
   */
  void deleteElementSizes();

  /**
   * @brief
   * @return Point3D<float64>
   */
  Point3D<float64> getParametricCenter() const override = 0;

  /**
   * @brief
   * @param pCoords
   * @param shape
   */
  void getShapeFunctions(const Point3D<float64>& pCoords, float64* shape) const override = 0;

  /**
   * @brief
   * @return uint32
   */
  uint32 getUnitDimensionality() const;

  /**
   * @brief
   * @param value
   */
  void setUnitDimensionality(uint32 value);

  /**
   * @brief
   * @return uint32
   */
  uint32 getSpatialDimensionality() const;

  /**
   * @brief
   * @param value
   */
  void setSpatialDimensionality(uint32 value);

  static std::set<std::string> StringListFromGeometryType(const std::set<Type>& geomTypes);

  static const std::set<Type>& GetAllGeomTypes();

  static const std::vector<std::string>& GetAllLengthUnitStrings();

  /**
   * @brief Returns the length units used by the geometry.
   * @return LengthUnit
   */
  LengthUnit getUnits() const;

  /**
   * @brief Sets the length units used by the geometry.
   * @param units
   */
  void setUnits(LengthUnit units);

  /**
   * @brief
   * @param geomType
   * @return std::string
   */
  static std::string GeomTypeToString(Type geomType);

  /**
   * @brief
   * @param unit
   * @return std::string
   */
  static std::string LengthUnitToString(LengthUnit unit);

  /**
   * @brief validates that linkages between shared node lists and their associated Attribute Matrix is correct.
   * @return A Result<> object possibly with error code and message.
   */
  Result<> validate() const override = 0;

protected:
  AbstractGeometry(DataStructure& dataStructure, std::string name);

  AbstractGeometry(DataStructure& dataStructure, std::string name, IdType importId);

  /**
   * @brief Updates the array IDs. Should only be called by AbstractDataObject::checkUpdatedIds.
   * @param updatedIdsMap
   */
  void checkUpdatedIdsImpl(const std::unordered_map<AbstractDataObject::IdType, AbstractDataObject::IdType>& updatedIdsMap) override;

  std::optional<IdType> m_ElementSizesId;

  LengthUnit m_Units = LengthUnit::Meter;
  uint32 m_UnitDimensionality = 3;
  uint32 m_SpacialDimensionality = 3;
};
} // namespace nx::core
