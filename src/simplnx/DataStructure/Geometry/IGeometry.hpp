#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DynamicListArray.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @class IGeometry
 * @brief Pure virtual interface for all geometry types.
 *
 * This interface defines the common contract that all geometries must satisfy.
 * Type aliases, enums, and string constants live here so that scope-resolution
 * references such as IGeometry::Type continue to compile unchanged.
 */
class SIMPLNX_EXPORT IGeometry
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

  static constexpr StringLiteral k_VoxelSizes = "Voxel Sizes";

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

  enum class LengthUnit : uint32
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

  virtual ~IGeometry() noexcept = default;

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
   * @brief validates that linkages between shared node lists and their associated Attribute Matrix is correct.
   * @return A Result<> object possibly with error code and message.
   */
  virtual Result<> validate() const = 0;

protected:
  IGeometry() = default;
  IGeometry(const IGeometry&) = default;
  IGeometry(IGeometry&&) = default;
  IGeometry& operator=(const IGeometry&) = default;
  IGeometry& operator=(IGeometry&&) noexcept = default;
};
} // namespace nx::core
