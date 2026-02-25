#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Result.hpp"
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

  /* We are leveraging the bounded nature of the following enum to expedite processing
   *
   * Steps for modification:
   * Tack new Type on the end of enum
   * Specify it's underlying value explicitly (increment of one from previous highest value)
   * Add the new typename to k_GeomTypeStrings
   *
   * DO NOT REORDER */
  enum class Type : uint32
  {
    Image = 0u,
    RectGrid = 1u,
    Vertex = 2u,
    Edge = 3u,
    Triangle = 4u,
    Quad = 5u,
    Tetrahedral = 6u,
    Hexahedral = 7u
  };

  inline static constexpr std::array<StringLiteral, 8> k_GeomTypeStrings = {"Image", "RectGrid", "Vertex", "Edge", "Triangle", "Quad", "Tetrahedral", "Hexahedral"};

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
   * @brief Pure-Virtual intended to calculate the sizes of each element
   * in the geometry and store it in a new or existing array in the datastructure
   * @param recalculate This will allow for skipping execution when an Element Sizes
   * Array exists and recalculate is `false`
   * @return Result<>
   */
  virtual Result<> findElementSizes(bool recalculate) = 0;

  /**
   * @brief Returns a pointer to the array containing element sizes.
   * @return const Float32Array* Pointer to the element sizes array, or nullptr if not available
   */
  const Float32Array* getElementSizes() const;

  /**
   * @brief Returns the optional ID of the element sizes array.
   * @return OptionalId The ID of the element sizes array if it exists
   */
  OptionalId getElementSizesId() const;

  /**
   * @brief Sets the ID of the element sizes array.
   * @param sizesId The ID of the element sizes array
   */
  void setElementSizesId(const OptionalId& sizesId);

  /**
   * @brief Deletes the element sizes array from the geometry.
   */
  void deleteElementSizes();

  /**
   * @brief Returns the parametric center of the geometry element.
   * @return Point3D<float64> The parametric center coordinates
   */
  virtual Point3D<float64> getParametricCenter() const = 0;

  /**
   * @brief Calculates shape functions at the given parametric coordinates.
   * @param pCoords The parametric coordinates
   * @param shape Output array to store the calculated shape function values
   */
  virtual void getShapeFunctions(const Point3D<float64>& pCoords, float64* shape) const = 0;

  /**
   * @brief Returns the dimensionality of the units used by the geometry.
   * @return uint32 The unit dimensionality (typically 1, 2, or 3)
   */
  uint32 getUnitDimensionality() const;

  /**
   * @brief Sets the dimensionality of the units used by the geometry.
   * @param value The unit dimensionality to set
   */
  void setUnitDimensionality(uint32 value);

  /**
   * @brief Returns the spatial dimensionality of the geometry.
   * @return uint32 The spatial dimensionality (typically 1, 2, or 3)
   */
  uint32 getSpatialDimensionality() const;

  /**
   * @brief Sets the spatial dimensionality of the geometry.
   * @param value The spatial dimensionality to set
   */
  void setSpatialDimensionality(uint32 value);

  /**
   * @brief Converts a set of geometry types to a set of their string representations.
   * @param geomTypes Set of geometry types to convert
   * @return std::set<std::string> Set of geometry type strings
   */
  static std::set<std::string> StringListFromGeometryType(const std::set<Type>& geomTypes);

  /**
   * @brief Returns a set of all available geometry types.
   * @return const std::set<Type>& Reference to the set of all geometry types
   */
  static const std::set<Type>& GetAllGeomTypes();

  /**
   * @brief Returns a vector of all available length unit strings.
   * @return const std::vector<std::string>& Reference to the vector of length unit strings
   */
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
   * @brief Converts a geometry type to its string representation.
   * @param geomType The geometry type to convert
   * @return std::string The string representation of the geometry type
   */
  static std::string GeomTypeToString(Type geomType);

  /**
   * @brief Converts a length unit enum to its string representation.
   * @param unit The length unit to convert
   * @return std::string The string representation of the length unit
   */
  static std::string LengthUnitToString(LengthUnit unit);

  /**
   * @brief validates that linkages between shared node lists and their associated Attribute Matrix is correct.
   * @return A Result<> object possibly with error code and message.
   */
  virtual Result<> validate() const = 0;

protected:
  /**
   * @brief Constructs an IGeometry with the specified name.
   * @param dataStructure The DataStructure this geometry belongs to
   * @param name The name for this geometry
   */
  IGeometry(DataStructure& dataStructure, std::string name);

  /**
   * @brief Constructs an IGeometry with the specified name and import ID.
   * @param dataStructure The DataStructure this geometry belongs to
   * @param name The name for this geometry
   * @param importId The ID to use for this imported object
   */
  IGeometry(DataStructure& dataStructure, std::string name, IdType importId);

  /**
   * @brief Updates the array IDs. Should only be called by DataObject::checkUpdatedIds.
   * @param updatedIdsMap
   */
  void checkUpdatedIdsImpl(const std::unordered_map<DataObject::IdType, DataObject::IdType>& updatedIdsMap) override;

  std::optional<IdType> m_ElementSizesId;

  LengthUnit m_Units = LengthUnit::Meter;
  uint32 m_UnitDimensionality = 3;
  uint32 m_SpacialDimensionality = 3;
};
} // namespace nx::core
