#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace nx::core
{

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
/**
 * @brief Checks the consumed int32 partition-index cast and identifier addition.
 * @param partitionCount Gives the final or parameter-derived partition count.
 * @param startingFeatureId Gives the signed starting identifier.
 * @return An error if a consumed index or resulting identifier cannot fit int32.
 */
SIMPLNXCORE_EXPORT Result<> CheckPartitionIdRange(usize partitionCount, int32 startingFeatureId);
} // namespace detail

/**
 * @brief Converts a final coordinate to a partition index with float64 arithmetic.
 * @param grid Defines the final partition geometry.
 * @param coordinate Gives the consumed data coordinate.
 * @pre The grid tuple product fits usize and its spacing is positive and finite.
 * @return An index, no index outside coverage, or a conversion error.
 */
SIMPLNXCORE_EXPORT Result<std::optional<usize>> ComputeCheckedPartitionIndex(const ImageGeom& grid, const Point3D<float64>& coordinate);
/**
 * @brief Converts a final coordinate with the existing float32 arithmetic.
 * @param grid Defines the final partition geometry.
 * @param coordinate Gives the consumed data coordinate.
 * @pre The grid tuple product fits usize and its spacing is positive and finite.
 * @return An index, no index outside coverage, or a conversion error.
 */
SIMPLNXCORE_EXPORT Result<std::optional<usize>> ComputeCheckedPartitionIndex(const ImageGeom& grid, const Point3Df& coordinate);

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
   * @return Error for an unknown geometry or bulk I/O, or success after cancellation.
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
