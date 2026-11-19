#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

#include <array>
#include <optional>

namespace nx::core
{
class RectGridGeom;

/**
 * @struct PartitionGeometryInputValues
 * @brief Stores partition-grid settings, paths, IDs, and mask options.
 */
struct SIMPLNXCORE_EXPORT PartitionGeometryInputValues
{
  ChoicesParameter::ValueType PartitioningMode;
  int32 StartingFeatureID;
  int32 OutOfBoundsFeatureID;
  VectorInt32Parameter::ValueType NumberOfCellsPerAxis;
  VectorFloat32Parameter::ValueType PartitionGridOrigin;
  VectorFloat32Parameter::ValueType CellLength;
  VectorFloat32Parameter::ValueType MinGridCoord;
  VectorFloat32Parameter::ValueType MaxGridCoord;
  DataPath InputGeomCellAMPath;
  DataPath PartitionGridGeomPath;
  std::string PartitionGridCellAMName;
  std::string PartitionGridFeatureIDsArrayName;
  DataPath InputGeometryToPartition;
  std::string PartitionIdsArrayName;
  DataPath ExistingPartitionGridPath;
  bool UseVertexMask;
  DataPath VertexMaskPath;
  std::string FeatureAttrMatrixName;
};

namespace detail
{
inline constexpr int32 k_UnsupportedPartitionGeometry = -86216;

// The filter checks requested grids in preflight. The algorithm checks existing grids at execution.
Result<> CheckPartitionIdRange(usize partitionCount, int32 startingFeatureId);

/**
 * @brief Checks a single axis for the Direct algorithm's fast path.
 * @param coordinate Gives the input cell center.
 * @param dimension Gives the partition grid axis length.
 * @param origin Gives the partition grid origin.
 * @param spacing Gives the partition grid spacing.
 * @return A mapped index, dimension for an exterior point, or no value if the conversion is unsafe.
 */
std::optional<usize> TryMapPartitionAxis(float64 coordinate, usize dimension, float32 origin, float32 spacing);

/**
 * @class RectGridPartitionCenters
 * @brief Reads rectilinear cell centers through bounded blocks of adjacent bounds.
 */
class RectGridPartitionCenters
{
public:
  /**
   * @brief Initializes the reader without accessing bound values.
   * @param grid Supplies the rectilinear bounds and must outlive the reader.
   */
  explicit RectGridPartitionCenters(const RectGridGeom& grid);

  /**
   * @brief Reads one center coordinate.
   * @param axis Selects X, Y or Z.
   * @param index Identifies a cell on the selected axis.
   * @return The center coordinate or a store read error.
   */
  Result<float64> readAxis(usize axis, usize index);

  /**
   * @brief Reads the three center coordinates of a cell.
   * @param index Identifies the cell in X-fastest order.
   * @return The coordinates or a store read error.
   */
  Result<Point3D<float64>> readTuple(usize index);

private:
  static constexpr usize k_ValuesPerBlock = 4096;
  struct BoundCache
  {
    std::array<float32, k_ValuesPerBlock> Values;
    usize Start = 0;
    usize Size = 0;
  };
  std::array<const AbstractDataStore<float32>*, 3> m_Bounds;
  SizeVec3 m_Dimensions;
  std::array<BoundCache, 3> m_Caches;
};
} // namespace detail

/**
 * @brief Converts a final coordinate to a partition index.
 * @param grid Defines the final partition geometry.
 * @param coordinate Gives the input coordinate and sets the arithmetic precision.
 * @param inputPath Identifies the geometry that supplies the coordinate.
 * @param elementIndex Identifies the coordinate within the geometry.
 * @pre The grid tuple product fits usize. Finite spacing values are positive.
 * @return An index, no index outside coverage, or a conversion error.
 */
Result<std::optional<usize>> ComputeCheckedPartitionIndex(const ImageGeom& grid, const Point3D<float64>& coordinate, const DataPath& inputPath, usize elementIndex);
Result<std::optional<usize>> ComputeCheckedPartitionIndex(const ImageGeom& grid, const Point3Df& coordinate, const DataPath& inputPath, usize elementIndex);

/**
 * @class PartitionGeometry
 * @brief Dispatches geometry partitioning from selected array storage.
 *
 * Dispatch targets include output IDs, created partition-grid IDs, node vertices,
 * and an optional vertex mask. RectGrid coordinate arrays are not dispatch targets.
 */
class SIMPLNXCORE_EXPORT PartitionGeometry
{
public:
  /**
   * @brief Defines the scalar store used for node coordinates.
   */
  using VertexStore = AbstractDataStore<IGeometry::SharedVertexList::value_type>;

  /**
   * @brief Creates a geometry-partition dispatcher.
   * @param dataStructure Provides input geometry and partition outputs.
   * @param msgHandler Is retained for the dispatched interface.
   * @param shouldCancel Stops later initialization or partition work when true.
   * @param inputValues Specifies validated paths and partition settings. The caller
   * must keep this object alive for the dispatcher lifetime.
   */
  PartitionGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, PartitionGeometryInputValues* inputValues);
  /**
   * @brief Destroys the non-owning dispatcher.
   */
  ~PartitionGeometry() noexcept;

  PartitionGeometry(const PartitionGeometry&) = delete;
  PartitionGeometry(PartitionGeometry&&) noexcept = delete;
  PartitionGeometry& operator=(const PartitionGeometry&) = delete;
  PartitionGeometry& operator=(PartitionGeometry&&) noexcept = delete;

  /**
   * @struct PSGeomInfo
   * @brief Stores dimensions and spatial metadata for a partition grid.
   */
  struct PSGeomInfo
  {
    USizeVec3 geometryDims;
    std::optional<FloatVec3> geometryOrigin;
    std::optional<FloatVec3> geometrySpacing;
    IGeometry::LengthUnit geometryUnits;
  };

  /**
   * @brief Selects direct or scanline partitioning from dispatch-target storage.
   * @return Error for invalid grid dimensions, unsafe coordinate mapping, unsupported geometry or bulk I/O.
   *
   * Cancellation or an I/O error can retain partial partition IDs. Created
   * partition-grid Feature IDs can also remain partial.
   * @pre Active created-grid counts are positive with representable products and IDs. Geometry, output and mask selections are valid.
   * @pre In Advanced mode, Partition Grid Origin is finite and Cell Length has finite positive components.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  PartitionGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
