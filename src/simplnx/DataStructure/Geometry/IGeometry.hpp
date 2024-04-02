#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/DynamicListArray.hpp"

namespace nx::core
{
class SIMPLNX_EXPORT IGeometry : public BaseGroup
{
public:
  friend class DataStructure;

  using StatusCode = int32;

  using MeshIndexType = uint64;
  using MeshIndexArrayType = DataArray<MeshIndexType>;
  using SharedVertexList = Float32Array;
  using SharedEdgeList = MeshIndexArrayType;
  using SharedFaceList = MeshIndexArrayType;
  using SharedTriList = MeshIndexArrayType;
  using SharedQuadList = MeshIndexArrayType;
  using SharedTetList = MeshIndexArrayType;
  using SharedHexList = MeshIndexArrayType;
  using ElementDynamicList = DynamicListArray<uint16, MeshIndexType>;

  static inline constexpr StringLiteral k_VoxelSizes = "Voxel Sizes";
  static inline constexpr StringLiteral k_TypeName = "IGeometry";

  enum class Type : uint32
  {
    Image,
    RectGrid,
    Vertex,
    Edge,
    Triangle,
    Quad,
    Tetrahedral,
    Hexahedral
  };

  enum class LengthUnit : EnumType
  {
    Yoctometer,
    Zeptometer,
    Attometer,
    Femtometer,
    Picometer,
    Nanometer,
    Micrometer,
    Millimeter,
    Centimeter,
    Decimeter,
    Meter,
    Decameter,
    Hectometer,
    Kilometer,
    Megameter,
    Gigameter,
    Terameter,
    Petameter,
    Exameter,
    Zettameter,
    Yottameter,
    Angstrom,
    Mil,
    Inch,
    Foot,
    Mile,
    Fathom,
    Unspecified = 100U,
    Unknown = 101U
  };

  IGeometry() = delete;

  IGeometry(const IGeometry&) = default;
  IGeometry(IGeometry&&) = default;

  IGeometry& operator=(const IGeometry&) = delete;
  IGeometry& operator=(IGeometry&&) noexcept = delete;

  ~IGeometry() noexcept override = default;

  /**
   * @brief Returns the type of geometry.
   * @return
   */
  virtual IGeometry::Type getGeomType() const = 0;

  /**
   * @brief Returns the number of Cells (NOT POINTS) of a Geometry
   * @return usize
   */
  virtual usize getNumberOfCells() const = 0;

  /**
   * @brief
   * @return StatusCode
   */
  virtual StatusCode findElementSizes() = 0;

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
  virtual Point3D<float64> getParametricCenter() const = 0;

  /**
   * @brief
   * @param pCoords
   * @param shape
   */
  virtual void getShapeFunctions(const Point3D<float64>& pCoords, float64* shape) const = 0;

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
   * @param unit
   * @return std::string
   */
  static std::string LengthUnitToString(LengthUnit unit);

  /**
   * @brief validates that linkages between shared node lists and their associated Attribute Matrix is correct.
   * @return A Result<> object possibly with error code and message.
   */
  virtual Result<> validateGeometry() const = 0;

protected:
  IGeometry(DataStructure& dataStructure, std::string name);

  IGeometry(DataStructure& dataStructure, std::string name, IdType importId);

  /**
   * @brief Updates the array IDs. Should only be called by DataObject::checkUpdatedIds.
   * @param updatedIds
   */
  void checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds) override;

  std::optional<IdType> m_ElementSizesId;

  LengthUnit m_Units = LengthUnit::Meter;
  uint32 m_UnitDimensionality = 3;
  uint32 m_SpacialDimensionality = 3;
};
} // namespace nx::core
